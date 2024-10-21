#ifndef GAME_H
#define GAME_H

/////////////////////////////////
// Game logic

internal void g_update_and_render(G_Scene *scene, OS_EventList events, U64 dt, U64 hot_key);

/////////////////////////////////
// Scene build api

// TODO: we may want to serialize scene to some persistent data format, then load it from disk
internal G_Scene *g_scene_load();
internal G_Node  *node_from_gltf(Arena *arena, String8 path, String8 string);
internal G_Node  *node_from_gltf_node(Arena *arena, cgltf_node *n);

/////////////////////////////////
// Entity

/////////////////////////////////
// Physics

internal void g_collision_detection_response_root(G_Node *root);

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

/////////////////////////////////
//- Terminal Functions

#endif
