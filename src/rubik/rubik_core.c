/////////////////////////////////
//~ Basic Type Functions

internal U64
rk_hash_from_string(U64 seed, String8 string)
{
  U64 result = XXH3_64bits_withSeed(string.str, string.size, seed);
  return result;
}

internal String8
rk_hash_part_from_key_string(String8 string)
{
  String8 result = string;
  // k: look for ### patterns, which can replace the entirety of the part of
  // the string that is hashed.
  U64 hash_replace_signifier_pos = str8_find_needle(string, 0, str8_lit("###"), 0);
  if(hash_replace_signifier_pos < string.size)
  {
    result = str8_skip(string, hash_replace_signifier_pos);
  }
  return result;
}

internal String8
rk_display_part_from_key_string(String8 string)
{
  U64 hash_pos = str8_find_needle(string, 0, str8_lit("##"), 0);
  string.size = hash_pos;
  return string;
}

/////////////////////////////////
//~ Key

internal RK_Key rk_key_from_string(RK_Key seed, String8 string)
{
  RK_Key result = {0};
  if(string.size != 0)
  {
    String8 hash_part = rk_hash_part_from_key_string(string);
    result.u64[0] = rk_hash_from_string(seed.u64[0], hash_part);
  }
  return result;
}

internal RK_Key
rk_key_from_stringf(RK_Key seed, char* fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);

  RK_Key result = {0};
  result = rk_key_from_string(seed, string);
  scratch_end(scratch);
  return result;
}

internal B32
rk_key_match(RK_Key a, RK_Key b)
{
  return MemoryMatchStruct(&a,&b);
}

internal RK_Key
rk_key_make(U64 a, U64 b)
{
  RK_Key result = {a,b};
  return result;
}

internal RK_Key
rk_key_zero()
{
  return (RK_Key){0}; 
}

internal Vec2F32
rk_2f32_from_key(RK_Key key)
{
  U32 high = (key.u64[0]>>32);
  U32 low = key.u64[0];
  Vec2F32 ret = {*((F32*)&high), *((F32*)&low)};
  return ret;
}

internal RK_Key
rk_key_from_2f32(Vec2F32 key_2f32)
{
  U32 high = *((U32*)(&key_2f32.x));
  U32 low = *((U32*)(&key_2f32.y));
  U64 key = ((U64)high<<32) | ((U64)low);
  RK_Key ret = {key, 0};
  return ret;
}

/////////////////////////////////
//~ Entry Call Functions

internal void
rk_init(OS_Handle os_wnd, R_Handle r_wnd)
{ 
  Arena *arena = arena_alloc();
  rk_state = push_array(arena, RK_State, 1);

  rk_state->arena = arena;
  rk_state->frame_arena = arena_alloc();
  rk_state->os_wnd = os_wnd;
  rk_state->r_wnd = r_wnd;
  rk_state->last_dpi = rk_state->dpi = os_dpi_from_window(os_wnd);
  rk_state->last_window_rect = rk_state->window_rect = os_client_rect_from_window(os_wnd, 1);
  rk_state->last_window_dim = rk_state->window_dim = dim_2f32(rk_state->window_rect);
  rk_state->last_mouse = rk_state->mouse = os_mouse_from_window(os_wnd);
}

internal B32
rk_frame(void)
{
  ProfBeginFunction();

  arena_clear(rk_frame_arena());
  dr_begin_frame();

  /////////////////////////////////
  //~ Get events from os
  
  rk_state->os_events = os_get_events(rk_frame_arena(), 0);
  rk_state->last_window_rect = rk_state->window_rect;
  rk_state->last_window_dim = dim_2f32(rk_state->last_window_rect);
  rk_state->window_rect = os_client_rect_from_window(rk_state->os_wnd, 0);
  rk_state->window_res_changed = rk_state->window_rect.x0 != rk_state->last_window_rect.x0 || rk_state->window_rect.x1 != rk_state->last_window_rect.x1 || rk_state->window_rect.y0 != rk_state->last_window_rect.y0 || rk_state->window_rect.y1 != rk_state->last_window_rect.y1;
  rk_state->window_dim = dim_2f32(rk_state->window_rect);
  rk_state->last_mouse = rk_state->mouse;
  rk_state->mouse = os_window_is_focused(rk_state->os_wnd) ? os_mouse_from_window(rk_state->os_wnd) : v2f32(-100,-100);
  rk_state->mouse_delta = sub_2f32(rk_state->mouse, rk_state->last_mouse);
  rk_state->last_dpi = rk_state->dpi;
  rk_state->dpi = os_dpi_from_window(rk_state->os_wnd);

  /////////////////////////////////
  //~ Calculate avg length in us of last many frames

  U64 frame_time_history_avg_us = 0;
  ProfScope("calculate avg length in us of last many frames")
  {
    U64 num_frames_in_history = Min(ArrayCount(rk_state->frame_time_us_history), rk_state->frame_index);
    U64 frame_time_history_sum_us = 0;
    if(num_frames_in_history > 0)
    {
      for(U64 i = 0; i < num_frames_in_history; i++)
      {
        frame_time_history_sum_us += rk_state->frame_time_us_history[i];
      }
      frame_time_history_avg_us = frame_time_history_sum_us/num_frames_in_history;
    }
  }

  /////////////////////////////////
  //~ Pick target hz

  // pick among a number of sensible targets to snap to, given how well we've been performing
  F32 target_hz = !os_window_is_focused(rk_state->os_wnd) ? 10.0f : os_get_gfx_info()->default_refresh_rate;
  if(rk_state->frame_index > 32)
  {
    F32 possible_alternate_hz_targets[] = {target_hz, 60.f, 120.f, 144.f, 240.f};
    F32 best_target_hz = target_hz;
    S64 best_target_hz_frame_time_us_diff = max_S64;
    for(U64 idx = 0; idx < ArrayCount(possible_alternate_hz_targets); idx += 1)
    {
      F32 candidate = possible_alternate_hz_targets[idx];
      if(candidate <= target_hz)
      {
        U64 candidate_frame_time_us = 1000000/(U64)candidate;
        S64 frame_time_us_diff = (S64)frame_time_history_avg_us - (S64)candidate_frame_time_us;
        if(abs_s64(frame_time_us_diff) < best_target_hz_frame_time_us_diff &&
           frame_time_history_avg_us < candidate_frame_time_us + candidate_frame_time_us/4)
        {
          best_target_hz = candidate;
          best_target_hz_frame_time_us_diff = frame_time_us_diff;
        }
      }
    }
    target_hz = best_target_hz;
  }

  rk_state->frame_dt = 1.f/target_hz;

  /////////////////////////////////
  //~ Fill animation rates

  rk_state->animation.vast_rate = 1 - pow_f32(2, (-60.f * rk_state->frame_dt));
  rk_state->animation.fast_rate = 1 - pow_f32(2, (-50.f * rk_state->frame_dt));
  rk_state->animation.fish_rate = 1 - pow_f32(2, (-40.f * rk_state->frame_dt));
  rk_state->animation.slow_rate = 1 - pow_f32(2, (-30.f * rk_state->frame_dt));
  rk_state->animation.slug_rate = 1 - pow_f32(2, (-15.f * rk_state->frame_dt));
  rk_state->animation.slaf_rate = 1 - pow_f32(2, (-8.f  * rk_state->frame_dt));

  // begin measuring actual per-frame work
  U64 begin_time_us = os_now_microseconds();

  // Call frame entry
  RK_FrameEntry *entry = rk_state->entry;
  DR_Bucket **buckets = 0;
  U64 bucket_count = 0;
  if(entry)
  {
    rk_state->window_should_close = entry->update(&buckets, &bucket_count);
  }

  /////////////////////////////////
  //~ Submit work

  rk_state->pre_cpu_time_us = os_now_microseconds()-begin_time_us;

  ProfScope("submit")
  if(os_window_is_focused(rk_state->os_wnd) && bucket_count > 0)
  {
    r_begin_frame();
    r_window_begin_frame(rk_state->os_wnd, rk_state->r_wnd);
    for(U64 i = 0; i < bucket_count; i++)
    {
      dr_submit_bucket(rk_state->os_wnd, rk_state->r_wnd, buckets[i]);
    }
    rk_state->pixel_hot_key = rk_key_from_2f32(r_window_end_frame(rk_state->os_wnd, rk_state->r_wnd, rk_state->mouse));
    r_end_frame();
  }

  /////////////////////////////////
  //~ Wait if we still have some cpu time left

  U64 frame_time_target_cap_us = (U64)(1000000/target_hz);
  U64 woik_us = os_now_microseconds()-begin_time_us;
  rk_state->cpu_time_us = woik_us;
  if(woik_us < frame_time_target_cap_us)
  {
    ProfScope("wait frame target cap")
    {
      while(woik_us < frame_time_target_cap_us)
      {
        // TODO: check if os supports ms granular sleep
        if(1)
        {
          os_sleep_milliseconds((frame_time_target_cap_us-woik_us)/1000);
        }
        woik_us = os_now_microseconds()-begin_time_us;
      }
    }
  }
  else
  {
    // Missed frame rate!
    // TODO(k): proper logging
    fprintf(stderr, "missed frame, over %06.2f ms from %06.2f ms\n", (woik_us-frame_time_target_cap_us)/1000.0, frame_time_target_cap_us/1000.0);

    // tag it
    ProfBegin("MISSED FRAME");
    ProfEnd();
  }

  /////////////////////////////////
  //~ Determine frame time, record it into history

  U64 end_time_us = os_now_microseconds();
  U64 frame_time_us = end_time_us-begin_time_us;
  rk_state->frame_time_us_history[rk_state->frame_index%ArrayCount(rk_state->frame_time_us_history)] = frame_time_us;

  /////////////////////////////////
  //~ Bump frame time counters

  rk_state->frame_index++;
  rk_state->time_in_seconds += rk_state->frame_dt;
  rk_state->time_in_us += frame_time_us;

  ProfEnd();
  return !rk_state->window_should_close;
}

/////////////////////////////////
//~ State accessor/mutator

internal Arena *
rk_frame_arena()
{
  return rk_state->frame_arena;
}
