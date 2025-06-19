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
#if 1
  F64 ret = time > off_time ? 0 : 1.0;
  *out_finished = time > off_time;
#else
  // TODO: this is wrong, fix it later
  F64 ret = 0.0;
  F64 release_amp = 0.0;

  // unpack params
  F64 attack_time  = envelope->attack_time;
  F64 decay_time   = envelope->decay_time;
  F64 release_time = envelope->release_time;
  F32 start_amp    = envelope->start_amp;
  F32 substain_amp = envelope->substain_amp;

  B32 on = on_time >= time && (time < off_time || off_time < on_time);

  if(on)
  {
    F64 life_time = time-on_time;

    if(life_time <= attack_time)
      ret = (life_time/attack_time) * start_amp;

    if(life_time > attack_time && life_time <= (attack_time+decay_time))
      ret = ((life_time-attack_time) / decay_time) * (substain_amp-start_amp) + start_amp;

    if(life_time > (attack_time+decay_time))
      ret = substain_amp;
  }
  else
  {
    F64 life_time = off_time - on_time;
    if(life_time <= attack_time)
      release_amp = (life_time / attack_time) * start_amp;

    if (life_time > attack_time && life_time <= (attack_time + decay_time))
      release_amp = ((life_time - attack_time) / decay_time) * (substain_amp - start_amp) + start_amp;

    if (life_time > (attack_time + decay_time))
      release_amp = substain_amp;

    ret = ((time - off_time) / release_time) * (0.0 - release_amp) + release_amp;
  }

  // amp should not be negative
  if(ret <= 0.01)
  {
    ret = 0.0;
  }

  *out_finished = ret == 0.0;
#endif
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OSC

internal F64
sy_osc(F64 hz, F64 time, SY_OSC_Kind kind)
{
  F64 ret;
  F64 w = hz*tau32; // angular velocity (radians per second)
  switch(kind)
  {
    case SY_OSC_Kind_Sin:
    {
      ret = sin(w*time);
    }break;
    case SY_OSC_Kind_Square:
    {
      ret = sin(w*time) > 0.0 ? 1.0 : -1.0;
    }break;
    case SY_OSC_Kind_Triangle:
    {
      ret = asin(sin(w*time) * 2.0 / pi32);
    }break;
    case SY_OSC_Kind_Saw:
    {
      ret = (2.0/pi32) * (hz * pi32 * fmod(time, 1.0/hz) - (pi32/2.0));
    }break;
    case SY_OSC_Kind_Random:
    {
      ret = 2.0 * ((F64)rand() / (F64)RAND_MAX) - 1.0;
    }break;
    default:{InvalidPath;}break;
  }
  return ret;
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

internal F32
sy_sound_from_instrument(SY_Instrument *instrument, F64 time)
{
  F32 ret = 0;
  for(SY_InstrumentOSCNode *osc_node = instrument->first_osc;
      osc_node != 0;
      osc_node = osc_node->next)
  {
    F32 value = sy_osc(osc_node->hz, time, osc_node->kind);
    ret += value;
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Note

internal SY_Note *
sy_notelist_push(SY_NoteList *list)
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
  DLLPushBack(list->first, list->last, ret);
  list->note_count++;
  return ret;
}

internal void
sy_notelist_remove(SY_NoteList *list, SY_Note *note)
{
  DLLRemove(list->first, list->last, note);
  list->note_count--;
  SLLStackPush(sy_state->first_free_note, note);
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
sy_sequencer_play(SY_Sequencer *sequencer, B32 reset_if_repeated)
{
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
}

internal void
sy_sequencer_pause(SY_Sequencer *sequencer)
{
  if(sequencer->playing)
  {
    sequencer->playing = 0;
    DLLRemove(sy_state->first_sequencer_to_process,
              sy_state->last_sequencer_to_process,
              sequencer);
  }
}

internal void
sy_sequencer_resume(SY_Sequencer *sequencer)
{
  if(!sequencer->playing)
  {
    sequencer->playing = 1;
    DLLPushBack(sy_state->first_sequencer_to_process,
                sy_state->last_sequencer_to_process,
                sequencer);
  }
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
      if(c->beats.str[curr_subbeat_index] == 'X')
      {
        SY_Note *n = sy_notelist_push(&sy_state->note_list);
        n->on_time = start_time;
        n->off_time = start_time+seq->subbeat_time;
        n->active = 1;
        n->src = c->instrument;
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
  // local_persist F64 time = 0;
  // F32 time_per_sample = 1 / 44100.0f;
  // F32 *dst = buffer;

  // for(U64 i = 0; i < frame_count; i++)
  // {
  //   for(U64 c = 0; c < channel_count; c++)
  //     F32 value = osc(440.0, time, OSC_Kind_Saw) +
  //                 osc(440.0 * 2.0, time, OSC_Kind_Square) +
  //                 osc(440.0 * 2.0, time, OSC_Kind_Sin);
  //     *dst++ = value;
  //   }
  //   time += time_per_sample;
  // }

  local_persist F64 time = 0;
  // TODO: pass sample rate in here
  F32 time_per_frame = 1.0 / 44100.0f;
  F32 *dst = buffer;

  // TODO: add os_audio_thread_lock/unlock

  // advancing sequencers
  F32 advance_time = time_per_frame*frame_count;
  for(SY_Sequencer *seq = sy_state->first_sequencer_to_process;
      seq != 0;
      seq = seq->next)
  {
    sy_sequencer_advance(seq, advance_time, time);
  }

  for(U64 i = 0; i < frame_count; i++)
  {
    for(SY_Note *note = sy_state->note_list.first; note != 0; note = note->next)
    {
      SY_Instrument *instrument = note->src;
      if(note->active)
      {
        B32 finished = 0;
        F32 amp = sy_amp_from_envelope(&instrument->env, time, note->on_time, note->off_time, &finished);
        if(finished) note->active = 0;
        if(amp > 0.0)
        {
          F32 value = sy_sound_from_instrument(instrument, time-note->on_time);
          for(U64 c = 0; c < channel_count; c++)
          {
            *(dst+c) += value*amp;
          }
        }
      }
    }
    dst += channel_count;
    time += time_per_frame;
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
