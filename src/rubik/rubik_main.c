#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/////////////////////////////////////////////////////////////////////////////////////////
// build options

#define BUILD_TITLE          "Rubik"
#define OS_FEATURE_GRAPHICAL 1
#define OS_FEATURE_AUDIO     1

/////////////////////////////////////////////////////////////////////////////////////////
// external includes

#define CGLTF_IMPLEMENTATION
#include "external/cgltf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"
// #include "external/linmath.h"
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
#include "serialize/serialize_inc.h"
#include "synth/synth.h"
#include "physics/physics_inc.h"
#include "rubik_core.h"
#include "rubik_ui_widget.h"
#include "rubik_asset.h"
#include "scenes/rubik_scenes_inc.h"

// [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "render/render_inc.c"
#include "font_provider/font_provider_inc.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"
#include "ui/ui_inc.c"
#include "serialize/serialize_inc.c"
#include "synth/synth.c"
#include "physics/physics_inc.c"
#include "rubik_core.c"
#include "rubik_ui_widget.c"
#include "rubik_asset.c"
#include "scenes/rubik_scenes_inc.c"

internal void
entry_point(CmdLine *cmd_line)
{
  ///////////////////////////////////////////////////////////////////////////////////////
  // init

  srand((unsigned int)time(NULL));

  Vec2F32 window_rect = {900*3, 900*2};
  String8 window_title = str8_lit("Rubik");

  // open window
  OS_Handle os_wnd = os_window_open(r2f32p(0,0, window_rect.x, window_rect.y), 0, window_title);
  os_window_first_paint(os_wnd);

  // init main audio device
  OS_Handle main_audio_device = os_audio_device_open();
  os_set_main_audio_device(main_audio_device);
  os_audio_device_start(main_audio_device);
  os_audio_set_master_volume(1.0);

  // render initialization
  r_init((char *)window_title.str, os_wnd, BUILD_DEBUG);
  R_Handle r_wnd = r_window_equip(os_wnd);

  // init ui state
  UI_State *ui = ui_state_alloc();
  ui_select_state(ui);

  // init game state
  rk_init(os_wnd, r_wnd);

  // load scene template and load default scene
  rk_state->template_count = ArrayCount(rk_scene_templates);
  rk_state->templates = rk_scene_templates;
  // load scene function table
  for(U64 i = 0; i < ArrayCount(rk_scene_function_table); i++)
  {
    rk_register_function(rk_scene_function_table[i].name, rk_scene_function_table[i].fn);
  }
  RK_Scene *default_scene = rk_state->templates[1].fn();
  // RK_Scene *default_scene = rk_scene_from_tscn(str8_lit("./src/rubik/scenes/4/default.tscn"));
  rk_state->active_scene = default_scene;

  ///////////////////////////////////////////////////////////////////////////////////////
  // main loop

  B32 open = 1;
  while(open)
  {
    ProfTick();
    U64 global_tick = update_tick_idx();
    open = rk_frame();
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // cleanup

  os_window_close(os_wnd);
}
