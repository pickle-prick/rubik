#ifndef GAME_UI_H
#define GAME_UI_H

////////////////////////////////
//~ k: Floating/Fixed Panes

internal UI_Box    *g_ui_pane_begin(Rng2F32 rect, B32 *open, String8 string);
internal UI_Signal g_ui_pane_end(void);

////////////////////////////////
//~ k: Macro Loop Wrappers

#define G_UI_Pane(r, o, s) DeferLoop(g_ui_pane_begin(r, o, s), g_ui_pane_end())

#endif
