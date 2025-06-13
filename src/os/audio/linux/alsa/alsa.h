#ifndef OS_AUDIO_LINUX_ALSA_H
#define OS_AUDIO_LINUX_ALSA_H

#include <alsa/asoundlib.h>

typedef enum AlsaDeviceMode
{
  AlsaDeviceMode_Mono,
  AlsaDeviceMode_Stereo,
  AlsaDeviceMode_COUNT,
} AlsaDeviceMode;

typedef enum AlsaDeviceAccessMode
{
  AlsaDeviceAccessMode_R,
  AlsaDeviceAccessMode_W,
  AlsaDeviceAccessMode_RW,
  AlsaDeviceAccessMode_COUNT,
} AlsaDeviceAccessMode;

typedef struct AlsaDeviceParams AlsaDeviceParams;
struct AlsaDeviceParams
{
  AlsaDeviceMode mode;
  AlsaDeviceAccessMode access_mode;
  U64 sample_rate;
  S32 sample_format;
  B32 buffer_size;
};

typedef int (*alsa_playback_callback)(U64 frames_n);
typedef struct AlsaDevice AlsaDevice;
struct AlsaDevice
{
  // TODO: add a free list
  // AlsaDevice *next;
  // U64 generation;

  snd_pcm_t *playback_handle;
  snd_pcm_t *capture_handle;

  AlsaDeviceMode mode;
  AlsaDeviceAccessMode access_mode;
  U64 sample_rate;
  S32 sample_format;
  // buffer
  B32 buffer_size;
  U8 *buffer;

  alsa_playback_callback *pb_callback;

  // thread?
};

typedef struct AlsaAudioStream AlsaAudioStream;
struct AlsaAudioStream
{

};

internal AlsaDevice* aa_device_open_(String8 device_name, AlsaDeviceParams *params);
#define aa_device_open(device_name, ...) aa_device_open_(device_name, &(AlsaDeviceParams){.mode = AlsaDeviceMode_Mono, .access_mode = AlsaDeviceAccessMode_W, .sample_rate = 44100, .sample_format = SND_PCM_FORMAT_S16_LE, __VA_ARGS__})
// TODO(XXX): use os handle instead
internal void aa_device_write(AlsaDevice *device, U8 *src, U64 frame_count);
internal void aa_device_set_pb_callback(AlsaDevice *device, alsa_playback_callback *cb);

#endif
