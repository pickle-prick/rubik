#ifndef INK_CORE_H
#define INK_CORE_H

/////////////////////////////////////////////////////////////////////////////////////////
//~ Enum

////////////////////////////////
//- Drag/Drop Types

typedef enum IK_DragDropState
{
  IK_DragDropState_Null,
  IK_DragDropState_Dragging,
  IK_DragDropState_Dropping,
  IK_DragDropState_COUNT
}
IK_DragDropState;

/////////////////////////////////////////////////////////////////////////////////////////
//~ Types

////////////////////////////////
//- Key

typedef struct IK_Key IK_Key;
struct IK_Key
{
  U64 u64[2];
};

////////////////////////////////
//- Setting Types

typedef struct IK_SettingVal IK_SettingVal;
struct IK_SettingVal
{
  B32 set;
  // TODO(k): we may want to support different number type here later
  S32 s32;
};

/////////////////////////////////
//- Generated code

#include "generated/ink.meta.h"

/////////////////////////////////
//- Theme Types 

typedef struct IK_Theme IK_Theme;
struct IK_Theme
{
  Vec4F32 colors[IK_ThemeColor_COUNT];
};

typedef enum IK_FontSlot
{
  IK_FontSlot_Main,
  IK_FontSlot_Code,
  IK_FontSlot_Icons,
  IK_FontSlot_Game,
  IK_FontSlot_COUNT
} IK_FontSlot;

typedef enum IK_PaletteCode
{
  IK_PaletteCode_Base,
  IK_PaletteCode_MenuBar,
  IK_PaletteCode_Floating,
  IK_PaletteCode_ImplicitButton,
  IK_PaletteCode_PlainButton,
  IK_PaletteCode_PositivePopButton,
  IK_PaletteCode_NegativePopButton,
  IK_PaletteCode_NeutralPopButton,
  IK_PaletteCode_ScrollBarButton,
  IK_PaletteCode_Tab,
  IK_PaletteCode_TabInactive,
  IK_PaletteCode_DropSiteOverlay,
  IK_PaletteCode_COUNT
} IK_PaletteCode;

/////////////////////////////////
//~ Dynamic drawing (in immediate mode fashion)

typedef struct IK_DrawNode IK_DrawNode;
struct IK_DrawNode
{
  IK_DrawNode *next;
  IK_DrawNode *draw_next;
  IK_DrawNode *draw_prev;

  R_GeoPolygonKind polygon;
  R_GeoTopologyKind topology;

  // vertex
  // src
  R_Vertex *vertices_src;
  U64 vertex_count;
  // dst
  R_Handle vertices;
  U64 vertices_buffer_offset;

  // indice
  // src
  U32 *indices_src;
  U64 indice_count;
  // dst
  R_Handle indices;
  U64 indices_buffer_offset;

  // for font altas
  R_Handle albedo_tex;
};

typedef struct IK_DrawList IK_DrawList;
struct IK_DrawList
{
  // per-build
  IK_DrawNode *first_node;
  IK_DrawNode *last_node;
  U64 node_count;
  U64 vertex_buffer_cmt;
  U64 indice_buffer_cmt;

  // persistent
  R_Handle vertices;
  R_Handle indices;
  U64 vertex_buffer_cap;
  U64 indice_buffer_cap;
};

////////////////////////////////
//- State 

typedef struct IK_State IK_State;
struct IK_State
{
  Arena                 *arena;
  Arena                 *frame_arenas[2];

  R_Handle              r_wnd;
  OS_Handle             os_wnd;
  OS_EventList          os_events;

  //- UI overlay signal (used for handle user input)
  UI_Signal             sig;

  // drawing buckets
  D_Bucket              *bucket_rect;

  // drawlists
  IK_DrawList           *drawlists[2];

  // frame history info
  U64                   frame_index;
  U64                   frame_time_us_history[64];
  F64                   time_in_seconds;
  U64                   time_in_us;

  // frame parameters
  F32                   frame_dt;
  F32                   cpu_time_us;
  F32                   pre_cpu_time_us;

  // window
  Rng2F32               window_rect;
  Vec2F32               window_dim;
  Rng2F32               last_window_rect;
  Vec2F32               last_window_dim;
  B32                   window_res_changed;
  F32                   dpi;
  F32                   last_dpi;
  B32                   window_should_close;

  // cursor
  Vec2F32               cursor;
  Vec2F32               last_cursor;
  Vec2F32               cursor_delta;
  B32                   cursor_hidden;

  // hot pixel key from renderer
  U64                   hot_pixel_key;

  // drag/drop state
  IK_DragDropState      drag_drop_state;
  U64                   drag_drop_slot;
  void                  *drag_drop_src;

  // theme
  IK_Theme              cfg_theme_target;
  IK_Theme              cfg_theme;
  F_Tag                 cfg_font_tags[IK_FontSlot_COUNT];

  // palette
  UI_Palette            cfg_ui_debug_palettes[IK_PaletteCode_COUNT]; // derivative from theme

  // global Settings
  IK_SettingVal         setting_vals[IK_SettingCode_COUNT];

  // views (UI)
  // IK_View               views[IK_ViewKind_COUNT];
  // B32                   views_enabled[IK_ViewKind_COUNT];

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

  IK_DeclStackNils;
  IK_DeclStacks;
};

/////////////////////////////////////////////////////////////////////////////////////////
//~ Globals

global IK_State *ik_state;

/////////////////////////////////////////////////////////////////////////////////////////
//~ Functions

/////////////////////////////////
//- Basic Type Functions

internal U64     ik_hash_from_string(U64 seed, String8 string);
internal String8 ik_hash_part_from_key_string(String8 string);
internal String8 ik_display_part_from_key_string(String8 string);

/////////////////////////////////
//- Key

internal IK_Key ik_key_from_string(IK_Key seed, String8 string);
internal IK_Key ik_key_from_stringf(IK_Key seed, char* fmt, ...);
internal B32    ik_key_match(IK_Key a, IK_Key b);
internal IK_Key ik_key_make(U64 a, U64 b);
internal IK_Key ik_key_zero();

/////////////////////////////////
//- Handle

// internal IK_Handle ik_handle_zero();
// internal B32       ik_handle_match(IK_Handle a, IK_Handle b);

/////////////////////////////////
//- State accessor/mutator

// init
internal void ik_init(OS_Handle os_wnd, R_Handle r_wnd);

// frame
internal B32          ik_frame(void);
internal Arena*       ik_frame_arena();
internal IK_DrawList* ik_frame_drawlist();

// color
internal Vec4F32 ik_rgba_from_theme_color(IK_ThemeColor color);

// code -> palette
internal UI_Palette *ik_palette_from_code(IK_PaletteCode code);

// fonts/size
internal F_Tag ik_font_from_slot(IK_FontSlot slot);
internal F32   ik_font_size_from_slot(IK_FontSlot slot);

/////////////////////////////////
//~ OS events

/////////////////////////////////
//- event consumption helpers

internal void ik_eat_event(OS_EventList *list, OS_Event *event);
internal B32  ik_key_press(OS_Modifiers mods, OS_Key key);
internal B32  ik_key_release(OS_Modifiers mods, OS_Key key);

/////////////////////////////////
//~ Dynamic drawing (in immediate mode fashion)

internal IK_DrawList* ik_drawlist_alloc(Arena *arena, U64 vertex_buffer_cap, U64 indice_buffer_cap);
internal IK_DrawNode* ik_drawlist_push(Arena *arena, IK_DrawList *drawlist, R_Vertex *vertices_src, U64 vertex_count, U32 *indices_src, U64 indice_count);
internal void         ik_drawlist_reset(IK_DrawList *drawlist);
internal void         ik_drawlist_build(IK_DrawList *drawlist); /* upload buffer from cpu to gpu */

/////////////////////////////////
//~ Helpers

#endif
