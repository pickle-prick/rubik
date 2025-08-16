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

////////////////////////////////
//- Camera

typedef struct IK_Camera IK_Camera;
struct IK_Camera
{
  Rng2F32 rect;
  Rng2F32 target_rect;
  F32 zn;
  F32 zf;

  // TODO: store zoom level

  // drag state
  B32 dragging;
  Vec2F32 mouse_drag_start;
  Rng2F32 rect_drag_start;
};

////////////////////////////////
//- Box types

typedef U64 IK_BoxFlags;
# define IK_BoxFlag_DrawRect  (IK_BoxFlags)(1ull<<0)
# define IK_BoxFlag_DrawText  (IK_BoxFlags)(1ull<<1)
# define IK_BoxFlag_DrawImage (IK_BoxFlags)(1ull<<2)

typedef struct IK_Frame IK_Frame;
typedef struct IK_Box IK_Box;
struct IK_Box
{
  IK_Key key;
  IK_Frame *frame;

  // Lookup table links
  IK_Box *hash_next;
  IK_Box *hash_prev;

  // Tree links
  IK_Box *next;
  IK_Box *prev;
  IK_Box *first;
  IK_Box *last;
  IK_Box *parent;

  // Per-build equipment
  U64 children_count;
  IK_BoxFlags flags;
  Rng2F32 rect;
  Vec4F32 color;
  R_Handle albedo_tex;
  OS_Cursor hover_cursor;
};

typedef struct IK_BoxRec IK_BoxRec;
struct IK_BoxRec
{
  IK_Box *next;
  S32 push_count;
  S32 pop_count;
};

////////////////////////////////
//- Frame

typedef struct IK_BoxHashSlot IK_BoxHashSlot;
struct IK_BoxHashSlot
{
  IK_Box *hash_first;
  IK_Box *hash_last;
};

typedef struct IK_Frame IK_Frame;
struct IK_Frame
{
  Arena *arena;
  U64 arena_clear_pos;
  IK_Frame *next;
  IK_Box *root;
  IK_Camera camera;

  // lookup table
  U64 box_table_size;
  IK_BoxHashSlot *box_table;

  // free links
  IK_Box *first_free_box;
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

  IK_Frame              *active_frame;

  // drawing buckets
  D_Bucket              *bucket_rect;
  D_Bucket              *bucket_geo2d;

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

  // user interaction state
  IK_Key                hot_box_key;
  IK_Key                active_box_key;
  U64                   hot_pixel_key; // hot pixel key from renderer

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

  // free stacks
  IK_Frame *first_free_frame;

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
internal UI_Palette* ik_palette_from_code(IK_PaletteCode code);

// fonts/size
internal F_Tag ik_font_from_slot(IK_FontSlot slot);
internal F32   ik_font_size_from_slot(IK_FontSlot slot);

// box lookup
internal IK_Box* ik_box_from_key(IK_Key key);

/////////////////////////////////
//~ OS event consumption helpers

internal void ik_eat_event(OS_EventList *list, OS_Event *event);
internal B32  ik_key_press(OS_Modifiers mods, OS_Key key);
internal B32  ik_key_release(OS_Modifiers mods, OS_Key key);

/////////////////////////////////
//~ Dynamic drawing (in immediate mode fashion)

//- Basic building API
internal IK_DrawList* ik_drawlist_alloc(Arena *arena, U64 vertex_buffer_cap, U64 indice_buffer_cap);
internal IK_DrawNode* ik_drawlist_push(Arena *arena, IK_DrawList *drawlist, R_Vertex *vertices_src, U64 vertex_count, U32 *indices_src, U64 indice_count);
internal void         ik_drawlist_build(IK_DrawList *drawlist); /* upload buffer from cpu to gpu */
internal void         ik_drawlist_reset(IK_DrawList *drawlist);

//- High level building API
internal IK_DrawNode* ik_drawlist_push_rect(Arena *arena, IK_DrawList *drawlist, Rng2F32 dst, Rng2F32 src);

/////////////////////////////////
//~ Box Type Functions

internal IK_BoxRec ik_box_rec_df(IK_Box *box, IK_Box *root, U64 sib_member_off, U64 child_member_off);
#define ik_box_rec_df_pre(box, root) ik_box_rec_df(box, root, OffsetOf(IK_Box, next), OffsetOf(IK_Box, first))
#define ik_box_rec_df_post(box, root) ik_box_rec_df(box, root, OffsetOf(IK_Box, prev), OffsetOf(IK_Box, last))

/////////////////////////////////
//~ Frame Building API

internal IK_Frame* ik_frame_alloc();
internal void ik_frame_release(IK_Frame *frame);

/////////////////////////////////
//~ Box Tree Building API

//- box node construction
internal IK_Box* ik_build_box_from_key(UI_BoxFlags flags, IK_Key key);
internal IK_Key  ik_active_seed_key(void);
internal IK_Box* ik_build_box_from_string(IK_BoxFlags flags, String8 string);
internal IK_Box* ik_build_box_from_stringf(IK_BoxFlags flags, char *fmt, ...);

//- box node equipment
internal String8 ik_box_equip_display_string(IK_Box *box, String8 string);

/////////////////////////////////
//~ UI Widget

internal void ik_ui_stats(void);

/////////////////////////////////
//~ Helpers

////////////////////////////////
//~ Macro Loop Wrappers

#define IK_Parent(v) DeferLoop(ik_push_parent(v), ik_pop_parent())
#define IK_Frame(v) DeferLoop(ik_push_frame(v), ik_pop_frame())

#endif
