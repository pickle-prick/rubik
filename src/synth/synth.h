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
  SY_OSC_Kind_Sine,
  SY_OSC_Kind_Square,
  SY_OSC_Kind_Triangle,
  SY_OSC_Kind_Saw,
  SY_OSC_Kind_NoiseWhite,
  SY_OSC_Kind_NoiseBrown,
  SY_OSC_Kind_COUNT,
} SY_OSC_Kind;

////////////////////////////////
// Filter

typedef enum SY_FilterKind
{
  SY_FilterKind_LowPass,
  SY_FilterKind_HighPass,
  SY_FilterKind_BandPass,
  SY_FilterKind_COUNT,
} SY_FilterKind;

typedef struct SY_Filter SY_Filter;
struct SY_Filter
{
  SY_FilterKind kind;
  union
  {
    struct
    {
      // F32 low;
    } low_pass;

    struct
    {
      // F32 high;
    } high_pass;

    struct
    {
      //F32 low;
      //F32 band;
      F32 cutoff_hz;
      F32 d;
    } band_pass;
  };
};

////////////////////////////////
// Oscillator

typedef struct SY_Oscillator SY_Oscillator;
struct SY_Oscillator
{
  SY_Oscillator *next;
  SY_Oscillator *prev;

  F64 base_hz;
  SY_OSC_Kind kind;
  F32 amp;

  F64 last_sample;
  struct
  {
    F64 last;
  } brown;

  // filter state
  struct
  {
    F32 low; 
    F32 band;
    F32 high; // not needed for band-pass output
  } filter;
};

////////////////////////////////
// Instrument

typedef struct SY_InstrumentOSCNode SY_InstrumentOSCNode;
struct SY_InstrumentOSCNode
{
  SY_InstrumentOSCNode *next;
  SY_InstrumentOSCNode *prev;
  F64 base_hz;
  SY_OSC_Kind kind;
  F32 amp;
  // SY_Filter filters[SY_FilterKind_COUNT];
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

  U64 id;
  F64 on_time;
  F64 off_time;
  B32 active;

  SY_EnvelopeADSR env;
  SY_Oscillator *first_oscillator;
  SY_Oscillator *last_oscillator;
  U64 oscillator_count;
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

  F32 tempo;      // beats per minute (BPM)
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
  F32 dice; // [0.0-1.0]
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
    SY_Oscillator *first_free_oscillator;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Globals

global SY_State *sy_state = 0;
global F64 sy_wall_time = 0;

/////////////////////////////////////////////////////////////////////////////////////////
// State

internal void sy_init(void);

/////////////////////////////////////////////////////////////////////////////////////////
// Envelop

internal F32 sy_amp_from_envelope(SY_EnvelopeADSR *envelope, F64 time, F64 on_time, F64 off_time, B32 *out_finished);

/////////////////////////////////////////////////////////////////////////////////////////
// Oscillator

internal SY_Oscillator* sy_oscillator_alloc();
internal void           sy_oscillator_release(SY_Oscillator *oscillator);
internal F64            sy_sample_from_oscillator(SY_Oscillator *oscillator, F64 time);

/////////////////////////////////////////////////////////////////////////////////////////
// Instrument

internal SY_Instrument*        sy_instrument_alloc(String8 name);
internal void                  sy_instrument_release(SY_Instrument *instrument);
internal SY_InstrumentOSCNode* sy_instrument_push_osc(SY_Instrument *instrument);
internal SY_Note*              sy_instrument_play(SY_Instrument *instrument, F64 delay, F64 duration, U64 note_id, F32 volume);

/////////////////////////////////////////////////////////////////////////////////////////
// Note

internal SY_Note*       sy_note_alloc();
internal void           sy_note_release(SY_Note *note);
internal SY_Note*       sy_notelist_push(SY_NoteList *list);
internal void           sy_notelist_remove(SY_NoteList *list, SY_Note *note);
internal SY_Oscillator* sy_note_push_oscillator(SY_Note *note);
internal F32            sy_sample_from_note(SY_Note *note, F64 local_time);

/////////////////////////////////////////////////////////////////////////////////////////
// Sequencer

internal SY_Sequencer* sy_sequencer_alloc();
internal void          sy_sequencer_release(SY_Sequencer *sequencer);
internal void          sy_sequencer_play(SY_Sequencer *sequencer, B32 reset_if_repeated);
internal void          sy_sequencer_pause(SY_Sequencer *sequencer);
internal void          sy_sequencer_resume(SY_Sequencer *sequencer);
internal void          sy_sequencer_set_volume(SY_Sequencer *sequencer, F32 volume);
internal void          sy_sequencer_set_dice(SY_Sequencer *sequencer, F32 dice);
internal void          sy_sequencer_set_looping(SY_Sequencer *sequencer, B32 looping);
internal SY_Channel*   sy_sequencer_push_channel(SY_Sequencer *sequencer);
internal void          sy_sequencer_advance(SY_Sequencer *sequencer, F64 advance_time, F64 wall_time);

/////////////////////////////////////////////////////////////////////////////////////////
// Audio Callback

internal void sy_audio_stream_output_callback(void *buffer, U64 frame_count, U64 channel_count);

/////////////////////////////////////////////////////////////////////////////////////////
// Helpers

#define sy_hz_from_note_id(base_hz,note_id) ((F32)base_hz*pow_f32(1.0594630943592952645618252949463,note_id))

#endif // SYNTH.H
