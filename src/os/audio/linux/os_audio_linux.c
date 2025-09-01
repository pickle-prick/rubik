/////////////////////////////////////////////////////////////////////////////////////////
// Audio Backend Includes

#include "external/miniaudio/miniaudio.c"

/////////////////////////////////////////////////////////////////////////////////////////
// @os_hooks Main Initialization API (Implemented Per-OS)

// global state
internal void os_audio_init(void)
{
  Arena *arena = arena_alloc();
  os_lnx_audio_state = push_array(arena, OS_LNX_AudioState, 1);
  os_lnx_audio_state->arena = arena;

  // init ma context
  ma_context_config ctx_cfg = ma_context_config_init();
  ma_result err = ma_context_init(0, 0, &ctx_cfg, &os_lnx_audio_state->context);
  AssertAlways(err == MA_SUCCESS);
  err = ma_mutex_init(&os_lnx_audio_state->lock);
  Assert(err == MA_SUCCESS);
}

internal void
os_set_main_audio_device(OS_Handle handle)
{
  OS_LNX_AudioDevice *device = os_lnx_audio_device_from_handle(handle);
  os_lnx_audio_state->main_device = device;
}

internal void
os_audio_set_master_volume(F32 volume)
{
  OS_LNX_AudioDevice *device = os_lnx_audio_state->main_device;
  if(device)
  {
    ma_mutex_lock(&os_lnx_audio_state->lock);
    device->master_volume = volume;
    ma_result err = ma_device_set_master_volume(&device->m_device, volume);
    AssertAlways(err == MA_SUCCESS);
    ma_mutex_unlock(&os_lnx_audio_state->lock);
  }
}

// audio thread lock

internal void os_audio_thread_lock()
{
  ma_mutex_lock(&os_lnx_audio_state->lock);
}

internal void os_audio_thread_release()
{
  ma_mutex_unlock(&os_lnx_audio_state->lock);
}

// device

internal void
os_lnx_audio_frames_mix(F32 *dst, F32 *src, U64 frame_count, F32 volume, F32 pan)
{
  U64 channel_count = os_lnx_audio_state->main_device->m_device.playback.channels;

  // considering pan if it's stereo
  if(channel_count == 2)
  {
    F32 left = pan;
    F32 right = 1.0 - left;

    // fast sine approximation in [0..1] for pan law: y = 0.5f*x*(3 - x*x);
    F32 levels[2] = { volume*0.5f*left*(3.0f - left*left), volume*0.5f*right*(3.0f - right*right) };

    for(U64 frame_index = 0; frame_index < frame_count; frame_index++)
    {
      dst[0] += src[0]*levels[0];
      dst[1] += src[1]*levels[1];
      dst += 2;
      src += 2;
    }
  }
  else
  {
    for(U64 sample_index = 0; sample_index < frame_count*channel_count; sample_index++)
    {
      // output accumulates input multiplied by volume
      *dst += (*src) * volume;
      dst++;
      src++;
    }
  }
}

// this function will be called when miniaudio needs more data
// all the mixing takes place here
internal void
os_lnx_device_output_callback(ma_device *device, void *frames_out, const void *frames_in, ma_uint32 frame_count)
{
  // NOTE(k): this thread is spawned by ma, we can't safely use scratch arena here

  // mixing is basically just an accumulation, we need to initialize the output buffer to 0
  MemoryZero(frames_out, frame_count*device->playback.channels*ma_get_bytes_per_sample(device->playback.format));

  // TODO(k): using a mutex here for thread-safety which makes things not real-time
  // this is unlikely to be necessary for this project, but may want to consider how you might want to avoid this
  ma_mutex_lock(&os_lnx_audio_state->lock);

  U64 channel_count = os_lnx_audio_state->main_device->m_device.playback.channels;
  U64 sample_count = frame_count*channel_count;

  Temp scratch = scratch_begin(0,0);
  F32 *dst = (F32*)frames_out;
  F32 *src = push_array(scratch.arena, F32, sample_count);
  B32 dirty = 0;
  for(OS_LNX_AudioBuffer *buffer = os_lnx_audio_state->first_process_audio_buffer; buffer != 0; buffer = buffer->next)
  {
    if(buffer->playing || (!buffer->paused))
    {
      if(dirty)
      {
        MemoryZero(src, sizeof(F32)*sample_count);
      }

      switch(buffer->kind)
      {
        case OS_LNX_AudioBufferKind_Stream:
        {
          // buffer output callback
          // TODO: read it from it's internal format
          buffer->output_callback((void*)src, frame_count, channel_count);
          buffer->frames_processed += frame_count;
          dirty = 1;
        }break;
        case OS_LNX_AudioBufferKind_Static:
        {
          U64 frame_count_to_read = frame_count;
          // U64 frame_count_read = 0;
          U64 buffer_channel_count = (buffer->byte_count/buffer->frame_count) / sizeof(S16);

          F32 *dst = src;
          S16 *src = &((S16*)buffer->bytes)[buffer->frame_cursor_pos*buffer_channel_count];
          AssertAlways(buffer->frame_count > 0);

          while(frame_count_to_read > 0)
          {
            if(!(buffer->frame_cursor_pos < buffer->frame_count))
            {
              buffer->frame_cursor_pos = 0;
              buffer->frames_processed = 0;
              if(buffer->looping)
              {
                src = &((S16*)buffer->bytes)[buffer->frame_cursor_pos*buffer_channel_count];
              }
              else
              {
                buffer->paused = 1;
                buffer->playing = 0;
                break;
              }
            }

            // NOTE(k): since we are loading all sound/music as s16, we need to convert it into f32
            if(buffer_channel_count == 1)
            {
              *(dst++) = *src / (F32)max_S16;
              *(dst++) = *src / (F32)max_S16;
              src++;
            }
            else if(buffer_channel_count == 2)
            {
              *(dst++) = *(src++) / (F32)max_S16;
              *(dst++) = *(src++) / (F32)max_S16;
            }
            else {NotImplemented;}

            buffer->frame_cursor_pos++;
            buffer->frames_processed++;

            frame_count_to_read--;
            // frame_count_read++;
          }

          dirty = 1;
        }break;
        default:{InvalidPath;}break;
      }

      // TODO: static buffer won't have to fill all frames
      if(dirty) os_lnx_audio_frames_mix(dst, src, frame_count, buffer->volume, buffer->pan);
    }
  }
  scratch_end(scratch);
  ma_mutex_unlock(&os_lnx_audio_state->lock);
}

// device
internal OS_Handle
os_audio_device_open(void)
{
  OS_LNX_AudioDevice *device = os_lnx_audio_state->first_free_audio_device;
  if(device)
  {
    SLLStackPop(os_lnx_audio_state->first_free_audio_device);
  }
  else
  {
    device = push_array_no_zero(os_lnx_audio_state->arena, OS_LNX_AudioDevice, 1);
  }
  MemoryZeroStruct(device);

  // unpack params
  // NOTE(k): using f32 as format because it simplifies mixing
  OS_AudioFormat playback_format = OS_LNX_AUDIO_DEVICE_PLAYBACK_FORMAT;
  U64 playback_channel_count = OS_LNX_AUDIO_DEVICE_PLAYBACK_CHANNELS;
  OS_AudioFormat capture_format = OS_AudioFormat_S16;
  U64 capture_channel_count = 1;
  U64 sample_rate = OS_LNX_AUDIO_DEVICE_SAMPLE_RATE; // NOTE(k): we could set it to 0 to use device default sample rate 

  struct
  {
    OS_AudioFormat src;
    ma_format dst;
  } format_map[] =
  {
    {OS_AudioFormat_NULL, ma_format_unknown},
    {OS_AudioFormat_U8,   ma_format_u8},
    {OS_AudioFormat_S16,  ma_format_s16},
    {OS_AudioFormat_S24,  ma_format_s24},
    {OS_AudioFormat_S32,  ma_format_s32},
    {OS_AudioFormat_F32,  ma_format_f32},
  };

  ma_result err;
  ma_device_config config   = ma_device_config_init(ma_device_type_playback);
  config.playback.pDeviceID = 0; // NULl for the default playback device
  config.playback.format    = format_map[playback_format].dst; // NOTE: using the default device we are using floating point as format because it simplifies mixing
  config.playback.channels  = playback_channel_count;
  config.capture.pDeviceID  = 0; // NULL for the default capture device
  config.capture.format     = format_map[capture_format].dst;
  config.capture.channels   = capture_channel_count;
  config.sampleRate         = sample_rate;
  config.dataCallback       = os_lnx_device_output_callback;
  config.pUserData          = NULL;

  err = ma_device_init(&os_lnx_audio_state->context, &config, &device->m_device);
  Assert(err == MA_SUCCESS);

  OS_Handle ret = os_lnx_handle_from_audio_device(device);
  return ret;
}

internal void
os_audio_device_close(OS_Handle handle)
{
  NotImplemented;
}

internal void
os_audio_device_start(OS_Handle handle)
{
  OS_LNX_AudioDevice *device = os_lnx_audio_device_from_handle(handle);
  if(device)
  {
    ma_result err = ma_device_start(&device->m_device);
    AssertAlways(err == MA_SUCCESS);
  }
}

internal void
os_audio_device_stop(OS_Handle handle)
{
  OS_LNX_AudioDevice *device = os_lnx_audio_device_from_handle(handle);
  if(device)
  {
    ma_result err = ma_device_stop(&device->m_device);
    Assert(err == MA_SUCCESS);
  }
}

internal OS_AudioDeviceConfig
os_audio_deivce_cfg(OS_Handle handle)
{
  OS_AudioDeviceConfig ret = {0};
  OS_LNX_AudioDevice *device = os_lnx_audio_device_from_handle(handle);
  if(device)
  {
    ret.state = (OS_AudioDeviceState)ma_device_get_state(&device->m_device);
    ret.sample_rate = device->m_device.sampleRate;

    ret.playback.name = str8_cstring(device->m_device.playback.name);
    ret.playback.format = (OS_AudioFormat)device->m_device.playback.format;
    ret.playback.channel_count = device->m_device.playback.channels;

    ret.capture.name = str8_cstring(device->m_device.capture.name);
    ret.capture.format = (OS_AudioFormat)device->m_device.capture.format;
    ret.capture.channel_count = device->m_device.capture.channels;
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
// audio stream functions

internal OS_Handle
os_audio_stream_alloc(U32 sample_rate, U32 sample_size, U32 channel_count)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_state->first_free_audio_stream;
  if(stream)
  {
    SLLStackPop(os_lnx_audio_state->first_free_audio_stream);
  }
  else
  {
    stream = push_array_no_zero(os_lnx_audio_state->arena, OS_LNX_AudioStream, 1);
  }
  MemoryZeroStruct(stream);

  stream->sample_rate   = sample_rate;
  stream->sample_size   = sample_size;
  stream->channel_count = channel_count;

  OS_LNX_AudioBuffer *audio_buffer = os_lnx_audio_buffer_alloc(0);
  audio_buffer->kind = OS_LNX_AudioBufferKind_Stream;
  stream->buffer = audio_buffer;

  OS_Handle ret = os_lnx_handle_from_audio_stream(stream);
  return ret;
}

internal void
os_lnx_audio_buffer_play(OS_LNX_AudioBuffer *buffer)
{
  if(buffer)
  {
    ma_mutex_lock(&os_lnx_audio_state->lock);
    buffer->playing = 1;
    buffer->paused = 0;
    buffer->frame_cursor_pos = 0;
    buffer->frames_processed = 0;
    ma_mutex_unlock(&os_lnx_audio_state->lock);
  }
}

internal void
os_lnx_audio_buffer_pause(OS_LNX_AudioBuffer *buffer)
{
  if(buffer)
  {
    ma_mutex_lock(&os_lnx_audio_state->lock);
    buffer->playing = 0;
    buffer->paused = 1;
    ma_mutex_unlock(&os_lnx_audio_state->lock);
  }
}

internal void
os_lnx_audio_buffer_resume(OS_LNX_AudioBuffer *buffer)
{
  if(buffer)
  {
    ma_mutex_lock(&os_lnx_audio_state->lock);
    buffer->playing = 1;
    buffer->paused = 0;
    ma_mutex_unlock(&os_lnx_audio_state->lock);
  }
}

internal void
os_lnx_audio_buffer_set_output_callback(OS_LNX_AudioBuffer *buffer, OS_AudioOutputCallback cb)
{
  if(buffer)
  {
    ma_mutex_lock(&os_lnx_audio_state->lock);
    buffer->output_callback = cb;
    ma_mutex_unlock(&os_lnx_audio_state->lock);
  }
}

internal void
os_lnx_audio_buffer_set_volume(OS_LNX_AudioBuffer *buffer, F32 volume)
{
  if(buffer)
  {
    ma_mutex_lock(&os_lnx_audio_state->lock);
    buffer->volume = volume;
    ma_mutex_unlock(&os_lnx_audio_state->lock);
  }
}

internal void
os_lnx_audio_buffer_set_looping(OS_LNX_AudioBuffer *buffer, B32 looping)
{
  if(buffer)
  {
    ma_mutex_lock(&os_lnx_audio_state->lock);
    buffer->looping = looping;
    ma_mutex_unlock(&os_lnx_audio_state->lock);
  }
}

internal void
os_lnx_audio_buffer_set_pan(OS_LNX_AudioBuffer *buffer, F32 pan)
{
  if(buffer)
  {
    ma_mutex_lock(&os_lnx_audio_state->lock);
    buffer->pan = pan;
    ma_mutex_unlock(&os_lnx_audio_state->lock);
  }
}

internal void
os_audio_stream_play(OS_Handle handle)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    os_lnx_audio_buffer_play(stream->buffer);
  }
}

internal void
os_audio_stream_pause(OS_Handle handle)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    os_lnx_audio_buffer_pause(stream->buffer);
  }
}

internal void
os_audio_stream_resume(OS_Handle handle)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    os_lnx_audio_buffer_resume(stream->buffer);
  }
}

internal void
os_audio_stream_set_output_callback(OS_Handle handle, OS_AudioOutputCallback cb)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    os_lnx_audio_buffer_set_output_callback(stream->buffer, cb);
  }
}

internal void
os_audio_stream_set_volume(OS_Handle handle, F32 volume)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    os_lnx_audio_buffer_set_volume(stream->buffer, volume);
  }
}

internal void
os_audio_stream_set_pan(OS_Handle handle, F32 pan)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    os_lnx_audio_buffer_set_pan(stream->buffer, pan);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Sound

internal OS_LNX_AudioBuffer *
os_lnx_audio_buffer_alloc(U64 byte_count)
{
  OS_LNX_AudioBuffer *ret = 0;

  U8 *bytes = 0;
  U64 byte_cap = 0;

  // TODO: some buffer is used as streaming, so do we need to handle them differently

  for(OS_LNX_AudioBuffer *candidate = os_lnx_audio_state->first_free_audio_buffer;
      candidate != 0;
      candidate = candidate->next)
  {
    if(candidate->byte_cap >= byte_count)
    {
      F32 waste_tolerance_pct = 0.1;
      U64 waste_byte_count_allowed = candidate->byte_cap*waste_tolerance_pct;
      if((candidate->byte_cap-byte_count) <= waste_byte_count_allowed)
      {
        DLLRemove(os_lnx_audio_state->first_free_audio_buffer,
                  os_lnx_audio_state->last_free_audio_buffer,
                  candidate);
        byte_cap = candidate->byte_cap;
        bytes = candidate->bytes;
        ret = candidate;
        break;
      }
    }
  }

  if(ret == 0)
  {
    bytes = push_array(os_lnx_audio_state->arena, U8, byte_count);
    byte_cap = byte_count;
    ret = push_array_no_zero(os_lnx_audio_state->arena, OS_LNX_AudioBuffer, 1);
  }

  MemoryZeroStruct(ret);
  ret->bytes = bytes;
  ret->byte_count = byte_count;
  ret->byte_cap = byte_cap;
  ret->volume = 1.0;
  ret->pan = 0.5;
  ret->paused = 1;
  // push it into stack for processing/mixing
  DLLPushBack(os_lnx_audio_state->first_process_audio_buffer, os_lnx_audio_state->last_process_audio_buffer, ret);
  return ret;
}

internal void
os_lnx_audio_buffer_release(OS_LNX_AudioBuffer *audio_buffer)
{
  DLLRemove(os_lnx_audio_state->first_process_audio_buffer, 
            os_lnx_audio_state->last_process_audio_buffer,
            audio_buffer);
  DLLPushBack(os_lnx_audio_state->first_free_audio_buffer,
              os_lnx_audio_state->last_free_audio_buffer,
              audio_buffer);
}

internal OS_LNX_Sound *
os_lnx_sound_alloc(void)
{
  OS_LNX_Sound *ret = os_lnx_audio_state->first_free_sound;
  if(ret)
  {
    SLLStackPop(os_lnx_audio_state->first_free_sound);
  }
  else
  {
    ret = push_array_no_zero(os_lnx_audio_state->arena, OS_LNX_Sound, 1);
  }
  MemoryZeroStruct(ret);
  return ret;
}

internal void
os_lnx_sound_release(OS_LNX_Sound *sound)
{
  os_lnx_audio_buffer_release(sound->buffer);
  SLLStackPush(os_lnx_audio_state->first_free_sound, sound);
}

internal OS_Handle
os_sound_from_file(char *filename)
{
  Temp scratch = scratch_begin(0,0);
  OS_AudioWave *wave = os_wave_from_file(scratch.arena, filename);
  OS_Handle ret = os_sound_from_wave(wave);
  scratch_end(scratch);
  return ret;
}

internal OS_Handle
os_sound_from_wave(OS_AudioWave *wave)
{
  OS_LNX_Sound *sound = os_lnx_sound_alloc();
  OS_LNX_AudioBuffer *audio_buffer = os_lnx_audio_buffer_alloc(wave->byte_count);
  audio_buffer->frame_count = wave->frame_count;
  MemoryCopy(audio_buffer->bytes, wave->bytes, wave->byte_count);

  sound->buffer = audio_buffer;
  sound->frame_count = wave->frame_count;
  return os_lnx_handle_from_sound(sound);
}

internal void
os_sound_set_volume(OS_Handle handle, F32 volume)
{
  OS_LNX_Sound *sound = os_lnx_sound_from_handle(handle);
  if(sound)
  {
    os_lnx_audio_buffer_set_volume(sound->buffer, volume);
  }
}

internal void
os_sound_set_looping(OS_Handle handle, B32 looping)
{
  OS_LNX_Sound *sound = os_lnx_sound_from_handle(handle);
  if(sound)
  {
    os_lnx_audio_buffer_set_looping(sound->buffer, looping);
  }
}

internal void
os_sound_play(OS_Handle handle)
{
  OS_LNX_Sound *sound = os_lnx_sound_from_handle(handle);
  if(sound)
  {
    os_lnx_audio_buffer_play(sound->buffer);
  }
}

internal void
os_sound_pause(OS_Handle handle)
{

}

internal void
os_sound_resume(OS_Handle handle)
{

}

/////////////////////////////////////////////////////////////////////////////////////////
// handle

internal OS_Handle
os_lnx_handle_from_audio_device(OS_LNX_AudioDevice *device)
{
  OS_Handle ret = {0};
  if(device)
  {
    ret.u64[0] = (U64)device;
  }
  return ret;
}

internal OS_LNX_AudioDevice *
os_lnx_audio_device_from_handle(OS_Handle handle)
{
  OS_LNX_AudioDevice *ret = (OS_LNX_AudioDevice*)handle.u64[0];
  return ret;
}

internal OS_Handle
os_lnx_handle_from_audio_stream(OS_LNX_AudioStream *stream)
{
  OS_Handle ret = {0};
  if(stream)
  {
    ret.u64[0] = (U64)stream;
  }
  return ret;
}

internal OS_LNX_AudioStream *
os_lnx_audio_stream_from_handle(OS_Handle handle)
{
  OS_LNX_AudioStream *ret = (OS_LNX_AudioStream*)handle.u64[0];
  return ret;
}

internal OS_Handle
os_lnx_handle_from_sound(OS_LNX_Sound *sound)
{
  OS_Handle ret = {0};
  if(sound)
  {
    ret.u64[0] = (U64)sound;
  }
  return ret;
}

internal OS_LNX_Sound *
os_lnx_sound_from_handle(OS_Handle handle)
{
  OS_LNX_Sound *ret = (OS_LNX_Sound*)handle.u64[0];
  return ret;
}
