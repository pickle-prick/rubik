#ifndef OS_AUDIO_LINUX_H
#define OS_AUDIO_LINUX_H

/////////////////////////////////////////////////////////////////////////////////////////
// Audio Backend Includes

// #include "os/audio/linux/alsa/alsa.h"

#define MA_ON_THREAD_ENTRY     \
  TCTX tctx_;                  \
  tctx_init_and_equip(&tctx_);
#define MA_ON_THREAD_EXIT      \
  tctx_release();

#include "external/miniaudio/miniaudio.h"

typedef struct OS_LNX_AudioDeviceConfig OS_LNX_AudioDeviceConfig;
struct OS_LNX_AudioDeviceConfig
{
  struct
  {
    U32 format;
    U32 channel_count;
    // OS_AudioOutputCallback output_callback;
  } playback;

  struct
  {
    U32 format;
    U32 channel_count;
    // TODO: input callback
  } capture;

  U32 sample_rate;
};

typedef struct OS_LNX_AudioDevice OS_LNX_AudioDevice;
struct OS_LNX_AudioDevice
{
  OS_LNX_AudioDevice *next;

  OS_LNX_AudioDeviceConfig config;

  // TODO: we want to use different audio backends (for example alsa, jack in linux)
  ma_device handle;

  // NOTE: device can both act as playback and capture
  //       so there is two names
  char playback_name[256];
  char capture_name[256];

  B32 started;
};

typedef struct OS_LNX_AudioBuffer OS_LNX_AudioBuffer;
struct OS_LNX_AudioBuffer
{
  OS_LNX_AudioBuffer *next;
  OS_LNX_AudioBuffer *prev;

  F32 volume;
  // F32 pitch;
  F32 pan;

  B32 playing;
  B32 paused;
  B32 looping;

  U64 frame_count;
  U64 frame_cursor_pos;
  U64 frames_processed;

  // TODO: how do we reuse this, buffer size could vary for music stream
  //       maybe use a ring buffer?
  U8 *data; // data buffer, on music stream keeps filling

  OS_AudioOutputCallback output_callback;
};

typedef struct OS_LNX_AudioStream OS_LNX_AudioStream;
struct OS_LNX_AudioStream
{
  OS_LNX_AudioStream *next;
  OS_LNX_AudioStream *prev;

  OS_LNX_AudioBuffer buffer;

  U32 sample_rate;   // frequency (samples per second)
  U32 sample_size;   // bit depth (bits per sample): 8, 16, 32 (24 not supported)
  U32 channel_count; // number of channels (1-mono, 2-stereo, ...)
};

typedef struct OS_LNX_AudioState OS_LNX_AudioState;
struct OS_LNX_AudioState
{
  Arena *arena;

  ma_context context; // miniaudio context data
  ma_mutex lock;      // miniaudio mutex lock

  // main device
  OS_LNX_AudioDevice *main_device;

  // process list
  OS_LNX_AudioBuffer *first_process_audio_buffer;
  OS_LNX_AudioBuffer *last_process_audio_buffer;

  // free list
  OS_LNX_AudioDevice *first_free_audio_device;
  OS_LNX_AudioStream *first_free_audio_stream;
};

/////////////////////////////////////////////////////////////////////////////////////////

global OS_LNX_AudioState *os_lnx_audio_state = 0;

/////////////////////////////////////////////////////////////////////////////////////////
// AudioBuffer

internal void os_lnx_audio_buffer_play(OS_LNX_AudioBuffer *buffer);
internal void os_lnx_audio_buffer_pause(OS_LNX_AudioBuffer *buffer);
internal void os_lnx_audio_buffer_resume(OS_LNX_AudioBuffer *buffer);
internal void os_lnx_audio_buffer_set_output_callback(OS_LNX_AudioBuffer *buffer, OS_AudioOutputCallback cb);
internal void os_lnx_audio_buffer_set_volume(OS_LNX_AudioBuffer *buffer, F32 volume);
internal void os_lnx_audio_buffer_set_pan(OS_LNX_AudioBuffer *buffer, F32 pan);

/////////////////////////////////////////////////////////////////////////////////////////
// Handle

internal OS_Handle           os_lnx_handle_from_audio_device(OS_LNX_AudioDevice *device);
internal OS_LNX_AudioDevice* os_lnx_audio_device_from_handle(OS_Handle handle);
internal OS_Handle           os_lnx_handle_from_audio_stream(OS_LNX_AudioStream *stream);
internal OS_LNX_AudioStream* os_lnx_audio_stream_from_handle(OS_Handle handle);
#endif
