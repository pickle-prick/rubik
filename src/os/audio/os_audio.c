#define DR_WAV_IMPLEMENTATION
#include "external/dr_wav.h"

/////////////////////////////////////////////////////////////////////////////////////////
// wave

internal OS_AudioWave *
os_wave_from_bytes(Arena *arena, String8 audio_format, U8 *bytes, U64 byte_count)
{
  OS_AudioWave *ret = push_array(arena, OS_AudioWave, 1);

  if(str8_match(audio_format, str8_lit("wav"), 0))
  {
    drwav wav = {0};
    B32 success = drwav_init_memory(&wav, bytes, byte_count, 0);
    AssertAlways(success);

    // unpack results
    U64 frame_count = (U64)wav.totalPCMFrameCount;
    U64 sample_rate = (U64)wav.sampleRate;
    U64 sample_size = 16; // 16 bit sample (S16)
    U64 channel_count = wav.channels;
    U64 byte_count = frame_count*channel_count*sizeof(S16);
    S16 *data = (S16*)push_array(arena, U8, byte_count);
    // NOTE(k): we are forcing conversion to 16bit sample size on reading
    drwav_read_pcm_frames_s16(&wav, frame_count, data);

    ret->frame_count = frame_count;
    ret->sample_rate = sample_rate;
    ret->sample_size = sample_size;
    ret->bytes = (void*)data;
    ret->byte_count = byte_count;
    ret->channel_count = channel_count;

    drwav_uninit(&wav);
  }
  else
  {
    NotImplemented;
  }
  return ret;
}

internal OS_AudioWave *
os_wave_from_file(Arena *arena, char *filename)
{
  Temp scratch = scratch_begin(&arena, 1);
  OS_AudioWave *ret = push_array(arena, OS_AudioWave, 1);

  String8 file_ext = str8_skip_last_dot(str8_cstring(filename));

  U8 *file_bytes;
  U64 file_byte_count;
  FileReadAll(scratch.arena, str8_cstring(filename), &file_bytes, &file_byte_count);

  if(str8_match(file_ext, str8_lit("wav"), 0))
  {
    ret = os_wave_from_bytes(arena, str8_lit("wav"), file_bytes, file_byte_count);
  }
  else
  {
    NotImplemented;
  }
  scratch_end(scratch);
  return ret;
}
