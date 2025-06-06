#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

////////////////////////////////
//~ k: Build Options

#define BUILD_TITLE "Rubik"
#define OS_FEATURE_GRAPHICAL 1

// Third Party
#define CGLTF_IMPLEMENTATION
#include "external/cgltf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"
// #include "external/linmath.h"
#include "external/xxHash/xxhash.c"
#include "external/xxHash/xxhash.h"

// [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "render/render_inc.h"
#include "font_provider/font_provider_inc.h"
#include "font_cache/font_cache.h"
#include "draw/draw.h"
#include "ui/ui_inc.h"
#include "serialize/serialize_inc.h"
#include "./physics/physics_inc.h"
#include "./rubik_core.h"
#include "./rubik_ui_widget.h"
#include "./rubik_asset.h"
#include "./scenes/rubik_scenes_inc.h"

// [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "render/render_inc.c"
#include "font_provider/font_provider_inc.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"
#include "ui/ui_inc.c"
#include "serialize/serialize_inc.c"
#include "./physics/physics_inc.c"
#include "./rubik_core.c"
#include "./rubik_ui_widget.c"
#include "./rubik_asset.c"
#include "./scenes/rubik_scenes_inc.c"

internal void
entry_point(CmdLine *cmd_line)
{
  /////////////////////////////////
  //~ Init

  srand((unsigned int)time(NULL));

  Vec2F32 default_resolution = {900, 900};
  String8 window_title = str8_lit("Rubik");

  //- Open window
  OS_Handle os_wnd = os_window_open(default_resolution, 0, window_title);
  os_window_first_paint(os_wnd);

  //- Render initialization
  r_init((char *)window_title.str, os_wnd, BUILD_DEBUG);
  R_Handle wnd = r_window_equip(os_wnd);

  Arena *frame_arena = arena_alloc();

  //- Time delta
  // TODO: using moving average to smooth out the value
  U64 frame_dt_us  = 0;
  U64 frame_us     = os_now_microseconds();
  U64 cpu_start_us = 0;
  U64 cpu_end_us   = 0;
  U64 cpu_dt_us    = 0;
  U64 gpu_start_us = 0;
  U64 gpu_end_us   = 0;
  U64 gpu_dt_us    = 0;

  //- User interaction
  OS_EventList events;

  //- Init ui state
  UI_State *ui = ui_state_alloc();
  ui_select_state(ui);

  //- Init game state
  rk_init(os_wnd);

  //- Load scene template and load default scene
  rk_state->template_count = ArrayCount(rk_scene_templates);
  rk_state->templates = rk_scene_templates;

  // Load scene function table
  for(U64 i = 0; i < ArrayCount(rk_scene_function_table); i++)
  {
    rk_register_function(rk_scene_function_table[i].name, rk_scene_function_table[i].fn);
  }

  RK_Scene *default_scene = rk_state->templates[1].fn();
  // RK_Scene *default_scene = rk_scene_from_tscn(str8_lit("./src/rubik/scenes/4/default.tscn"));
  rk_state->active_scene = default_scene;

  // Hot id
  U64 hot_id = 0;

  /////////////////////////////////
  // Main game loop

  while(rk_state->window_should_close == 0)
  {
    ProfTick();
    ProfBegin("frame");
    rk_state->debug.frame_dt_us = frame_dt_us;
    rk_state->debug.cpu_dt_us   = cpu_dt_us;
    rk_state->debug.gpu_dt_us   = gpu_dt_us;

    frame_dt_us = os_now_microseconds()-frame_us;
    frame_us = os_now_microseconds();

    //- k: begin of frame
    ProfBegin("frame begin");
    r_begin_frame();
    r_window_begin_frame(os_wnd, wnd);
    gpu_end_us = os_now_microseconds();
    gpu_dt_us = (gpu_end_us - gpu_start_us);
    cpu_start_us = os_now_microseconds();
    d_begin_frame();

    //- Poll events
    events = os_get_events(frame_arena, 0);

    ProfEnd();

    /////////////////////////////////
    //~ Game frame

    // TODO(k): calculation of frame_dt_us may be not accurate
    D_Bucket *d_bucket = rk_frame(events, frame_dt_us, hot_id);

    /////////////////////////////////
    //~ End of frame

    // TODO(XXX): the order here may be wrong, check later
    ProfBegin("sub bucket");
    d_submit_bucket(os_wnd, wnd, d_bucket);
    hot_id = r_window_end_frame(wnd, rk_state->cursor);
    r_end_frame();
    arena_clear(frame_arena);
    ProfEnd();

    gpu_start_us = os_now_microseconds();
    cpu_end_us = os_now_microseconds();
    cpu_dt_us = cpu_end_us - cpu_start_us;
    ProfEnd();
  }

  /////////////////////////////////
  //~ Cleanup

  os_window_close(os_wnd);
}
