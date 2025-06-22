#ifndef SYNTH_H
#define SYNTH_H

/////////////////////////////////////////////////////////////////////////////////////////
//  Basic Type

typedef struct SY_EnvelopeADSR SY_EnvelopeADSR;
struct SY_EnvelopeADSR
{
  F64 attack_time;
  F64 decay_time;
  F64 release_time;
  F32 sustain_amp;
  F32 start_amp;
};

typedef enum SY_OSC_Kind
{
  SY_OSC_Kind_Sin,
  SY_OSC_Kind_Square,
  SY_OSC_Kind_Triangle,
  SY_OSC_Kind_Saw,
  SY_OSC_Kind_Random,
  SY_OSC_Kind_COUNT,
} SY_OSC_Kind;

////////////////////////////////
// Instrument

typedef struct SY_InstrumentOSCNode SY_InstrumentOSCNode;
struct SY_InstrumentOSCNode
{
  SY_InstrumentOSCNode *next;
  SY_InstrumentOSCNode *prev;
  F64 hz;
  SY_OSC_Kind kind;
};

typedef struct SY_Instrument SY_Instrument;
struct SY_Instrument
{
  SY_Instrument *next;
  U8 name[256];
  SY_EnvelopeADSR env;
  SY_InstrumentOSCNode *first_osc;
  SY_InstrumentOSCNode *last_osc;
  U64 osc_node_count;
  F32 volume; // 0-1
};

////////////////////////////////
// Note

typedef struct SY_Note SY_Note;
struct SY_Note
{
  SY_Note *next;
  SY_Note *prev;

  // TODO: tone or id?
  F64 on_time;
  F64 off_time;
  B32 active;
  SY_Instrument *src;
};

typedef struct SY_NoteList SY_NoteList;
struct SY_NoteList
{
  SY_Note *first;
  SY_Note *last;
  U64 note_count;
};

typedef struct SY_Channel SY_Channel;
struct SY_Channel
{
  SY_Channel *next;
  SY_Channel *prev;

  SY_Instrument *instrument;
  String8 beats;
};

typedef struct SY_Sequencer SY_Sequencer;
struct SY_Sequencer
{
  SY_Sequencer *next;
  SY_Sequencer *prev;

  F32 tempo; // beats per minute (BPM)
  U64 beat_count; //main beat
  U64 subbeat_count;
  U64 total_subbeat_count;
  F32 subbeat_time;
  F32 duration;

  // inc
  U64 curr_subbeat_index;
  F64 local_time;
  F64 overdo_time;

  SY_Channel *first_channel;
  SY_Channel *last_channel;
  U64 channel_count;
  B32 loop;
  F32 volume;
  B32 playing;
};

/////////////////////////////////////////////////////////////////////////////////////////
//  Main State Type

typedef struct SY_State SY_State;
struct SY_State
{
    Arena *arena;

    OS_Handle main_stream;

    // process list
    SY_NoteList note_list;
    SY_Sequencer *first_sequencer_to_process;
    SY_Sequencer *last_sequencer_to_process;

    // free list
    SY_Instrument *first_free_instrument;
    SY_InstrumentOSCNode *first_free_instrument_osc_node;
    SY_Sequencer *first_free_sequencer;
    SY_Channel *first_free_channel;
    SY_Note *first_free_note;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Globals

global SY_State *sy_state = 0;

/////////////////////////////////////////////////////////////////////////////////////////
// State

internal void sy_init(void);

/////////////////////////////////////////////////////////////////////////////////////////
// Envelop

internal F32 sy_amp_from_envelope(SY_EnvelopeADSR *envelope, F64 time, F64 on_time, F64 off_time, B32 *out_finished);

/////////////////////////////////////////////////////////////////////////////////////////
// OSC

internal F64 sy_osc(F64 hz, F64 time, SY_OSC_Kind osc_kind);

/////////////////////////////////////////////////////////////////////////////////////////
// Instrument

internal SY_Instrument*        sy_instrument_alloc(String8 name);
internal void                  sy_instrument_release(SY_Instrument *instrument);
internal SY_InstrumentOSCNode* sy_instrument_push_osc(SY_Instrument *instrument);
internal F32                   sy_sound_from_instrument(SY_Instrument *instrument, F64 time);
// TODO: instrument_play to be implemented

/////////////////////////////////////////////////////////////////////////////////////////
// Note

internal SY_Note* sy_notelist_push(SY_NoteList *list);
internal void     sy_notelist_remove(SY_NoteList *list, SY_Note *note);

/////////////////////////////////////////////////////////////////////////////////////////
// Sequencer

internal SY_Sequencer* sy_sequencer_alloc();
internal void          sy_sequencer_play(SY_Sequencer *sequencer, B32 reset_if_repeated);
internal void          sy_sequencer_pause(SY_Sequencer *sequencer);
internal void          sy_sequencer_resume(SY_Sequencer *sequencer);
internal SY_Channel*   sy_sequencer_push_channel(SY_Sequencer *sequencer);
internal void          sy_sequencer_advance(SY_Sequencer *sequencer, F64 advance_time, F64 wall_time);

/////////////////////////////////////////////////////////////////////////////////////////
// Audio Callback

internal void sy_audio_stream_output_callback(void *buffer, U64 frame_count, U64 channel_count);

#endif // SYNTH.H
