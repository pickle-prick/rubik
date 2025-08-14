#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/////////////////////////////////////////////////////////////////////////////////////////
// build options

#define BUILD_TITLE          "Ink"
#define OS_FEATURE_GRAPHICAL 1
// TODO: required for linkage for now
#define OS_FEATURE_AUDIO     1

/////////////////////////////////////////////////////////////////////////////////////////
// external includes

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"
#include "external/xxHash/xxhash.c"
#include "external/xxHash/xxhash.h"
#define DR_WAV_IMPLEMENTATION
#include "external/dr_wav.h"

/////////////////////////////////////////////////////////////////////////////////////////
// single unit includes

// [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "render/render_inc.h"
#include "font_provider/font_provider_inc.h"
#include "font_cache/font_cache.h"
#include "draw/draw.h"
#include "ui/ui_inc.h"
#include "synth/synth.h"
#include "ink_core.h"

// [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "render/render_inc.c"
#include "font_provider/font_provider_inc.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"
#include "ui/ui_inc.c"
#include "synth/synth.c"
#include "ink_core.c"

internal void
entry_point(CmdLine *cmd_line)
{
  ///////////////////////////////////////////////////////////////////////////////////////
  // init

  // change working directory
  String8 binary_path = os_get_process_info()->binary_path; // only directory
  {
    Temp scratch = scratch_begin(0,0);
    String8List parts = str8_split_path(scratch.arena, binary_path);
    str8_list_push(scratch.arena, &parts, str8_lit(".."));
    str8_path_list_resolve_dots_in_place(&parts, PathStyle_SystemAbsolute);
    String8 working_directory = push_str8_copy(scratch.arena, str8_path_list_join_by_style(scratch.arena, &parts, PathStyle_SystemAbsolute));
    os_set_current_path(working_directory);
    scratch_end(scratch);
  }

  U32 seed = time(NULL);
  srand(seed);

  Vec2F32 window_rect = {900*3, 900*2};
  String8 window_title = str8_lit("Rubik");

  // open window
  OS_Handle os_wnd = os_window_open(r2f32p(0,0, window_rect.x, window_rect.y), 0, window_title);
  os_window_first_paint(os_wnd);

  // init main audio device
  OS_Handle main_audio_device = os_audio_device_open();
  os_set_main_audio_device(main_audio_device);
  os_audio_device_start(main_audio_device);
  os_audio_set_master_volume(0.9);

  // render initialization
  r_init((char *)window_title.str, os_wnd, BUILD_DEBUG);
  R_Handle r_wnd = r_window_equip(os_wnd);

  // init ui state
  UI_State *ui = ui_state_alloc();
  ui_select_state(ui);

  // init game state
  ik_init(os_wnd, r_wnd);

  ///////////////////////////////////////////////////////////////////////////////////////
  // main loop

  B32 open = 1;
  while(open)
  {
    ProfTick(0);
    U64 global_tick = update_tick_idx();
    open = ik_frame();
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // cleanup

  os_window_close(os_wnd);
}
