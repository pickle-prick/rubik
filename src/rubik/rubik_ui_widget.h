#ifndef GAME_UI_H
#define GAME_UI_H

////////////////////////////////
//~ k: Basic widget

internal UI_Signal rk_icon_button(RK_IconKind kind, String8 string);
internal UI_Signal rk_icon_buttonf(RK_IconKind kind, char *fmt, ...);

////////////////////////////////
//~ k: Floating/Fixed Panes

// floating pane
internal UI_Box*   rk_ui_pane_begin(Rng2F32 *rect, B32 *open, String8 string);
internal UI_Signal rk_ui_pane_end(void);
internal UI_Box*   rk_ui_dropdown_begin(String8 string);
internal UI_Signal rk_ui_dropdown_end(void);
internal void      rk_ui_dropdown_hide(void);
internal UI_Signal rk_ui_checkbox(B32 *b);
internal UI_Box*   rk_ui_tab_begin(String8 string, B32 *open, UI_Size padding_x, UI_Size padding_y);
internal UI_Signal rk_ui_tab_end();
internal UI_Box*   rk_ui_img(String8 string, Vec2F32 size, R_Handle albedo_tex, Vec2F32 img_size);

////////////////////////////////
//~ k: Macro Loop Wrappers

#define RK_UI_Pane(r,o,s)    DeferLoop(rk_ui_pane_begin((r), (o), (s)), rk_ui_pane_end())
#define RK_UI_Tab(s,o,px,py) DeferLoop(rk_ui_tab_begin(s, o, px, py), rk_ui_tab_end())

////////////////////////////////
//- k: UI Building Helpers

#define RK_Palette(code) UI_Palette(rk_palette_from_code(code))
#define RK_Font(slot) UI_Font(rk_font_from_slot(slot))

#endif
