
int
playback_callback(snd_pcm_sframes_t nframes)
{
  int err;

  // TODO: get deivce here
  AlsaDevice *device;

  // fill buffer with data
  err = snd_pcm_writei(device->playback_handle, device->buffer, nframes);
  if(err < 0)
  {
    fprintf(stderr, "write failed '(%s)'\n", snd_strerror(err));
  }
  return err;
}

internal AlsaDevice *
aa_device_open_(String8 device_name, AlsaDeviceParams *params)
{
  Arena *arena =  os_lnx_audio_state->arena;
  AlsaDevice *ret = push_array(arena, AlsaDevice, 1);

  snd_pcm_t *playback_handle = 0;
  snd_pcm_t *capture_handle = 0;

  snd_pcm_hw_params_t *hw_params;
  // unpack params
  S32 sample_format = params->sample_format;
  U32 sample_rate = params->sample_rate;
  AlsaDeviceAccessMode access_mode = params->access_mode;
  AlsaDeviceMode mode = params->mode;

  U64 channel_count = 0;
  switch(mode)
  {
    case AlsaDeviceMode_Mono:
    {
      channel_count = 1;
    }break;
    case AlsaDeviceMode_Stereo:
    {
      channel_count = 2;
    }break;
    default:{InvalidPath;}break;
  }

  // alloc buffer
  U8 *buffer = 0;
  // TODO: what is frames?
  snd_pcm_uframes_t frame_count = 32;
  U64 buffer_size = frame_count*channel_count;
  buffer = push_array(arena, U8, buffer_size);

  // open device
  int err;
  err = snd_pcm_open(&playback_handle, (const char*)device_name.str, SND_PCM_STREAM_PLAYBACK, 0);
  if(err < 0)
  {
    fprintf(stderr, "cannot open audio device '%s' (%s)\n", device_name.str, snd_strerror(err));
    exit(1);
  }

  // alloc params
  err = snd_pcm_hw_params_malloc(&hw_params);
  if(err < 0)
  {
    fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror (err));
    exit (1);
  }

  // init params
  err = snd_pcm_hw_params_any(playback_handle, hw_params);
  if(err < 0)
  {
    fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
    exit (1);
  }

  // set access mode
  // TODO: based on access_mode above
  err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  if(err < 0)
  {
    fprintf (stderr, "cannot set access type (%s)\n", snd_strerror (err));
    exit (1);
  }

  // set format
  err = snd_pcm_hw_params_set_format(playback_handle, hw_params, sample_format);
  if(err < 0)
  {
    fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
    exit (1);
  }

  err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &sample_rate, 0);
  if(err < 0)
  {
    fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
    exit (1);
  }

  // set params
  err = snd_pcm_hw_params(playback_handle, hw_params);
  if(err < 0)
  {
    fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
    exit (1);
  }

  // free params
  snd_pcm_hw_params_free(hw_params);

  // TODO(XXX): not sure if this will lock this device out from other application

  // prepare audio device
  err = snd_pcm_prepare(playback_handle);
  if(err < 0)
  {
    fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
    exit (1);
  }

  // TESTING
  // {
  //   int total_samples = sample_rate * 0.5; // 0.5 secs
  //   for(U64 i = 0; i < total_samples; i+=frame_count)
  //   {
  //     for(U64 f = 0; f < frame_count; f++)
  //     {
  //       double t = (double)(i+f) / sample_rate;
  //       #define FREQUENCY 440.0
  //       buffer[f] = (int16_t)(sin(2 * 3.1415 * FREQUENCY * t) * (32767*0.1));  // 16-bit sine
  //     }

  //     err = snd_pcm_writei(playback_handle, buffer, frame_count);
  //     // if(err != 128)
  //     // {
  //     //   fprintf (stderr, "write to audio interface failed (%s)\n", snd_strerror (err));
  //     //   exit (1);
  //     // }
  //   }
  //   snd_pcm_drain(playback_handle);
  //   snd_pcm_close(playback_handle);
  // }

  // fill info
  ret->playback_handle = playback_handle;
  ret->capture_handle = capture_handle;
  ret->mode = mode;
  ret->access_mode = access_mode;
  // TODO: maybe we could remove mode, just use channel count?
  ret->sample_rate = sample_rate;
  ret->sample_format = sample_format;
  ret->buffer_size = buffer_size;
  ret->buffer = buffer;
  return ret;
}
