#ifndef OS_AUDIO_LINUX_H
#define OS_AUDIO_LINUX_H

/////////////////////////////////////////////////////////////////////////////////////////
// Constants

#define OS_LNX_AUDIO_DEVICE_PLAYBACK_FORMAT   OS_AudioFormat_F32
#define OS_LNX_AUDIO_DEVICE_PLAYBACK_CHANNELS 2
#define OS_LNX_AUDIO_DEVICE_CAPTURE_FORMAT    OS_AudioFormat_S16
#define OS_LNX_AUDIO_DEVICE_CAPTURE_CHANNELS  1
#define OS_LNX_AUDIO_DEVICE_SAMPLE_RATE       44100

/////////////////////////////////////////////////////////////////////////////////////////
// Audio Backend Includes

// TODO
// #include "os/audio/linux/alsa/alsa.h"
// #include "os/audio/linux/jack/jack.h"
// #include "os/audio/linux/pipewire/pipewire.h"

// miniaudio
// init thread tctx (for use of scratch arena)
#define MA_ON_THREAD_ENTRY     \
  TCTX tctx_;                  \
  tctx_init_and_equip(&tctx_);
#define MA_ON_THREAD_EXIT      \
  tctx_release();
#include "external/miniaudio/miniaudio.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Enums

typedef enum OS_LNX_AudioBufferKind
{
  OS_LNX_AudioBufferKind_Static,
  OS_LNX_AudioBufferKind_Stream,
  OS_LNX_AudioBufferKind_COUNT,
} OS_LNX_AudioBufferKind;

/////////////////////////////////////////////////////////////////////////////////////////
// Types

typedef struct OS_LNX_AudioDevice OS_LNX_AudioDevice;
struct OS_LNX_AudioDevice
{
  OS_LNX_AudioDevice *next;

  // TODO: we want to use different audio backends (for example alsa, jack in linux)
  ma_device m_device;

  B32 started;
  F32 master_volume;
};

typedef struct OS_LNX_AudioBuffer OS_LNX_AudioBuffer;
struct OS_LNX_AudioBuffer
{
  OS_LNX_AudioBuffer *next;
  OS_LNX_AudioBuffer *prev;

  OS_LNX_AudioBufferKind kind;

  F32 volume;
  // F32 pitch;
  F32 pan;

  B32 playing;
  B32 paused;
  B32 looping;

  U64 frame_count;
  U64 frame_cursor_pos;
  U64 frames_processed;

  U8 *bytes; // data buffer, on music stream keeps filling
  U64 byte_count;
  U64 byte_cap;

  OS_AudioOutputCallback output_callback;
};

typedef struct OS_LNX_AudioStream OS_LNX_AudioStream;
struct OS_LNX_AudioStream
{
  OS_LNX_AudioStream *next;
  OS_LNX_AudioStream *prev;

  OS_LNX_AudioBuffer *buffer;

  U32 sample_rate;   // frequency (samples per second)
  U32 sample_size;   // bit depth (bits per sample): 8, 16, 32 (24 not supported)
  U32 channel_count; // number of channels (1-mono, 2-stereo, ...)
};

typedef struct OS_LNX_Sound OS_LNX_Sound;
struct OS_LNX_Sound
{
  OS_LNX_Sound *next;
  OS_LNX_AudioBuffer *buffer;
  U64 frame_count; // considering channels
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
  OS_LNX_Sound       *first_free_sound;
  // NOTE(k): we need use a DLL for this one, since we need match the buffer size when reusing
  OS_LNX_AudioBuffer *first_free_audio_buffer;
  OS_LNX_AudioBuffer *last_free_audio_buffer;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Globals

global OS_LNX_AudioState *os_lnx_audio_state = 0;

/////////////////////////////////////////////////////////////////////////////////////////
//~ Functions

/////////////////////////////////////////////////////////////////////////////////////////
// AudioBuffer

internal OS_LNX_AudioBuffer* os_lnx_audio_buffer_alloc(U64 byte_size);
internal void                os_lnx_audio_buffer_release(OS_LNX_AudioBuffer *audio_buffer);
internal void                os_lnx_audio_buffer_play(OS_LNX_AudioBuffer *buffer);
internal void                os_lnx_audio_buffer_pause(OS_LNX_AudioBuffer *buffer);
internal void                os_lnx_audio_buffer_resume(OS_LNX_AudioBuffer *buffer);
internal void                os_lnx_audio_buffer_set_output_callback(OS_LNX_AudioBuffer *buffer, OS_AudioOutputCallback cb);
internal void                os_lnx_audio_buffer_set_volume(OS_LNX_AudioBuffer *buffer, F32 volume);
internal void                os_lnx_audio_buffer_set_looping(OS_LNX_AudioBuffer *buffer, B32 looping);
internal void                os_lnx_audio_buffer_set_pan(OS_LNX_AudioBuffer *buffer, F32 pan);

/////////////////////////////////////////////////////////////////////////////////////////
// Sound

internal OS_LNX_Sound* os_lnx_sound_alloc(void);
internal void          os_lnx_sound_release(OS_LNX_Sound *sound);

/////////////////////////////////////////////////////////////////////////////////////////
// Handle

internal OS_Handle           os_lnx_handle_from_audio_device(OS_LNX_AudioDevice *device);
internal OS_LNX_AudioDevice* os_lnx_audio_device_from_handle(OS_Handle handle);
internal OS_Handle           os_lnx_handle_from_audio_stream(OS_LNX_AudioStream *stream);
internal OS_LNX_AudioStream* os_lnx_audio_stream_from_handle(OS_Handle handle);
internal OS_Handle           os_lnx_handle_from_sound(OS_LNX_Sound *sound);
internal OS_LNX_Sound*       os_lnx_sound_from_handle(OS_Handle handle);
#endif
