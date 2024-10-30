#ifndef GAME_H
#define GAME_H

/////////////////////////////////
// Game logic

internal void g_update_and_render(G_Scene *scene, OS_EventList events, U64 dt, U64 hot_key);

/////////////////////////////////
//~ Scripting

G_NODE_CUSTOM_UPDATE(camera_fn);

/////////////////////////////////
// Game ui
#define g_kv(k, fmt, ...)                \
    do {                                 \
        UI_Row {                         \
            ui_label(str8_lit(k));       \
            ui_spacer(ui_pct(1.0, 0.0)); \
            ui_labelf(fmt, __VA_ARGS__); \
        }                                \
    } while(0)
#endif
