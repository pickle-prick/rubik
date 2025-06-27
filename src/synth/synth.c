/////////////////////////////////////////////////////////////////////////////////////////
// State

internal void
sy_init(void)
{
  Arena *arena = arena_alloc();
  sy_state = push_array(arena, SY_State, 1);
  sy_state->arena = arena;

  OS_Handle main_stream = os_audio_stream_alloc(48000, sizeof(F32), 1);
  os_audio_stream_set_output_callback(main_stream, sy_audio_stream_output_callback);
  os_audio_stream_play(main_stream);
  os_audio_stream_set_volume(main_stream, 0.1);
  sy_state->main_stream = main_stream;
  // os_audio_stream_pause(speaker_stream);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Envelop

internal F32
sy_amp_from_envelope(SY_EnvelopeADSR *envelope, F64 time, F64 on_time, F64 off_time, B32 *out_finished)
{
#if 0
  F64 ret = time > off_time ? 0 : 1.0;
  *out_finished = time > off_time;
#else
  F64 ret = 0.0;
  F64 release_amp = 0.0;

  // unpack params
  F64 attack_time  = envelope->attack_time;
  F64 decay_time   = envelope->decay_time;
  F64 release_time = envelope->release_time;
  F32 start_amp    = envelope->start_amp;
  F32 sustain_amp = envelope->sustain_amp;

  B32 on = on_time <= time && (time < off_time || off_time < on_time);

  if(on)
  {
    F64 life_time = time-on_time;

    // attach stage
    if(life_time <= attack_time && life_time > 0.0)
      ret = (life_time/attack_time) * start_amp;

    // decay stage
    if(life_time > attack_time && life_time <= (attack_time+decay_time))
      ret = ((life_time-attack_time) / decay_time) * (sustain_amp-start_amp) + start_amp;

    // subtain stage
    if(life_time > (attack_time+decay_time))
      ret = sustain_amp;

    Assert(!isnan(ret));
  }
  else
  {
    F64 life_time = off_time - on_time;

    // still in attack stage
    if(life_time <= attack_time)
      release_amp = (life_time / attack_time) * start_amp;

    // still in decay stage
    if (life_time > attack_time && life_time <= (attack_time + decay_time))
      release_amp = ((life_time - attack_time) / decay_time) * (sustain_amp - start_amp) + start_amp;

    // in substain stage
    if (life_time > (attack_time + decay_time))
      release_amp = sustain_amp;

    if(release_time > 0)
      ret = ((time - off_time) / release_time) * (0.0 - release_amp) + release_amp;
    
    Assert(!isnan(ret));
  }

  // amp should not be negative
  if(ret <= 0.01)
  {
    ret = 0.0;
  }

  *out_finished = off_time >= on_time && time > (off_time+release_time);
#endif
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Oscillator

internal SY_Oscillator *
sy_oscillator_alloc()
{
  SY_Oscillator *ret = sy_state->first_free_oscillator;
  if(ret)
  {
    SLLStackPop(sy_state->first_free_oscillator);
  }
  else
  {
    ret = push_array_no_zero(sy_state->arena, SY_Oscillator, 1);
  }
  MemoryZeroStruct(ret);
  return ret;
}

internal void           
sy_oscillator_release(SY_Oscillator *oscillator)
{
  SLLStackPush(sy_state->first_free_oscillator, oscillator);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Instrument

internal SY_Instrument *
sy_instrument_alloc(String8 name)
{
  SY_Instrument *ret = sy_state->first_free_instrument;
  if(ret)
  {
    SLLStackPop(sy_state->first_free_instrument);
  }
  else
  {
    ret = push_array_no_zero(sy_state->arena, SY_Instrument, 1);
  }
  MemoryZeroStruct(ret);
  push_str8_copy_static(name, ret->name);
  return ret;
}

internal void
sy_instrument_release(SY_Instrument *instrument)
{
  // free osc nodes
  for(SY_InstrumentOSCNode *osc_node = instrument->first_osc;
      osc_node != 0;)
  {
    SY_InstrumentOSCNode *next = osc_node->next;
    SLLStackPush(sy_state->first_free_instrument_osc_node, osc_node);
    osc_node = next;
  }
  SLLStackPush(sy_state->first_free_instrument, instrument);
}

internal SY_InstrumentOSCNode *
sy_instrument_push_osc(SY_Instrument *instrument)
{
  SY_InstrumentOSCNode *ret = sy_state->first_free_instrument_osc_node;
  if(ret)
  {
    SLLStackPop(sy_state->first_free_instrument_osc_node);
  }
  else
  {
    ret = push_array_no_zero(sy_state->arena, SY_InstrumentOSCNode, 1);
  }
  MemoryZeroStruct(ret);
  DLLPushBack(instrument->first_osc, instrument->last_osc, ret);
  instrument->osc_node_count++;
  return ret;
}

internal SY_Note *
sy_instrument_play(SY_Instrument *instrument, F64 delay, F64 duration, U64 note_id, F32 volume)
{
  SY_Note *ret = sy_notelist_push(&sy_state->note_list);

  os_audio_thread_lock();
  ret->on_time = sy_wall_time+delay;
  ret->off_time = sy_wall_time+delay+duration;
  ret->active = 1;
  ret->id = note_id;
  ret->env = instrument->env;
  for(SY_InstrumentOSCNode *osc_node = instrument->first_osc;
      osc_node != 0; osc_node = osc_node->next)
  {
    SY_Oscillator *oscillator = sy_note_push_oscillator(ret);
    oscillator->base_hz = osc_node->base_hz;
    oscillator->amp = osc_node->amp*volume;
    oscillator->kind = osc_node->kind;
  }
  os_audio_thread_release();
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Note

internal SY_Note *
sy_note_alloc()
{
  SY_Note *ret = sy_state->first_free_note;
  if(ret)
  {
    SLLStackPop(sy_state->first_free_note);
  }
  else
  {
    ret = push_array_no_zero(sy_state->arena, SY_Note, 1);
  }
  MemoryZeroStruct(ret);
  return ret;
}

internal void
sy_note_release(SY_Note *note)
{
  for(SY_Oscillator *osc = note->first_oscillator; osc != 0;)
  {
    SY_Oscillator *next = osc->next;
    sy_oscillator_release(osc);
    osc = next;
  }
  SLLStackPush(sy_state->first_free_note, note);
}

internal SY_Note *
sy_notelist_push(SY_NoteList *list)
{
  SY_Note *ret = sy_note_alloc();
  DLLPushBack(list->first, list->last, ret);
  list->note_count++;
  return ret;
}

internal void
sy_notelist_remove(SY_NoteList *list, SY_Note *note)
{
  DLLRemove(list->first, list->last, note);
  list->note_count--;
  sy_note_release(note);
}

internal SY_Oscillator *
sy_note_push_oscillator(SY_Note *note)
{
  SY_Oscillator *ret = sy_oscillator_alloc();
  DLLPushBack(note->first_oscillator, note->last_oscillator, ret);
  note->oscillator_count++;
  return ret;
}

internal F32
sy_sample_from_note(SY_Note *note, F64 time)
{
  F32 ret = 0;
  for(SY_Oscillator *oscillator = note->first_oscillator;
      oscillator != 0;
      oscillator = oscillator->next)
  {
    F32 hz = sy_hz_from_note_id(oscillator->base_hz, note->id);
    F64 w = hz*tau32; // angular velocity (randians per second)
    F32 r = 0;
    switch(oscillator->kind)
    {
      case SY_OSC_Kind_Sine:
      {
        r = sin(w*time);
      }break;
      case SY_OSC_Kind_Square:
      {
        r = sin(w*time) > 0.0 ? 1.0 : -1.0;
      }break;
      case SY_OSC_Kind_Triangle:
      {
        r = asin(sin(w*time) * 2.0 / pi32);
      }break;
      case SY_OSC_Kind_Saw:
      {
        r = (2.0/pi32) * (hz * pi32 * fmod(time, 1.0/hz) - (pi32/2.0));
      }break;
      case SY_OSC_Kind_NoiseWhite:
      {
        r = 2.0 * rand_f32() - 1.0;
      }break;
      case SY_OSC_Kind_NoiseBrown:
      {
        F32 white = 2.0 * rand_f32() - 1.0;
        F32 last = oscillator->brown.last;
        last += white*0.2f;
        last = Clamp(-1.0, last, 1.0);
        r = last;
        oscillator->brown.last = last;

        // TODO: move bitcrusing somewhere else
        U64 bits = 2;
        F32 step = 2.0f / (1<<bits);
        r = roundf(r/step)*step;

        F32 low = oscillator->filter.low;
        F32 band = oscillator->filter.band;

        F32 cutoff_hz = 3500.0; // or 3000
        F32 sample_rate = 44100.0;

        F32 f = 2.0 * sinf(pi32 * (cutoff_hz/sample_rate));
        F32 d = 0.05;

        F32 high = r - low - d*band;
        band += f*high;
        low += f*band;

        oscillator->filter.low = low;
        oscillator->filter.band = band;

        r = band;

        AssertAlways(!isnan(r));
        AssertAlways(isfinite(r));
      }break;
      default:{InvalidPath;}break;
    }

    // TODO: filters here

    // TODO: not sure if we need to store it as post multiplied by amp
    oscillator->last_sample = r;
    ret+=r*oscillator->amp;
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Sequencer

internal SY_Sequencer *
sy_sequencer_alloc()
{
  SY_Sequencer *ret = sy_state->first_free_sequencer;
  if(ret)
  {
    SLLStackPop(sy_state->first_free_sequencer);
  }
  else
  {
    ret = push_array_no_zero(sy_state->arena, SY_Sequencer, 1);
  }
  MemoryZeroStruct(ret);
  return ret;
}

internal void
sy_sequencer_release(SY_Sequencer *sequencer)
{
  for(SY_Channel *channel = sequencer->first_channel; channel != 0;)
  {
    SY_Channel *next = channel->next;
    SLLStackPush(sy_state->first_free_channel, channel);
    channel = next;
  }
  SLLStackPush(sy_state->first_free_sequencer, sequencer);
}

internal void
sy_sequencer_play(SY_Sequencer *sequencer, B32 reset_if_repeated)
{
  if(sequencer)
  {
    os_audio_thread_lock();
    if(!sequencer->playing)
    {
      // reset sequencer progress
      sequencer->overdo_time = 0.0;
      sequencer->curr_subbeat_index = 0;

      sequencer->playing = 1;
      DLLPushBack(sy_state->first_sequencer_to_process,
                  sy_state->last_sequencer_to_process,
                  sequencer);
    }
    else if(reset_if_repeated)
    {
      // reset sequencer progress
      sequencer->overdo_time = 0.0;
      sequencer->curr_subbeat_index = 0;
    }
    os_audio_thread_release();
  }
}

internal void
sy_sequencer_pause(SY_Sequencer *sequencer)
{
  if(sequencer->playing)
  {
    os_audio_thread_lock();
    sequencer->playing = 0;
    DLLRemove(sy_state->first_sequencer_to_process,
              sy_state->last_sequencer_to_process,
              sequencer);
    os_audio_thread_release();
  }
}

internal void
sy_sequencer_resume(SY_Sequencer *sequencer)
{
  if(!sequencer->playing)
  {
    os_audio_thread_lock();
    sequencer->playing = 1;
    DLLPushBack(sy_state->first_sequencer_to_process,
                sy_state->last_sequencer_to_process,
                sequencer);
    os_audio_thread_release();
  }
}

internal void
sy_sequencer_set_volume(SY_Sequencer *sequencer, F32 volume)
{
  os_audio_thread_lock();
  sequencer->volume = volume;
  os_audio_thread_release();
}

internal void
sy_sequencer_set_dice(SY_Sequencer *sequencer, F32 dice)
{
  os_audio_thread_lock();
  sequencer->dice = dice;
  os_audio_thread_release();
}

internal void
sy_sequencer_set_looping(SY_Sequencer *sequencer, B32 looping)
{
  os_audio_thread_lock();
  sequencer->loop = looping;
  os_audio_thread_release();
}

internal SY_Channel *
sy_sequencer_push_channel(SY_Sequencer *sequencer)
{
  SY_Channel *ret = sy_state->first_free_channel;
  if(ret)
  {
    SLLStackPop(sy_state->first_free_channel);
  }
  else
  {
    ret = push_array_no_zero(sy_state->arena, SY_Channel, 1);
  }

  DLLPushBack(sequencer->first_channel, sequencer->last_channel, ret);
  sequencer->channel_count++;
  return ret;
}

internal void
sy_sequencer_advance(SY_Sequencer *seq, F64 advance_time, F64 wall_time)
{
  F64 local_time = seq->local_time;
  U64 curr_subbeat_index = seq->curr_subbeat_index;
  F64 start_time = wall_time+seq->overdo_time;
  F64 time_to_process = advance_time-seq->overdo_time;

  if(time_to_process > 0 && curr_subbeat_index == seq->total_subbeat_count)
  {
    if(seq->loop)
    {
      curr_subbeat_index = 0;
      local_time = 0;
    }
    else
    {
      seq->local_time = 0.0;
      seq->playing = 0;
      seq->curr_subbeat_index = 0;
      DLLRemove(sy_state->first_sequencer_to_process,
                sy_state->last_sequencer_to_process,
                seq);
      return;
    }
  }

  while(curr_subbeat_index < seq->total_subbeat_count && time_to_process > 0)
  {
    // process channels 
    for(SY_Channel *c = seq->first_channel; c != 0; c = c->next)
    {
      B32 hit = c->beats.str[curr_subbeat_index] == 'X';
      if(!hit && c->beats.str[curr_subbeat_index] == '?')
      {
        hit = rand_f32()<seq->dice;
      }
      if(hit)
      {
        SY_Note *n = sy_notelist_push(&sy_state->note_list);
        n->on_time = start_time;
        n->off_time = start_time+seq->subbeat_time;
        n->active = 1;
        n->id = 0;
        n->env = c->instrument->env;
        for(SY_InstrumentOSCNode *osc_node = c->instrument->first_osc;
            osc_node != 0; osc_node = osc_node->next)
        {
          SY_Oscillator *oscillator = sy_note_push_oscillator(n);
          oscillator->base_hz = osc_node->base_hz;
          oscillator->amp = osc_node->amp*seq->volume;
          oscillator->kind = osc_node->kind;
        }
      }
    }

    // increment
    time_to_process -= seq->subbeat_time;
    start_time += seq->subbeat_time;
    local_time += seq->subbeat_time;
    curr_subbeat_index++;
  }
  AssertAlways(time_to_process <= 0);
  seq->overdo_time = time_to_process < 0 ? -time_to_process : 0;
  seq->curr_subbeat_index = curr_subbeat_index;
  seq->local_time = local_time;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Audio Callback

internal void
sy_audio_stream_output_callback(void *buffer, U64 frame_count, U64 channel_count)
{
  // TODO: pass sample rate in here
  F32 time_per_frame = 1.0 / 44100.0f;
  F32 *dst = buffer;

  // advancing sequencers
  F32 advance_time = time_per_frame*frame_count;
  for(SY_Sequencer *seq = sy_state->first_sequencer_to_process;
      seq != 0;
      seq = seq->next)
  {
    sy_sequencer_advance(seq, advance_time, sy_wall_time);
  }

  for(U64 i = 0; i < frame_count; i++)
  {
    for(SY_Note *note = sy_state->note_list.first; note != 0; note = note->next)
    {
      if(note->active)
      {
        B32 finished = 0;
        F32 amp = sy_amp_from_envelope(&note->env, sy_wall_time, note->on_time, note->off_time, &finished);
        if(finished) note->active = 0;
        if(amp > 0.0)
        {
          F32 sample = sy_sample_from_note(note, sy_wall_time-note->on_time);
          for(U64 c = 0; c < channel_count; c++)
          {
            *(dst+c) += sample*amp;
          }
        }
      }
    }
    dst += channel_count;
    sy_wall_time += time_per_frame;
  }

  // remove dead notes
  for(SY_Note *note = sy_state->note_list.first; note != 0;)
  {
    SY_Note *next = note->next;
    if(!note->active)
    {
      sy_notelist_remove(&sy_state->note_list, note);
    }
    note = next;
  }
}
