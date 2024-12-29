#ifndef UI_CORE_H
#define UI_CORE_H

////////////////////////////////
//~ rjf: Icon Info

typedef enum UI_IconKind
{
    UI_IconKind_Null,
    UI_IconKind_RightArrow,
    UI_IconKind_DownArrow,
    UI_IconKind_LeftArrow,
    UI_IconKind_UpArrow,
    UI_IconKind_RightCaret,
    UI_IconKind_DownCaret,
    UI_IconKind_LeftCaret,
    UI_IconKind_UpCaret,
    UI_IconKind_CheckHollow,
    UI_IconKind_CheckFilled,
    UI_IconKind_COUNT
}
UI_IconKind;

typedef struct UI_IconInfo UI_IconInfo;
struct UI_IconInfo
{
    F_Tag   icon_font;
    String8 icon_kind_text_map[UI_IconKind_COUNT];
};

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Mouse Button Kinds

typedef enum UI_MouseButtonKind {
    UI_MouseButtonKind_Left,
    UI_MouseButtonKind_Middle,
    UI_MouseButtonKind_Right,
    UI_MouseButtonKind_COUNT
} UI_MouseButtonKind;

////////////////////////////////
//~ rjf: Focus Types

typedef enum UI_FocusKind {
    UI_FocusKind_Null,
    UI_FocusKind_Off,
    UI_FocusKind_On,
    UI_FocusKind_Root,
    UI_FocusKind_COUNT
} UI_FocusKind;

/////////////////////////////////////////////////////////////////////////////////////////
// Events
typedef enum UI_EventKind {
    UI_EventKind_Null,
    UI_EventKind_Press,
    UI_EventKind_Release,
    UI_EventKind_Text,
    UI_EventKind_Navigate,
    UI_EventKind_Edit,
    UI_EventKind_MouseMove,
    UI_EventKind_Scroll,
    UI_EventKind_FileDrop,
    UI_EventKind_AutocompleteHint,
    UI_EventKind_COUNT
} UI_EventKind;

typedef enum UI_EventActionSlot {
    UI_EventActionSlot_Null,
    UI_EventActionSlot_Accept,
    UI_EventActionSlot_Cancel,
    UI_EventActionSlot_Edit,
    UI_EventActionSlot_COUNT
} UI_EventActionSlot;

typedef U32 UI_EventFlags;
enum
{
    UI_EventFlag_KeepMark            = (1<<0),
    UI_EventFlag_Delete              = (1<<1),
    UI_EventFlag_Copy                = (1<<2),
    UI_EventFlag_Paste               = (1<<3),
    UI_EventFlag_ZeroDeltaOnSelect   = (1<<4),
    UI_EventFlag_PickSelectSide      = (1<<5),
    UI_EventFlag_CapAtLine           = (1<<6),
    UI_EventFlag_ExplicitDirectional = (1<<7),
    UI_EventFlag_Reorder             = (1<<8),
};

typedef enum UI_EventDeltaUnit
{
    UI_EventDeltaUnit_Null,
    UI_EventDeltaUnit_Char,
    UI_EventDeltaUnit_Word,
    UI_EventDeltaUnit_Line,
    UI_EventDeltaUnit_Page,
    UI_EventDeltaUnit_Whole,
    UI_EventDeltaUnit_COUNT
} UI_EventDeltaUnit;

typedef struct UI_Event UI_Event;
struct UI_Event {
    UI_EventKind       kind;
    UI_EventFlags      flags;
    UI_EventActionSlot slot;
    Vec2F32            pos;
    OS_Key             key;
    OS_Modifiers       modifiers;
    String8            string;
    UI_EventDeltaUnit  delta_unit;
    Vec2F32            delta_2f32;
    Vec2S32            delta_2s32;
    U64                timestamp_us;
};

typedef struct UI_EventNode UI_EventNode;
struct UI_EventNode {
    UI_EventNode *next;
    UI_EventNode *prev;
    UI_Event     v;
};

typedef struct UI_EventList UI_EventList;
struct UI_EventList {
    UI_EventNode *first;
    UI_EventNode *last;
    U64          count;
};

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Textual Operations

typedef U32 UI_TxtOpFlags;
enum
{
    UI_TxtOpFlag_Invalid = (1<<0),
    UI_TxtOpFlag_Copy    = (1<<1),
};

typedef struct UI_TxtOp UI_TxtOp;
struct UI_TxtOp
{
    UI_TxtOpFlags flags;
    String8 replace;
    String8 copy;
    TxtRng range;
    TxtPt cursor;
    TxtPt mark;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Keys
typedef struct UI_Key UI_Key;
struct UI_Key  {
    U64 u64[1];
};

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Sizes

typedef enum UI_SizeKind
{
    UI_SizeKind_Null,
    UI_SizeKind_Pixels,      // size is computed via a preferred pixel value
    UI_SizeKind_TextContent, // size is computed via the dimensions of box's rendered string
    UI_SizeKind_ParentPct,   // size is computed via a well-determined parent or grandparent size
    UI_SizeKind_ChildrenSum, // size is computed via summing well-determined sizes of children
} UI_SizeKind;

typedef struct UI_Size UI_Size;
struct UI_Size
{
    UI_SizeKind kind;
    F32 value;
    F32 strictness;
};

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Palettes

typedef enum UI_ColorCode
{
    UI_ColorCode_Null,
    UI_ColorCode_Background,
    UI_ColorCode_Text,
    UI_ColorCode_TextWeak,
    UI_ColorCode_Border,
    UI_ColorCode_Overlay,
    UI_ColorCode_Cursor,
    UI_ColorCode_Selection,
    UI_ColorCode_COUNT
} UI_ColorCode;

typedef struct UI_Palette UI_Palette;
struct UI_Palette
{
    union
    {
        Vec4F32 colors[UI_ColorCode_COUNT];
        struct
        {
            Vec4F32 null;
            Vec4F32 background;
            Vec4F32 text;
            Vec4F32 text_weak;
            Vec4F32 border;
            Vec4F32 overlay;
            Vec4F32 cursor;
            Vec4F32 selection;
        };
    };
};

typedef struct UI_WidgetPaletteInfo UI_WidgetPaletteInfo;
struct UI_WidgetPaletteInfo
{
    UI_Palette *tooltip_palette;
    UI_Palette *ctx_menu_palette;
    UI_Palette *scrollbar_palette;
};

////////////////////////////////
//~ rjf: Animation Info

typedef U32 UI_AnimationInfoFlags;
enum
{
    UI_AnimationInfoFlag_HotAnimations          = (1<<0),
    UI_AnimationInfoFlag_ActiveAnimations       = (1<<1),
    UI_AnimationInfoFlag_FocusAnimations        = (1<<2),
    UI_AnimationInfoFlag_TooltipAnimations      = (1<<3),
    UI_AnimationInfoFlag_ContextMenuAnimations  = (1<<4),
    UI_AnimationInfoFlag_ScrollingAnimations    = (1<<5),
    UI_AnimationInfoFlag_All = 0xffffffff,
};

typedef struct UI_AnimationInfo UI_AnimationInfo;
struct UI_AnimationInfo
{
    UI_AnimationInfoFlags flags;
};

////////////////////////////////
//~ rjf: Scroll Positions

typedef struct UI_ScrollPt UI_ScrollPt;
struct UI_ScrollPt
{
    S64 idx;
    F32 off;
};

typedef union UI_ScrollPt2 UI_ScrollPt2;
union UI_ScrollPt2
{
    UI_ScrollPt v[2];
    struct
    {
        UI_ScrollPt x;
        UI_ScrollPt y;
    };
};

/////////////////////////////////////////////////////////////////////////////////////////
// Box types

typedef enum UI_TextAlign
{
    UI_TextAlign_Left,
    UI_TextAlign_Center,
    UI_TextAlign_Right,
    UI_TextAlign_COUNT
} UI_TextAlign;

struct UI_Box;
#define UI_BOX_CUSTOM_DRAW(name) void name(struct UI_Box *box, void *user_data)
typedef UI_BOX_CUSTOM_DRAW(UI_BoxCustomDrawFunctionType);

typedef U64 UI_BoxFlags;
//{
//- rjf: interaction
# define UI_BoxFlag_MouseClickable            (UI_BoxFlags)(1ull<<0)
# define UI_BoxFlag_KeyboardClickable         (UI_BoxFlags)(1ull<<1)
# define UI_BoxFlag_DropSite                  (UI_BoxFlags)(1ull<<2)
# define UI_BoxFlag_ClickToFocus              (UI_BoxFlags)(1ull<<3)
# define UI_BoxFlag_Scroll                    (UI_BoxFlags)(1ull<<4)
# define UI_BoxFlag_ViewScrollX               (UI_BoxFlags)(1ull<<5)
# define UI_BoxFlag_ViewScrollY               (UI_BoxFlags)(1ull<<6)
# define UI_BoxFlag_ViewClampX                (UI_BoxFlags)(1ull<<7)
# define UI_BoxFlag_ViewClampY                (UI_BoxFlags)(1ull<<8)
# define UI_BoxFlag_FocusHot                  (UI_BoxFlags)(1ull<<9)
# define UI_BoxFlag_FocusActive               (UI_BoxFlags)(1ull<<10)
# define UI_BoxFlag_FocusHotDisabled          (UI_BoxFlags)(1ull<<11)
# define UI_BoxFlag_FocusActiveDisabled       (UI_BoxFlags)(1ull<<12)
# define UI_BoxFlag_DefaultFocusNavX          (UI_BoxFlags)(1ull<<13)
# define UI_BoxFlag_DefaultFocusNavY          (UI_BoxFlags)(1ull<<14)
# define UI_BoxFlag_DefaultFocusEdit          (UI_BoxFlags)(1ull<<15)
# define UI_BoxFlag_FocusNavSkip              (UI_BoxFlags)(1ull<<16)
# define UI_BoxFlag_DisableTruncatedHover     (UI_BoxFlags)(1ull<<17)
# define UI_BoxFlag_Disabled                  (UI_BoxFlags)(1ull<<18)

//- rjf: layout
# define UI_BoxFlag_FloatingX                 (UI_BoxFlags)(1ull<<19)
# define UI_BoxFlag_FloatingY                 (UI_BoxFlags)(1ull<<20)
# define UI_BoxFlag_FixedWidth                (UI_BoxFlags)(1ull<<21)
# define UI_BoxFlag_FixedHeight               (UI_BoxFlags)(1ull<<22)
# define UI_BoxFlag_AllowOverflowX            (UI_BoxFlags)(1ull<<23)
# define UI_BoxFlag_AllowOverflowY            (UI_BoxFlags)(1ull<<24)
# define UI_BoxFlag_SkipViewOffX              (UI_BoxFlags)(1ull<<25)
# define UI_BoxFlag_SkipViewOffY              (UI_BoxFlags)(1ull<<26)

//- rjf: appearance / animation
# define UI_BoxFlag_DrawDropShadow            (UI_BoxFlags)(1ull<<27)
# define UI_BoxFlag_DrawBackgroundBlur        (UI_BoxFlags)(1ull<<28)
# define UI_BoxFlag_DrawBackground            (UI_BoxFlags)(1ull<<29)
# define UI_BoxFlag_DrawBorder                (UI_BoxFlags)(1ull<<30)
# define UI_BoxFlag_DrawSideTop               (UI_BoxFlags)(1ull<<31)
# define UI_BoxFlag_DrawSideBottom            (UI_BoxFlags)(1ull<<32)
# define UI_BoxFlag_DrawSideLeft              (UI_BoxFlags)(1ull<<33)
# define UI_BoxFlag_DrawSideRight             (UI_BoxFlags)(1ull<<34)
# define UI_BoxFlag_DrawText                  (UI_BoxFlags)(1ull<<35)
# define UI_BoxFlag_DrawTextFastpathCodepoint (UI_BoxFlags)(1ull<<36)
# define UI_BoxFlag_DrawTextWeak              (UI_BoxFlags)(1ull<<37)
# define UI_BoxFlag_DrawHotEffects            (UI_BoxFlags)(1ull<<38)
# define UI_BoxFlag_DrawActiveEffects         (UI_BoxFlags)(1ull<<39)
# define UI_BoxFlag_DrawOverlay               (UI_BoxFlags)(1ull<<40)
# define UI_BoxFlag_DrawBucket                (UI_BoxFlags)(1ull<<41)
# define UI_BoxFlag_Clip                      (UI_BoxFlags)(1ull<<42)
# define UI_BoxFlag_AnimatePosX               (UI_BoxFlags)(1ull<<43)
# define UI_BoxFlag_AnimatePosY               (UI_BoxFlags)(1ull<<44)
# define UI_BoxFlag_DisableTextTrunc          (UI_BoxFlags)(1ull<<45)
# define UI_BoxFlag_DisableIDString           (UI_BoxFlags)(1ull<<46)
# define UI_BoxFlag_DisableFocusBorder        (UI_BoxFlags)(1ull<<47)
# define UI_BoxFlag_DisableFocusOverlay       (UI_BoxFlags)(1ull<<48)
# define UI_BoxFlag_HasDisplayString          (UI_BoxFlags)(1ull<<49)
# define UI_BoxFlag_HasFuzzyMatchRanges       (UI_BoxFlags)(1ull<<50)
# define UI_BoxFlag_RoundChildrenByParent     (UI_BoxFlags)(1ull<<51)

//- rjf: bundles
# define UI_BoxFlag_Clickable           (UI_BoxFlag_MouseClickable|UI_BoxFlag_KeyboardClickable)
# define UI_BoxFlag_DefaultFocusNav     (UI_BoxFlag_DefaultFocusNavX|UI_BoxFlag_DefaultFocusNavY|UI_BoxFlag_DefaultFocusEdit)
# define UI_BoxFlag_Floating            (UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY)
# define UI_BoxFlag_FixedSize           (UI_BoxFlag_FixedWidth|UI_BoxFlag_FixedHeight)
# define UI_BoxFlag_AllowOverflow       (UI_BoxFlag_AllowOverflowX|UI_BoxFlag_AllowOverflowY)
# define UI_BoxFlag_AnimatePos          (UI_BoxFlag_AnimatePosX|UI_BoxFlag_AnimatePosY)
# define UI_BoxFlag_ViewScroll          (UI_BoxFlag_ViewScrollX|UI_BoxFlag_ViewScrollY)
# define UI_BoxFlag_ViewClamp           (UI_BoxFlag_ViewClampX|UI_BoxFlag_ViewClampY)
# define UI_BoxFlag_DisableFocusEffects (UI_BoxFlag_DisableFocusBorder|UI_BoxFlag_DisableFocusOverlay)
//}

typedef struct UI_Box UI_Box;
struct UI_Box {
    // Persistent links for map
    UI_Box                       *hash_next;
    UI_Box                       *hash_prev;

    // Children
    UI_Box                       *first;
    UI_Box                       *last;
    // Siblings
    UI_Box                       *next;
    UI_Box                       *prev;

    UI_Box                       *parent;

    // Per-build equipment
    UI_Key                       key;
    String8                      string;
    String8                      indentifier;
    UI_TextAlign                 text_align;
    UI_BoxFlags                  flags;
    Vec2F32                      fixed_position;
    Vec2F32                      fixed_size;
    UI_Size                      pref_size[Axis2_COUNT];
    Axis2                        child_layout_axis;
    U64                          child_count;
    F_Tag                        font;
    U64                          font_size;
    U64                          tab_size;
    F_RasterFlags                text_raster_flags;
    F32                          corner_radii[Corner_COUNT];
    F32                          transparency;
    F32                          text_padding;
    UI_Palette                   *palette;
    D_Bucket                     *draw_bucket;
    UI_BoxCustomDrawFunctionType *custom_draw;
    void                         *custom_draw_user_data;
    OS_Cursor                    hover_cursor;

    // Per-build artifacts
    Rng2F32                      rect;
    D_FancyRunList               display_string_runs;
    Vec2F32                      fixed_position_animated;
    Vec2F32                      position_delta;

    // Persistent data
    U64                          first_touched_build_index;
    U64                          last_touched_build_index;
    U64                          first_disabled_build_index;
    F32                          hot_t;
    F32                          active_t;
    F32                          disabled_t;
    F32                          focus_hot_t;
    F32                          focus_active_t;
    F32                          focus_active_disabled_t;
    Vec2F32                      view_off;
    Vec2F32                      view_off_target;
    Vec2F32                      view_bounds;
    UI_Key                       default_nav_focus_next_hot_key;
    UI_Key                       default_nav_focus_next_active_key;
    UI_Key                       default_nav_focus_hot_key;
    UI_Key                       default_nav_focus_active_key;
};

typedef struct UI_BoxRec UI_BoxRec;
struct UI_BoxRec
{
    UI_Box *next;
    S32 push_count;
    S32 pop_count;
};

typedef struct UI_BoxNode UI_BoxNode;
struct UI_BoxNode {
    UI_BoxNode *next;
    UI_Box *box;
};

typedef struct UI_BoxList UI_BoxList;
struct UI_BoxList {
    UI_BoxNode *first;
    UI_BoxNode *last;
    U64        count;
};

typedef U32 UI_SignalFlags;
enum
{
    // rjf: mouse press -> box was pressed while hovering
    UI_SignalFlag_LeftPressed         = (1<<0),
    UI_SignalFlag_MiddlePressed       = (1<<1),
    UI_SignalFlag_RightPressed        = (1<<2),

    // rjf: dragging -> box was previously pressed, user is still holding button
    UI_SignalFlag_LeftDragging        = (1<<3),
    UI_SignalFlag_MiddleDragging      = (1<<4),
    UI_SignalFlag_RightDragging       = (1<<5),

    // rjf: double-dragging -> box was previously double-clicked, user is still holding button
    UI_SignalFlag_LeftDoubleDragging  = (1<<6),
    UI_SignalFlag_MiddleDoubleDragging= (1<<7),
    UI_SignalFlag_RightDoubleDragging = (1<<8),

    // rjf: triple-dragging -> box was previously triple-clicked, user is still holding button
    UI_SignalFlag_LeftTripleDragging  = (1<<9),
    UI_SignalFlag_MiddleTripleDragging= (1<<10),
    UI_SignalFlag_RightTripleDragging = (1<<11),

    // rjf: released -> box was previously pressed & user released, in or out of bounds
    UI_SignalFlag_LeftReleased        = (1<<12),
    UI_SignalFlag_MiddleReleased      = (1<<13),
    UI_SignalFlag_RightReleased       = (1<<14),

    // rjf: clicked -> box was previously pressed & user released, in bounds
    UI_SignalFlag_LeftClicked         = (1<<15),
    UI_SignalFlag_MiddleClicked       = (1<<16),
    UI_SignalFlag_RightClicked        = (1<<17),

    // rjf: double clicked -> box was previously clicked, pressed again
    UI_SignalFlag_LeftDoubleClicked   = (1<<18),
    UI_SignalFlag_MiddleDoubleClicked = (1<<19),
    UI_SignalFlag_RightDoubleClicked  = (1<<20),

    // rjf: triple clicked -> box was previously clicked twice, pressed again
    UI_SignalFlag_LeftTripleClicked   = (1<<21),
    UI_SignalFlag_MiddleTripleClicked = (1<<22),
    UI_SignalFlag_RightTripleClicked  = (1<<23),

    // rjf: keyboard pressed -> box had focus, user activated via their keyboard
    UI_SignalFlag_KeyboardPressed     = (1<<24),

    // rjf: passive mouse info
    UI_SignalFlag_Hovering            = (1<<25), // hovering specifically this box
    UI_SignalFlag_MouseOver           = (1<<26), // mouse is over, but may be occluded

    // rjf: committing state changes via user interaction
    UI_SignalFlag_Commit              = (1<<27),

    // rjf: high-level combos
    UI_SignalFlag_Pressed = UI_SignalFlag_LeftPressed|UI_SignalFlag_KeyboardPressed,
    UI_SignalFlag_Released = UI_SignalFlag_LeftReleased,
    UI_SignalFlag_Clicked = UI_SignalFlag_LeftClicked|UI_SignalFlag_KeyboardPressed,
    UI_SignalFlag_DoubleClicked = UI_SignalFlag_LeftDoubleClicked,
    UI_SignalFlag_TripleClicked = UI_SignalFlag_LeftTripleClicked,
    UI_SignalFlag_Dragging = UI_SignalFlag_LeftDragging,
};

typedef struct UI_Signal UI_Signal;
struct UI_Signal {
    UI_Box *box;
    OS_Modifiers event_flags;
    Vec2S16 scroll;
    UI_SignalFlags f;
};

#define ui_pressed(s)        !!((s).f&UI_SignalFlag_Pressed)
#define ui_clicked(s)        !!((s).f&UI_SignalFlag_Clicked)
#define ui_released(s)       !!((s).f&UI_SignalFlag_Released)
#define ui_double_clicked(s) !!((s).f&UI_SignalFlag_DoubleClicked)
#define ui_triple_clicked(s) !!((s).f&UI_SignalFlag_TripleClicked)
#define ui_middle_clicked(s) !!((s).f&UI_SignalFlag_MiddleClicked)
#define ui_right_clicked(s)  !!((s).f&UI_SignalFlag_RightClicked)
#define ui_dragging(s)       !!((s).f&UI_SignalFlag_Dragging)
#define ui_hovering(s)       !!((s).f&UI_SignalFlag_Hovering)
#define ui_mouse_over(s)     !!((s).f&UI_SignalFlag_MouseOver)
#define ui_committed(s)      !!((s).f&UI_SignalFlag_Commit)

/////////////////////////////////////////////////////////////////////////////////////////
// Generated Code
// UI state stacks

#include "generated/ui.meta.h"

//- rjf: helpers
internal Rng2F32  ui_push_rect(Rng2F32 rect);
internal Rng2F32  ui_pop_rect(void);
internal void     ui_set_next_rect(Rng2F32 rect);
internal UI_Size  ui_push_pref_size(Axis2 axis, UI_Size v);
internal UI_Size  ui_pop_pref_size(Axis2 axis);
internal UI_Size  ui_set_next_pref_size(Axis2 axis, UI_Size v);
internal void     ui_push_corner_radius(F32 v);
internal void     ui_pop_corner_radius(void);

////////////////////////////////
//~ rjf: Macro Loop Wrappers

//- rjf: stacks (base)
#define UI_Parent(v) DeferLoop(ui_push_parent(v), ui_pop_parent())
#define UI_ChildLayoutAxis(v) DeferLoop(ui_push_child_layout_axis(v), ui_pop_child_layout_axis())
#define UI_FixedX(v) DeferLoop(ui_push_fixed_x(v), ui_pop_fixed_x())
#define UI_FixedY(v) DeferLoop(ui_push_fixed_y(v), ui_pop_fixed_y())
#define UI_FixedWidth(v) DeferLoop(ui_push_fixed_width(v), ui_pop_fixed_width())
#define UI_FixedHeight(v) DeferLoop(ui_push_fixed_height(v), ui_pop_fixed_height())
#define UI_PrefWidth(v) DeferLoop(ui_push_pref_width(v), ui_pop_pref_width())
#define UI_PrefHeight(v) DeferLoop(ui_push_pref_height(v), ui_pop_pref_height())
#define UI_Flags(v) DeferLoop(ui_push_flags(v), ui_pop_flags())
#define UI_FocusHot(v) DeferLoop(ui_push_focus_hot(v), ui_pop_focus_hot())
#define UI_FocusActive(v) DeferLoop(ui_push_focus_active(v), ui_pop_focus_active())
#define UI_FastpathCodepoint(v) DeferLoop(ui_push_fastpath_codepoint(v), ui_pop_fastpath_codepoint())
#define UI_GroupKey(v) DeferLoop(ui_push_group_key(v), ui_pop_group_key())
#define UI_Transparency(v) DeferLoop(ui_push_transparency(v), ui_pop_transparency())
#define UI_Palette(v) DeferLoop(ui_push_palette(v), ui_pop_palette())
#define UI_Squish(v) DeferLoop(ui_push_squish(v), ui_pop_squish())
#define UI_HoverCursor(v) DeferLoop(ui_push_hover_cursor(v), ui_pop_hover_cursor())
#define UI_Font(v) DeferLoop(ui_push_font(v), ui_pop_font())
#define UI_FontSize(v) DeferLoop(ui_push_font_size(v), ui_pop_font_size())
#define UI_TextRasterFlags(v) DeferLoop(ui_push_text_raster_flags(v), ui_pop_text_raster_flags())
#define UI_TabSize(v) DeferLoop(ui_push_tab_size(v), ui_pop_tab_size())
#define UI_CornerRadius00(v) DeferLoop(ui_push_corner_radius_00(v), ui_pop_corner_radius_00())
#define UI_CornerRadius01(v) DeferLoop(ui_push_corner_radius_01(v), ui_pop_corner_radius_01())
#define UI_CornerRadius10(v) DeferLoop(ui_push_corner_radius_10(v), ui_pop_corner_radius_10())
#define UI_CornerRadius11(v) DeferLoop(ui_push_corner_radius_11(v), ui_pop_corner_radius_11())
#define UI_BlurSize(v) DeferLoop(ui_push_blur_size(v), ui_pop_blur_size())
#define UI_TextPadding(v) DeferLoop(ui_push_text_padding(v), ui_pop_text_padding())
#define UI_TextAlignment(v) DeferLoop(ui_push_text_alignment(v), ui_pop_text_alignment())

//- rjf: stacks (compositions)
#define UI_WidthFill         UI_PrefWidth(ui_pct(1.f, 0.f))
#define UI_HeightFill        UI_PrefHeight(ui_pct(1.f, 0.f))
#define UI_Rect(r)           DeferLoop(ui_push_rect(r), ui_pop_rect())
#define UI_PrefSize(axis, v) DeferLoop(ui_push_pref_size((axis), (v)), ui_pop_pref_size(axis))
#define UI_CornerRadius(v)   DeferLoop(ui_push_corner_radius(v), ui_pop_corner_radius())
#define UI_Focus(kind)       DeferLoop((ui_push_focus_hot(kind), ui_push_focus_active(kind)), (ui_pop_focus_hot(), ui_pop_focus_active()))
#define UI_FlagsAdd(v)       DeferLoop(ui_push_flags(ui_top_flags()|(v)), ui_pop_flags())

//- rjf: tooltip
#define UI_TooltipBase DeferLoop(ui_tooltip_begin_base(), ui_tooltip_end_base())
#define UI_Tooltip DeferLoop(ui_tooltip_begin(), ui_tooltip_end())

//- rjf: context menu
#define UI_CtxMenu(key) DeferLoopChecked(ui_begin_ctx_menu(key), ui_end_ctx_menu())

/////////////////////////////////////////////////////////////////////////////////////////
// State Types
typedef struct UI_BoxHashSlot UI_BoxHashSlot;
struct UI_BoxHashSlot
{
    UI_Box *hash_first;
    UI_Box *hash_last;
};

typedef struct UI_State UI_State;
struct UI_State
{
    // Main arena
    Arena                *arena;

    // Build arena
    Arena                *build_arenas[2];
    U64                  build_index;

    // Box Cache
    UI_Box               *first_free_box;
    U64                  box_table_size;
    UI_BoxHashSlot       *box_table;

    // Build phase output
    UI_Box               *root;
    U64                  build_box_count;
    U64                  last_build_box_count;

    // Build parameters
    UI_IconInfo          icon_info;
    UI_WidgetPaletteInfo widget_palette_info;
    UI_AnimationInfo     animation_info;
    OS_Handle            os_wnd;
    Vec2F32              mouse;
    F32                  animation_dt;
    F32                  default_animation_rate;
    UI_EventList         *events;

    // User interaction state
    UI_Key               hot_box_key;
    UI_Key               active_box_key[UI_MouseButtonKind_COUNT];
    UI_Key               drop_hot_box_key;
    // Drag
    Vec2F32              drag_start_mouse;
    Arena                *drag_state_arena;
    String8              drag_state_data;
    U64                  press_timestamp_history_us[UI_MouseButtonKind_COUNT][3];
    UI_Key               press_key_history[UI_MouseButtonKind_COUNT][3];
    U64                  last_time_mousemoved_us;

    // Build phase stacks
    UI_DeclStackNils;
    UI_DeclStacks;
};

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Basic Type Functions

internal U64     ui_hash_from_string(U64 seed, String8 string);
internal String8 ui_hash_part_from_key_string(String8 string);
internal String8 ui_display_part_from_key_string(String8 string);
internal UI_Key  ui_key_zero(void);
internal UI_Key  ui_key_make(U64 v);
internal UI_Key  ui_key_from_string(UI_Key seed_key, String8 string);
internal UI_Key  ui_key_from_stringf(UI_Key seed_key, char *fmt, ...);
internal B32     ui_key_match(UI_Key a, UI_Key b);

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Event Type Functions

internal UI_EventNode *ui_event_list_push(Arena *arena, UI_EventList *list, UI_Event *v);
internal void ui_eat_event(UI_EventList *list, UI_EventNode *node);

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Text Operation Functions

internal B32 ui_char_is_scan_boundary(U8 c);
internal S64 ui_scanned_column_from_column(String8 string, S64 start_column, Side side);
internal UI_TxtOp ui_single_line_txt_op_from_event(Arena *arena, UI_Event *event, String8 string, TxtPt cursor, TxtPt mark);
internal String8 ui_push_string_replace_range(Arena *arena, String8 string, Rng1S64 range, String8 replace);

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Size Type Functions

internal UI_Size ui_size(UI_SizeKind kind, F32 value, F32 strictness);
#define ui_px(value, strictness)         ui_size(UI_SizeKind_Pixels, value, strictness)
#define ui_em(value, strictness)         ui_size(UI_SizeKind_Pixels, (value) * ui_top_font_size(), strictness)
#define ui_text_dim(padding, strictness) ui_size(UI_SizeKind_TextContent, padding, strictness)
#define ui_pct(value, strictness)        ui_size(UI_SizeKind_ParentPct, value, strictness)
#define ui_children_sum(strictness)      ui_size(UI_SizeKind_ChildrenSum, 0.f, strictness)

////////////////////////////////
//~ rjf: Color Scheme Type Functions

read_only global UI_Palette ui_g_nil_palette = {0};

////////////////////////////////
//~ rjf: Scroll Point Type Functions

internal UI_ScrollPt ui_scroll_pt(S64 idx, F32 off);
internal void ui_scroll_pt_target_idx(UI_ScrollPt *v, S64 idx);
internal void ui_scroll_pt_clamp_idx(UI_ScrollPt *v, Rng1S64 range);

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Box Type Functions

read_only global UI_Box ui_nil_box = {
    &ui_nil_box,
    &ui_nil_box,
    &ui_nil_box,
    &ui_nil_box,
    &ui_nil_box,
    &ui_nil_box,
    &ui_nil_box,
};
internal B32 ui_box_is_nil(UI_Box *box);
internal UI_BoxRec ui_box_rec_df(UI_Box *box, UI_Box *root, U64 sib_member_off, U64 child_member_off);
#define ui_box_rec_df_pre(box, root) ui_box_rec_df(box, root, OffsetOf(UI_Box, next), OffsetOf(UI_Box, first))
#define ui_box_rec_df_post(box, root) ui_box_rec_df(box, root, OffsetOf(UI_Box, prev), OffsetOf(UI_Box, last))
// internal void ui_box_list_push(Arena *arena, UI_BoxList *list, UI_Box *box);

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: State Allocating / Selection

internal UI_State *ui_state_alloc(void);
internal void     ui_state_release(UI_State *state);
internal UI_Box   *ui_root_from_state(UI_State *state);
internal void     ui_select_state(UI_State *state);

/////////////////////////////////////////////////////////////////////////////////////////
// Implicit State Accessor/Mutators

//- rjf: per-frame info
internal Arena             *ui_build_arena(void);
internal OS_Handle         ui_window(void);
internal UI_EventList      *ui_events(void);
internal F_Tag             ui_icon_font(void);
internal String8           ui_icon_string_from_kind(UI_IconKind icon_kind);
internal Vec2F32           ui_mouse(void);
internal F32               ui_dt(void);

//- rjf: drag data
internal Vec2F32           ui_drag_start_mouse(void);
internal Vec2F32           ui_drag_delta(void);
internal void              ui_store_drag_data(String8 string);
internal String8           ui_get_drag_data(U64 min_required_size);
#define ui_store_drag_struct(ptr) ui_store_drag_data(str8_struct(ptr))
#define ui_get_drag_struct(type) ((type *)ui_get_drag_data(sizeof(type)).str)

/////////////////////////////////////////////////////////////////////////////////////////
//- rjf: interaction keys
internal UI_Key            ui_hot_key(void);
internal UI_Key            ui_active_key(UI_MouseButtonKind button_kind);
internal UI_Key            ui_drop_hot_key(void);

//- rjf: controls over interaction
internal void              ui_kill_action(void);

//- rjf: box cache lookup
internal UI_Box *ui_box_from_key(UI_Key key);

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Top-Level Building API

internal void ui_begin_build(OS_Handle os_wnd, UI_EventList *events, UI_IconInfo *icon_info, UI_WidgetPaletteInfo *widget_palette_info, UI_AnimationInfo *animation_info, F32 animation_dt);
internal void ui_end_build(void);
internal void ui_calc_sizes_standalone__in_place_rec(UI_Box *root, Axis2 axis);
internal void ui_calc_sizes_upwards_dependent__in_place_rec(UI_Box *root, Axis2 axis);
internal void ui_calc_sizes_downwards_dependent__in_place_rec(UI_Box *root, Axis2 axis);
internal void ui_layout_enforce_constraints__in_place_rec(UI_Box *root, Axis2 axis);
internal void ui_layout_position__in_place_rec(UI_Box *root, Axis2 axis);
internal void ui_layout_root(UI_Box *root, Axis2 axis);

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Box Tree Building API

//- rjf: box node construction
internal UI_Box *ui_build_box_from_key(UI_BoxFlags flags, UI_Key key);
internal UI_Key ui_active_seed_key(void);
internal UI_Box *ui_build_box_from_string(UI_BoxFlags flags, String8 string);
internal UI_Box *ui_build_box_from_stringf(UI_BoxFlags flags, char *fmt, ...);

//- rjf: box node equipment
internal inline void       ui_box_equip_display_string(UI_Box *box, String8 string);
internal inline void       ui_box_equip_draw_bucket(UI_Box *box, D_Bucket *bucket);
internal inline void       ui_box_equip_custom_draw(UI_Box *box, UI_BoxCustomDrawFunctionType *custom_draw, void *user_data);

//- rjf: box accessors / queries
internal String8           ui_box_display_string(UI_Box *box);
internal Vec2F32           ui_box_text_position(UI_Box *box);
internal U64               ui_box_char_pos_from_xy(UI_Box *box, Vec2F32 xy);

//- rjf: focus tree coloring
internal B32               ui_is_focus_hot(void);
internal B32               ui_is_focus_active(void);

//- rjf: implicit auto-managed tree-based focus state
internal B32               ui_is_key_auto_focus_active(UI_Key key);
internal B32               ui_is_key_auto_focus_hot(UI_Key key);
internal void              ui_set_auto_focus_active_key(UI_Key key);
internal void              ui_set_auto_focus_hot_key(UI_Key key);

//- rjf: palette forming
internal UI_Palette *      ui_build_palette_(UI_Palette *base, UI_Palette *overrides);
#define ui_build_palette(base, ...) ui_build_palette_((base), &(UI_Palette){.text = v4f32(0, 0, 0, 0), __VA_ARGS__})

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: User Interaction

internal UI_Signal ui_signal_from_box(UI_Box *box);

/////////////////////////////////////////////////////////////////////////////////////////
// Globals
thread_static UI_State *ui_state = 0;

#endif
