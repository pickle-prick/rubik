#ifndef OS_AUDIO_H
#define OS_AUDIO_H

typedef enum OS_AudioFormat
{
  OS_AudioFormat_NULL,
  OS_AudioFormat_U8,
  OS_AudioFormat_S16,
  OS_AudioFormat_S24,
  OS_AudioFormat_S32,
  OS_AudioFormat_F32,
  OS_AudioFormat_COUNT,
} OS_AudioFormat;

typedef enum OS_AudioDeviceState
{
  OS_AUDIODEVICESTATE_UNINITIALIZED,
  OS_AudioDeviceState_Stopped,
  OS_AudioDeviceState_Started,
  OS_AudioDeviceState_Starting,
  OS_AudioDeviceState_Stopping,
  OS_AudioDeviceState_COUNT,
} OS_AudioDeviceState;


// TODO: revisit this
typedef void (*OS_AudioOutputCallback)(void *buffer, U64 frame_count, U64 channel_count);

typedef struct OS_AudioDeviceConfig OS_AudioDeviceConfig;
struct OS_AudioDeviceConfig
{
  OS_AudioDeviceState state;

  struct
  {
    String8 name;
    OS_AudioFormat format;
    U64 channel_count;
  } playback;

  struct
  {
    String8 name;
    OS_AudioFormat format;
    U64 channel_count;
  } capture;

  U32 sample_rate;
};

typedef struct OS_AudioWave OS_AudioWave;
struct OS_AudioWave
{
  U64 frame_count;
  U64 sample_rate;
  U64 sample_size;
  U64 channel_count;
  // NOTE: caller pass a arena for the life time of this object
  U8 *bytes;
  U64 byte_count;
};

/////////////////////////////////////////////////////////////////////////////////////////
// @os_hooks Main Initialization API (Implemented Per-OS)

// global state
internal void os_audio_init(void);
internal void os_set_main_audio_device(OS_Handle device);
internal void os_audio_set_master_volume(F32 volume);

// audio thread lock
internal void os_audio_thread_lock();
internal void os_audio_thread_release();

// device
internal OS_Handle            os_audio_device_open(void);
internal void                 os_audio_device_close(OS_Handle handle);
internal void                 os_audio_device_start(OS_Handle handle);
internal void                 os_audio_device_stop(OS_Handle handle);
internal OS_AudioDeviceConfig os_audio_deivce_cfg(OS_Handle device);

// audio stream functions
internal OS_Handle os_audio_stream_alloc(U32 sample_rate, U32 sample_size, U32 channel_count);
internal void      os_audio_stream_play(OS_Handle handle);
internal void      os_audio_stream_pause(OS_Handle handle);
internal void      os_audio_stream_resume(OS_Handle handle);
internal void      os_audio_stream_set_output_callback(OS_Handle handle, OS_AudioOutputCallback cb);
internal void      os_audio_stream_set_volume(OS_Handle stream, F32 volume);
internal void      os_audio_stream_set_pan(OS_Handle stream, F32 pan);

// sound
internal OS_Handle os_sound_from_file(char *filename);
internal OS_Handle os_sound_from_wave(OS_AudioWave *wave);
internal void      os_sound_set_volume(OS_Handle handle, F32 volume);
internal void      os_sound_set_looping(OS_Handle handle, B32 looping);
internal void      os_sound_play(OS_Handle handle);
internal void      os_sound_pause(OS_Handle handle);
internal void      os_sound_resume(OS_Handle handle);

/////////////////////////////////////////////////////////////////////////////////////////
// Helper Functions

// wave
internal OS_AudioWave* os_wave_from_bytes(Arena *arena, String8 audio_format, U8 *bytes, U64 byte_count);
internal OS_AudioWave* os_wave_from_file(Arena *arena, char *filename);

#endif // OS_AUDIO_H
