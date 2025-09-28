#ifndef RUBIK_CORE_H
#define RUBIK_CORE_H

////////////////////////////////
//~ Setting Types

typedef struct RK_SettingVal RK_SettingVal;
struct RK_SettingVal
{
  B32 set;
  S32 s32;
};

////////////////////////////////
//~ Key

typedef struct RK_Key RK_Key;
struct RK_Key
{
  U64 u64[2];
};

////////////////////////////////
//~ Frame Entry

#define RK_FRAME_ENTRY_UPDATE(name) B32 name(DR_Bucket **return_buckets, U64 *return_bucket_count)
typedef RK_FRAME_ENTRY_UPDATE(RK_FrameEntryUpdateFunctionType);

typedef struct RK_FrameEntry RK_FrameEntry;
struct RK_FrameEntry
{
  String8 name;
  RK_FrameEntryUpdateFunctionType *update;
};

/////////////////////////////////
//~ Generated code

#include "generated/rubik.meta.h"

/////////////////////////////////
//~ Theme Types 

typedef struct RK_Theme RK_Theme;
struct RK_Theme
{
  Vec4F32 colors[RK_ThemeColor_COUNT];
};

/////////////////////////////////
//~ State

typedef struct RK_State RK_State;
struct RK_State
{ 
  Arena *arena;
  Arena *frame_arena;

  R_Handle r_wnd;
  OS_Handle os_wnd;

  /////////////////////////////////
  //~ Equipment

  RK_FrameEntry *entry;

  /////////////////////////////////
  //~ Per-frame artifacts

  // frame history info
  U64 frame_index;
  U64 frame_time_us_history[64];
  F64 time_in_seconds;
  U64 time_in_us;

  // frame parameters
  F32 frame_dt;
  F32 cpu_time_us;
  F32 pre_cpu_time_us;

  // events
  OS_EventList os_events;

  // window
  Rng2F32 window_rect;
  Vec2F32 window_dim;
  Rng2F32 last_window_rect;
  Vec2F32 last_window_dim;
  B32 window_res_changed;
  F32 dpi;
  F32 last_dpi;
  B32 window_should_close;

  // mouse
  Vec2F32 mouse;
  Vec2F32 last_mouse;
  Vec2F32 mouse_delta;

  // pixel key
  U64 hot_pixel_key;

  // animation
  struct
  {
    F32 vast_rate;
    F32 fast_rate;
    F32 fish_rate;
    F32 slow_rate;
    F32 slug_rate;
    F32 slaf_rate;
  } animation;

  /////////////////////////////////
  //~ Initial State

  // global settings
  // RK_SettingVal setting_vals[RK_SettingCode_COUNT];
};

/////////////////////////////////
//~ Globals

global RK_State *rk_state;

/////////////////////////////////
//~ Basic Type Functions

internal U64     rk_hash_from_string(U64 seed, String8 string);
internal String8 rk_hash_part_from_key_string(String8 string);
internal String8 rk_display_part_from_key_string(String8 string);

/////////////////////////////////
//~ Key

internal RK_Key rk_key_from_string(RK_Key seed, String8 string);
internal RK_Key rk_key_from_stringf(RK_Key seed, char* fmt, ...);
internal B32    rk_key_match(RK_Key a, RK_Key b);
internal RK_Key rk_key_make(U64 a, U64 b);
internal RK_Key rk_key_zero();

/////////////////////////////////
//~ Entry Call Functions

internal void rk_init(OS_Handle os_wnd, R_Handle r_wnd);
internal B32  rk_frame(void);

/////////////////////////////////
//~ State accessor/mutator

internal Arena* rk_frame_arena();

#endif
