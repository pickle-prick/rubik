#ifndef OS_AUDIO_H
#define OS_AUDIO_H

typedef enum OS_AudioFormat
{
  OS_AudioFormat_U8,
  OS_AudioFormat_S16,
  OS_AudioFormat_S24,
  OS_AudioFormat_S32,
  OS_AudioFormat_F32,
  OS_AudioFormat_COUNT,
} OS_AudioFormat;

typedef void (*OS_AudioOutputCallback)(void *buffer, U64 frame_count);

/////////////////////////////////////////////////////////////////////////////////////////
// @os_hooks Main Initialization API (Implemented Per-OS)

// global state
internal void os_audio_init(void);
internal void os_set_main_audio_device(OS_Handle device);

// device
internal OS_Handle os_audio_device_open(void);
internal void      os_audio_device_close(OS_Handle handle);
internal void      os_audio_device_start(OS_Handle handle);
internal void      os_audio_device_stop(OS_Handle handle);
internal void      os_audio_device_set_master_volume(OS_Handle device, F32 volume);

// audio stream functions
internal OS_Handle os_audio_stream_alloc(U32 sample_rate, U32 sample_size, U32 channel_count);
internal void      os_audio_stream_play(OS_Handle handle);
internal void      os_audio_stream_pause(OS_Handle handle);
internal void      os_audio_stream_resume(OS_Handle handle);
internal void      os_audio_stream_set_output_callback(OS_Handle handle, OS_AudioOutputCallback cb);
internal void      os_audio_stream_set_volume(OS_Handle stream, F32 volume);
internal void      os_audio_stream_set_pan(OS_Handle stream, F32 pan);

// sound
internal OS_Handle os_sound_from_file(const char *filename);

// wave

#endif // OS_AUDIO_H
