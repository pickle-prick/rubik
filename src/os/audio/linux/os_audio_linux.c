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

// this function will be called when miniaudio needs more data
// all the mixing takes place here
internal void
os_lnx_device_output_callback(ma_device *pDevice, void *pFramesOut, const void *pFramesInput, ma_uint32 frameCount)
{

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
  OS_AudioFormat playback_format = OS_AudioFormat_S16;
  U64 playback_channel_count = 2;
  OS_AudioFormat capture_format = OS_AudioFormat_S16;
  U64 capture_channel_count = 1;
  U64 sample_rate = 0; // use device sample rate 

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

  // char *src = device->handle.playback.name;
  // U64 copy_size = Min(ArrayCount(device->playback_name), strlen(src));
  // MemoryCopy(device->playback_name, src, copy_size);
  // src = device->handle.capture.name;
  // copy_size = Min(ArrayCount(device->capture_name), strlen(src));

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
    Assert(err == MA_SUCCESS);
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

// load audio stream (to stream audio pcm data)
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

  // TODO: alloc a audio buffer (don't know what's that)

  ma_format dst_format = ((sample_size == 8)? ma_format_u8 : ((sample_size == 16)? ma_format_s16 : ma_format_f32));

  // // the size of a streaming buffer must be at least double the size of period
  // U64 period_size = os_lnx_audio_state->main_device->handle.playback.internalPeriodSizeInFrames;

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
os_audio_stream_play(OS_Handle handle)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    os_lnx_audio_buffer_play(stream->buffer);
  }
}

internal void
os_audio_stream_callback_set(OS_Handle handle, OS_AudioOutputCallback cb)
{
  OS_LNX_AudioStream *stream = os_lnx_audio_stream_from_handle(handle);
  if(stream)
  {
    stream->buffer->output_callback = cb;
  }
}

internal OS_Handle
os_lnx_handle_from_audio_device(OS_LNX_AudioDevice *device)
{
  OS_Handle ret = {0};
  if(device)
  {
    ret.u64[0] = (U64)device;
    // ret.u64[1] = device->generation;
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
