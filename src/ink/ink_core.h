#ifndef INK_CORE_H
#define INK_CORE_H

////////////////////////////////
//~ Mouse Button Kinds

typedef enum IK_MouseButtonKind
{
  IK_MouseButtonKind_Left,
  IK_MouseButtonKind_Middle,
  IK_MouseButtonKind_Right,
  IK_MouseButtonKind_COUNT
} IK_MouseButtonKind;

////////////////////////////////
//~ Tool Types

typedef enum IK_ToolKind
{
  IK_ToolKind_Hand,
  IK_ToolKind_Selection,
  IK_ToolKind_Rectangle,
  IK_ToolKind_Draw,
  IK_ToolKind_InsertImage,
  IK_ToolKind_Eraser,
  IK_ToolKind_COUNT,
} IK_ToolKind;

////////////////////////////////
//~ Drag/Drop Types

typedef enum IK_DragDropState
{
  IK_DragDropState_Null,
  IK_DragDropState_Dragging,
  IK_DragDropState_Dropping,
  IK_DragDropState_COUNT
}
IK_DragDropState;

////////////////////////////////
//~ Key

typedef struct IK_Key IK_Key;
struct IK_Key
{
  U64 u64[2];
};

////////////////////////////////
//~ Setting Types

typedef struct IK_SettingVal IK_SettingVal;
struct IK_SettingVal
{
  B32 set;
  // TODO(k): we may want to support different number type here later
  S32 s32;
};

////////////////////////////////
//~ Camera

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
  Vec2F32 drag_start_mouse;
  Rng2F32 drag_start_rect;
};

////////////////////////////////
//~ Box types

typedef U64 IK_BoxFlags;
# define IK_BoxFlag_MouseClickable (IK_BoxFlags)(1ull<<0)
# define IK_BoxFlag_ClickToFocus   (IK_BoxFlags)(1ull<<1)
# define IK_BoxFlag_Scroll         (IK_BoxFlags)(1ull<<2)
# define IK_BoxFlag_FixedRatio     (IK_BoxFlags)(1ull<<3)
# define IK_BoxFlag_DrawRect       (IK_BoxFlags)(1ull<<4)
# define IK_BoxFlag_DrawText       (IK_BoxFlags)(1ull<<5)
# define IK_BoxFlag_DrawImage      (IK_BoxFlags)(1ull<<6)
# define IK_BoxFlag_Disabled       (IK_BoxFlags)(1ull<<7)

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
  Vec2F32 position; // top left
  F32 rotation; // around center, turns
  F32 scale;
  Vec2F32 rect_size;
  Vec4F32 color;
  R_Handle albedo_tex;
  OS_Cursor hover_cursor;
  F32 ratio; // width/height

  // Per-frmae artifacts
  Rng2F32 fixed_rect;
  // F32 fixed_width;
  // F32 fixed_height;
  Mat4x4F32 fixed_xform;

  // Animation
  F32 hot_t;
  F32 active_t;
  F32 disabled_t;
  F32 focus_hot_t;
  F32 focus_active_t;
};

typedef struct IK_BoxRec IK_BoxRec;
struct IK_BoxRec
{
  IK_Box *next;
  S32 push_count;
  S32 pop_count;
};

////////////////////////////////
//~ Signal types

typedef U32 IK_SignalFlags;
enum
{
  // mouse press -> box was pressed while hovering
  IK_SignalFlag_LeftPressed         = (1<<0),
  IK_SignalFlag_MiddlePressed       = (1<<1),
  IK_SignalFlag_RightPressed        = (1<<2),

  // dragging -> box was previously pressed, user is still holding button
  IK_SignalFlag_LeftDragging        = (1<<3),
  IK_SignalFlag_MiddleDragging      = (1<<4),
  IK_SignalFlag_RightDragging       = (1<<5),

  // double-dragging -> box was previously double-clicked, user is still holding button
  IK_SignalFlag_LeftDoubleDragging  = (1<<6),
  IK_SignalFlag_MiddleDoubleDragging= (1<<7),
  IK_SignalFlag_RightDoubleDragging = (1<<8),

  // triple-dragging -> box was previously triple-clicked, user is still holding button
  IK_SignalFlag_LeftTripleDragging  = (1<<9),
  IK_SignalFlag_MiddleTripleDragging= (1<<10),
  IK_SignalFlag_RightTripleDragging = (1<<11),

  // released -> box was previously pressed & user released, in or out of bounds
  IK_SignalFlag_LeftReleased        = (1<<12),
  IK_SignalFlag_MiddleReleased      = (1<<13),
  IK_SignalFlag_RightReleased       = (1<<14),

  // clicked -> box was previously pressed & user released, in bounds
  IK_SignalFlag_LeftClicked         = (1<<15),
  IK_SignalFlag_MiddleClicked       = (1<<16),
  IK_SignalFlag_RightClicked        = (1<<17),

  // double clicked -> box was previously clicked, pressed again
  IK_SignalFlag_LeftDoubleClicked   = (1<<18),
  IK_SignalFlag_MiddleDoubleClicked = (1<<19),
  IK_SignalFlag_RightDoubleClicked  = (1<<20),

  // triple clicked -> box was previously clicked twice, pressed again
  IK_SignalFlag_LeftTripleClicked   = (1<<21),
  IK_SignalFlag_MiddleTripleClicked = (1<<22),
  IK_SignalFlag_RightTripleClicked  = (1<<23),

  // keyboard pressed -> box had focus, user activated via their keyboard
  IK_SignalFlag_KeyboardPressed     = (1<<24),

  // passive mouse info
  IK_SignalFlag_Hovering            = (1<<25), // hovering specifically this box
  IK_SignalFlag_MouseOver           = (1<<26), // mouse is over, but may be occluded

  // committing state changes via user interaction
  IK_SignalFlag_Commit              = (1<<27),

  // high-level combos
  IK_SignalFlag_Pressed       = IK_SignalFlag_LeftPressed|IK_SignalFlag_KeyboardPressed,
  IK_SignalFlag_Released      = IK_SignalFlag_LeftReleased,
  IK_SignalFlag_Clicked       = IK_SignalFlag_LeftClicked|IK_SignalFlag_KeyboardPressed,
  IK_SignalFlag_DoubleClicked = IK_SignalFlag_LeftDoubleClicked,
  IK_SignalFlag_TripleClicked = IK_SignalFlag_LeftTripleClicked,
  IK_SignalFlag_Dragging      = IK_SignalFlag_LeftDragging,
};

typedef struct IK_Signal IK_Signal;
struct IK_Signal
{
  IK_Box *box;
  OS_Modifiers event_flags;
  Vec2S16 scroll;
  IK_SignalFlags f;
};

#define ik_pressed(s)        !!((s).f&IK_SignalFlag_Pressed)
#define ik_clicked(s)        !!((s).f&IK_SignalFlag_Clicked)
#define ik_released(s)       !!((s).f&IK_SignalFlag_Released)
#define ik_double_clicked(s) !!((s).f&IK_SignalFlag_DoubleClicked)
#define ik_triple_clicked(s) !!((s).f&IK_SignalFlag_TripleClicked)
#define ik_middle_clicked(s) !!((s).f&IK_SignalFlag_MiddleClicked)
#define ik_right_clicked(s)  !!((s).f&IK_SignalFlag_RightClicked)
#define ik_dragging(s)       !!((s).f&IK_SignalFlag_Dragging)
#define ik_hovering(s)       !!((s).f&IK_SignalFlag_Hovering)
#define ik_mouse_over(s)     !!((s).f&IK_SignalFlag_MouseOver)
#define ik_committed(s)      !!((s).f&IK_SignalFlag_Commit)

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
  IK_Box *root_overlay;
  IK_Camera camera;

  // lookup table
  U64 box_table_size;
  IK_BoxHashSlot *box_table;

  // free links
  IK_Box *first_free_box;
};

/////////////////////////////////
//~ Generated code

#include "generated/ink.meta.h"

/////////////////////////////////
//~ Theme Types 

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
  IK_FontSlot_ToolbarIcons,
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
//~ State 

typedef struct IK_State IK_State;
struct IK_State
{
  Arena                 *arena;
  Arena                 *frame_arenas[2];

  R_Handle              r_wnd;
  OS_Handle             os_wnd;
  OS_EventList          os_events;

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

  // camera
  Mat4x4F32             proj_mat;
  Mat4x4F32             proj_mat_inv;
  Vec2F32               world_to_screen_ratio;

  // mouse
  Vec2F32               mouse;
  Vec2F32               mouse_in_world;
  Vec2F32               last_mouse;
  Vec2F32               mouse_delta; // frame delta
  B32                   cursor_hidden;

  // user interaction state
  IK_Key                hot_box_key;
  IK_Key                active_box_key[IK_MouseButtonKind_COUNT];
  IK_Key                focus_hot_box_key;
  IK_Key                focus_active_box_key;
  U64                   hot_pixel_key; // hot pixel key from renderer
  U64                   press_timestamp_history_us[IK_MouseButtonKind_COUNT][3];
  IK_Key                press_key_history[IK_MouseButtonKind_COUNT][3];
  Vec2F32               drag_start_mouse;
  Arena                 *drag_state_arena;
  String8               drag_state_data;

  // toolbar
  IK_ToolKind           tool;

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

/////////////////////////////////
//~ Globals

global IK_State *ik_state;

/////////////////////////////////
//~ Basic Type Functions

internal U64     ik_hash_from_string(U64 seed, String8 string);
internal String8 ik_hash_part_from_key_string(String8 string);
internal String8 ik_display_part_from_key_string(String8 string);

/////////////////////////////////
//~ Key

internal IK_Key ik_key_from_string(IK_Key seed, String8 string);
internal IK_Key ik_key_from_stringf(IK_Key seed, char* fmt, ...);
internal B32    ik_key_match(IK_Key a, IK_Key b);
internal IK_Key ik_key_make(U64 a, U64 b);
internal IK_Key ik_key_zero();

/////////////////////////////////
//~ Handle

// internal IK_Handle ik_handle_zero();
// internal B32       ik_handle_match(IK_Handle a, IK_Handle b);

/////////////////////////////////
//~ State accessor/mutator

//- init
internal void ik_init(OS_Handle os_wnd, R_Handle r_wnd);

//- frame
internal B32          ik_frame(void);
internal Arena*       ik_frame_arena(void);
internal IK_DrawList* ik_frame_drawlist(void);

//- editor

internal IK_ToolKind ik_tool(void);

//- color
internal Vec4F32 ik_rgba_from_theme_color(IK_ThemeColor color);

//- code -> palette
internal UI_Palette* ik_palette_from_code(IK_PaletteCode code);

//- fonts/size
internal F_Tag ik_font_from_slot(IK_FontSlot slot);
internal F32   ik_font_size_from_slot(IK_FontSlot slot);

//- box lookup
internal IK_Box* ik_box_from_key(IK_Key key);

//- drag data
internal Vec2F32 ik_drag_start_mouse(void);
internal Vec2F32 ik_drag_delta(void);
internal void    ik_store_drag_data(String8 string);
internal String8 ik_get_drag_data(U64 min_required_size);
#define ik_store_drag_struct(ptr) ik_store_drag_data(str8_struct(ptr))
#define ik_get_drag_struct(type) ((type *)ik_get_drag_data(sizeof(type)).str)

/////////////////////////////////
//~ OS event consumption helpers

internal void ik_eat_event(OS_EventList *list, OS_Event *event);
internal B32  ik_key_press(OS_Modifiers mods, OS_Key key);
internal B32  ik_key_release(OS_Modifiers mods, OS_Key key);

/////////////////////////////////
//~ Dynamic drawing (in immediate mode fashion)

//- basic building API
internal IK_DrawList* ik_drawlist_alloc(Arena *arena, U64 vertex_buffer_cap, U64 indice_buffer_cap);
internal IK_DrawNode* ik_drawlist_push(Arena *arena, IK_DrawList *drawlist, R_Vertex *vertices_src, U64 vertex_count, U32 *indices_src, U64 indice_count);
internal void         ik_drawlist_build(IK_DrawList *drawlist); /* upload buffer from cpu to gpu */
internal void         ik_drawlist_reset(IK_DrawList *drawlist);

//- high level building API
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

//- box node destruction
internal void ik_box_release(IK_Box *box);

//- box node equipment
internal String8 ik_box_equip_display_string(IK_Box *box, String8 string);

/////////////////////////////////
//~ User interaction

internal IK_Signal ik_signal_from_box(IK_Box *box);

/////////////////////////////////
//~ UI Widget

internal void ik_ui_stats(void);
internal void ik_ui_toolbar(void);
internal void ik_ui_selection(void);

/////////////////////////////////
//~ Helpers

internal Vec2F32 ik_screen_pos_in_world(Mat4x4F32 proj_view_mat_inv, Vec2F32 pos);
#define ik_mouse_in_world(pvmi) ik_screen_pos_in_world(pvmi, ik_state->mouse)

////////////////////////////////
//~ Macro Loop Wrappers

#define IK_Parent(v) DeferLoop(ik_push_parent(v), ik_pop_parent())
#define IK_Frame(v) DeferLoop(ik_push_frame(v), ik_pop_frame())

#endif
