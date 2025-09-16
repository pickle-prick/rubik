// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef UI_BASIC_WIDGETS_H
#define UI_BASIC_WIDGETS_H

////////////////////////////////
//~ rjf: Scroll List Types

typedef U32 UI_ScrollListFlags;
enum
{
  UI_ScrollListFlag_Nav  = (1<<0),
  UI_ScrollListFlag_Snap = (1<<1),
  UI_ScrollListFlag_All  = 0xffffffff,
};

typedef struct UI_ScrollListRowBlock UI_ScrollListRowBlock;
struct UI_ScrollListRowBlock
{
  U64 row_count;
  U64 item_count;
};

typedef struct UI_ScrollListRowBlockChunkNode UI_ScrollListRowBlockChunkNode;
struct UI_ScrollListRowBlockChunkNode
{
  UI_ScrollListRowBlockChunkNode *next;
  UI_ScrollListRowBlock *v;
  U64 count;
  U64 cap;
};

typedef struct UI_ScrollListRowBlockChunkList UI_ScrollListRowBlockChunkList;
struct UI_ScrollListRowBlockChunkList
{
  UI_ScrollListRowBlockChunkNode *first;
  UI_ScrollListRowBlockChunkNode *last;
  U64 chunk_count;
  U64 total_count;
};

typedef struct UI_ScrollListRowBlockArray UI_ScrollListRowBlockArray;
struct UI_ScrollListRowBlockArray
{
  UI_ScrollListRowBlock *v;
  U64 count;
};

typedef struct UI_ScrollListParams UI_ScrollListParams;
struct UI_ScrollListParams
{
  UI_ScrollListFlags flags;
  Vec2F32 dim_px;
  F32 row_height_px;
  UI_ScrollListRowBlockArray row_blocks;
  Rng2S64 cursor_range;
  Rng1S64 item_range;
  B32 cursor_min_is_empty_selection[Axis2_COUNT];
};

typedef struct UI_ScrollListSignal UI_ScrollListSignal;
struct UI_ScrollListSignal
{
  B32 cursor_moved;
};

////////////////////////////////
//~ rjf: Basic Widgets

internal void ui_divider(UI_Size size);
internal UI_Signal ui_label(String8 string);
internal UI_Signal ui_labelf(char *fmt, ...);
internal void ui_label_multiline(F32 max, String8 string);
internal void ui_label_multilinef(F32 max, char *fmt, ...);
internal UI_Signal ui_spacer(UI_Size size);
internal UI_Signal ui_button(String8 string);
internal UI_Signal ui_buttonf(char *fmt, ...);
internal UI_Signal ui_hover_label(String8 string);
internal UI_Signal ui_hover_labelf(char *fmt, ...);
internal UI_Signal ui_line_edit(TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, String8 pre_edit_value, String8 string);
internal UI_Signal ui_line_editf(TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, String8 pre_edit_value, char *fmt, ...);
internal UI_Signal ui_f32_edit(F32 *n, F32 min, F32 max, TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, B32 *has_draft, String8 string);

//- rjf: too tips
internal void      ui_tooltip_begin_base(void);
internal void      ui_tooltip_end_base(void);
internal void      ui_tooltip_begin(void);
internal void      ui_tooltip_end(void);

//- rjf: context menus
internal void ui_ctx_menu_open(UI_Key key, UI_Key anchor_box_key, Vec2F32 anchor_off);
internal void ui_ctx_menu_close(void);
internal B32  ui_begin_ctx_menu(UI_Key key);
internal void ui_end_ctx_menu(void);
internal B32  ui_ctx_menu_is_open(UI_Key key);
internal B32  ui_any_ctx_menu_is_open(void);

////////////////////////////////
//~ rjf: Images

internal UI_Signal ui_image(R_Handle texture, R_Tex2DSampleKind sample_kind, Rng2F32 region, Vec4F32 tint, F32 blur, String8 string);
internal UI_Signal ui_imagef(R_Handle texture, R_Tex2DSampleKind sample_kind, Rng2F32 region, Vec4F32 tint, F32 blur, char *fmt, ...);

////////////////////////////////
//~ rjf: Special Buttons

internal UI_Signal ui_close(String8 string);
internal UI_Signal ui_closef(char *fmt, ...);
internal UI_Signal ui_expander(B32 is_expanded, String8 string);
internal UI_Signal ui_expanderf(B32 is_expanded, char *fmt, ...);

////////////////////////////////
//~ rjf: Color Pickers

//- rjf: tooltips
internal void ui_do_color_tooltip_hsv(Vec3F32 hsv);
internal void ui_do_color_tooltip_hsva(Vec4F32 hsva);

//- rjf: saturation/value picker
internal UI_Signal ui_sat_val_picker(F32 hue, F32 *out_sat, F32 *out_val, String8 string);
internal UI_Signal ui_sat_val_pickerf(F32 hue, F32 *out_sat, F32 *out_val, char *fmt, ...);

//- rjf: hue picker
internal UI_Signal ui_hue_picker(F32 *out_hue, F32 sat, F32 val, String8 string);
internal UI_Signal ui_hue_pickerf(F32 *out_hue, F32 sat, F32 val, char *fmt, ...);

//- rjf: alpha picker
internal UI_Signal ui_alpha_picker(F32 *out_alpha, String8 string);
internal UI_Signal ui_alpha_pickerf(F32 *out_alpha, char *fmt, ...);

////////////////////////////////
//~ rjf: Simple Layout Widgets

internal UI_Box *ui_row_begin(void);
internal UI_Signal ui_row_end(void);
internal UI_Box *ui_column_begin(void);
internal UI_Signal ui_column_end(void);
internal UI_Box *ui_named_row_begin(String8 string);
internal UI_Signal ui_named_row_end(void);
internal UI_Box *ui_named_column_begin(String8 string);
internal UI_Signal ui_named_column_end(void);

////////////////////////////////
//~ rjf: Floating Panes

internal UI_Box *ui_pane_begin(Rng2F32 rect, String8 string);
internal UI_Box *ui_pane_beginf(Rng2F32 rect, char *fmt, ...);
internal UI_Signal ui_pane_end(void);

////////////////////////////////
//~ k: Scroll Regions

internal UI_ScrollPt ui_scroll_bar(Axis2 axis, UI_Size off_axis_size, UI_ScrollPt pt, Rng1S64 idx_range, S64 view_num_indices);
internal void ui_scroll_list_begin(UI_ScrollListParams *params, UI_ScrollPt *scroll_pt_out, Vec2S64 *cursor_out, Vec2S64 *mark_out, Rng1S64 *visible_row_range_out, UI_ScrollListSignal *signal_out);
internal void ui_scroll_list_end(void);

////////////////////////////////
//~ rjf: Macro Loop Wrappers

#define UI_Row DeferLoop(ui_row_begin(), ui_row_end())
#define UI_Column DeferLoop(ui_column_begin(), ui_column_end())
#define UI_NamedRow(s) DeferLoop(ui_named_row_begin(s), ui_named_row_end())
#define UI_NamedColumn(s) DeferLoop(ui_named_column_begin(s), ui_named_column_end())
#define UI_Pane(r, s) DeferLoop(ui_pane_begin(r, s), ui_pane_end())
#define UI_PaneF(r, ...) DeferLoop(ui_pane_beginf(r, __VA_ARGS__), ui_pane_end())
#define UI_Padding(size) DeferLoop(ui_spacer(size), ui_spacer(size))
#define UI_Center UI_Padding(ui_pct(1, 0))
#define UI_ScrollList(params, scroll_pt_out, cursor_out, mark_out, visible_row_range_out, signal_out) DeferLoop(ui_scroll_list_begin((params), (scroll_pt_out), (cursor_out), (mark_out), (visible_row_range_out), (signal_out)), ui_scroll_list_end())

// tooltip
#define UI_TooltipBase DeferLoop(ui_tooltip_begin_base(), ui_tooltip_end_base())
#define UI_Tooltip DeferLoop(ui_tooltip_begin(), ui_tooltip_end())

// ctx menu
#define UI_CtxMenu(key) DeferLoopChecked(ui_begin_ctx_menu(key), ui_end_ctx_menu())

#endif // UI_BASIC_WIDGETS_H
