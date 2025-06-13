/////////////////////////////////////////////////////////////////////////////////////////
// Audio Backend Includes

// #include "os/audio/linux/alsa/alsa.c"
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
os_lnx_audio_frames_mix(F32 *dst, F32 *src, U64 frame_count, F32 volume, F32 pan)
{
  U64 channel_count = os_lnx_audio_state->main_device->config.playback.channel_count;

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

  U64 channel_count = os_lnx_audio_state->main_device->config.playback.channel_count;
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

      // buffer output callback
      // TODO: read it from it's internal format
      buffer->output_callback((void*)src, frame_count);
      buffer->frames_processed += frame_count;
      // TODO: frame_cursor and looping (mainly for music)
      os_lnx_audio_frames_mix(dst, src, frame_count, buffer->volume, buffer->pan);
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
  // TODO: pass these params from the caller
  // NOTE(k): using f32 as format because it simplifies mixing
  OS_AudioFormat playback_format = OS_AudioFormat_F32;
  U64 playback_channel_count = 1;
  OS_AudioFormat capture_format = OS_AudioFormat_S16;
  U64 capture_channel_count = 1;
  U64 sample_rate = 48000; // use device sample rate 

  struct
  {
    OS_AudioFormat src;
    ma_format dst;
  } format_map[] =
  {
    {OS_AudioFormat_U8,  ma_format_u8},
    {OS_AudioFormat_S16, ma_format_s16},
    {OS_AudioFormat_S24, ma_format_s24},
    {OS_AudioFormat_S32, ma_format_s32},
    {OS_AudioFormat_F32, ma_format_f32},
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

  err = ma_device_init(&os_lnx_audio_state->context, &config, &device->handle);
  Assert(err == MA_SUCCESS);

  // fill info
  device->config.playback.format = playback_format;
  device->config.playback.channel_count = playback_channel_count;
  device->config.capture.format = capture_format;
  device->config.capture.channel_count = capture_channel_count;
  device->config.sample_rate = sample_rate;

  char *src = device->handle.playback.name;
  U64 copy_size = Min(ArrayCount(device->playback_name), strlen(src));
  MemoryCopy(device->playback_name, src, copy_size);
  src = device->handle.capture.name;
  copy_size = Min(ArrayCount(device->capture_name), strlen(src));

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
    ma_result err = ma_device_start(&device->handle);
    AssertAlways(err == MA_SUCCESS);
  }
}

internal void
os_audio_device_stop(OS_Handle handle)
{
  OS_LNX_AudioDevice *device = os_lnx_audio_device_from_handle(handle);
  ma_result err = ma_device_stop(&device->handle);
  Assert(err == MA_SUCCESS);
}

internal void
os_audio_device_set_master_volume(OS_Handle device, F32 volume)
{
  NotImplemented;
}

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
  stream->buffer.pan    = 0.5;
  stream->buffer.volume = 1.0;

  // push it into stack for processing/mixing
  DLLPushBack(os_lnx_audio_state->first_process_audio_buffer, os_lnx_audio_state->last_process_audio_buffer, &stream->buffer);
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
    os_lnx_audio_buffer_play(&stream->buffer);
  }
}

internal void
os_audio_stream_pause(OS_Handle handle)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    os_lnx_audio_buffer_pause(&stream->buffer);
  }
}

internal void
os_audio_stream_resume(OS_Handle handle)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    os_lnx_audio_buffer_resume(&stream->buffer);
  }
}

internal void
os_audio_stream_set_output_callback(OS_Handle handle, OS_AudioOutputCallback cb)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    os_lnx_audio_buffer_set_output_callback(&stream->buffer, cb);
  }
}

internal void
os_audio_stream_set_volume(OS_Handle handle, F32 volume)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    os_lnx_audio_buffer_set_volume(&stream->buffer, volume);
  }
}

internal void
os_audio_stream_set_pan(OS_Handle handle, F32 pan)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    os_lnx_audio_buffer_set_pan(&stream->buffer, pan);
  }
}

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
