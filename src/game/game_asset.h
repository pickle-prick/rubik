#ifndef GAME_ASSERT_H
#define GAME_ASSERT_H

/////////////////////////////////
// GLTF2.0

internal G_Model                 *g_model_from_gltf(Arena *arena, String8 path);
internal G_Key                   g_key_from_gltf_node(cgltf_node *cn);
internal G_ModelNode             *g_mnode_from_gltf_node(Arena *arena, cgltf_node *cn, G_ModelNode *parent, G_ModelNode_HashSlot *hash_table, U64 hash_table_size);
internal G_MeshSkeletonAnimation *g_skeleton_anim_from_gltf_animation(cgltf_animation *cgltf_anim);
internal G_Node                  *g_node_from_model(G_Model *model);

/////////////////////////////////
// Mesh primitives

internal void g_mesh_primitive_box(Arena *arena, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void g_mesh_primitive_sphere(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere);
internal void g_mesh_primitive_cylinder(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom);
internal void g_mesh_primitive_capsule(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, F32 radius, F32 height, U64 radial_segments, U64 rings);

#endif
