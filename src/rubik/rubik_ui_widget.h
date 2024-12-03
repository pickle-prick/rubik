#ifndef GAME_UI_H
#define GAME_UI_H

////////////////////////////////
//~ k: Floating/Fixed Panes

internal UI_Box*   rk_ui_pane_begin(Rng2F32 rect, B32 *open, String8 string);
internal UI_Signal rk_ui_pane_end(void);
internal UI_Box*   rk_ui_dropdown_begin(String8 string);
internal UI_Signal rk_ui_dropdown_end(void);
internal void      rk_ui_dropdown_hide(void);

////////////////////////////////
//~ k: Macro Loop Wrappers

#define RK_UI_Pane(r, o, s) DeferLoop(rk_ui_pane_begin(r, o, s), rk_ui_pane_end())

#endif
