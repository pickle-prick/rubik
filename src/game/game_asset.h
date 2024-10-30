#ifndef GAME_ASSERT_H
#define GAME_ASSERT_H

/////////////////////////////////
// Scene build api

internal G_Scene                 *g_scene_load();
internal G_Model                 *g_model_from_gltf(Arena *arena, String8 path);
internal G_Key                   g_key_from_gltf_node(cgltf_node *cn);
internal G_ModelNode             *g_mnode_from_gltf_node(Arena *arena, cgltf_node *cn, G_ModelNode *parent, G_ModelNode_HashSlot *hash_table, U64 hash_table_size);
internal G_MeshSkeletonAnimation *g_skeleton_anim_from_gltf_animation(cgltf_animation *cgltf_anim);
internal G_Node                  *g_node_from_model(G_Model *model);

#endif
