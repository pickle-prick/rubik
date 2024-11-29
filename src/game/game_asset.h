#ifndef GAME_ASSERT_H
#define GAME_ASSERT_H

/////////////////////////////////
//~ Scene
internal G_Scene *g_default_scene();

/////////////////////////////////
//- Scene serialization/deserialization

internal G_Scene *g_scene_from_file(String8 path);
internal void    g_scene_to_file(G_Scene *scene, String8 path);

/////////////////////////////////
// GLTF2.0

internal G_Model*                 g_model_from_gltf(Arena *arena, String8 path);
internal G_Model*                 g_model_from_gltf_cached(String8 gltf_path);
internal G_Key                    g_key_from_gltf_node(cgltf_node *cn);
internal G_ModelNode*             g_mnode_from_gltf_node(Arena *arena, cgltf_node *cn, G_ModelNode *parent, G_ModelNode_HashSlot *hash_table, U64 hash_table_size);
internal G_MeshSkeletonAnimation* g_skeleton_anim_from_gltf_animation(cgltf_animation *cgltf_anim);
internal G_Node*                  g_node_from_model(G_Model *model, G_Key seed_key);

/////////////////////////////////
// Mesh primitives

internal void g_mesh_primitive_box(Arena *arena, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void g_mesh_primitive_plane(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, Vec2F32 size, U64 subdivide_w, U64 subdivide_d);
internal void g_mesh_primitive_sphere(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere);
internal void g_mesh_primitive_cylinder(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom);
internal void g_mesh_primitive_capsule(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, F32 radius, F32 height, U64 radial_segments, U64 rings);

/////////////////////////////////
// Node building helpers

internal G_Node* g_box_node(String8 string, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d);
internal G_Node* g_box_node_cached(String8 string, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d);
internal G_Node* g_plane_node(String8 string, Vec2F32 size, U64 subdivide_w, U64 subdivide_d);
internal G_Node* g_plane_node_cached(String8 string, Vec2F32 size, U64 subdivide_w, U64 subdivide_d);
internal G_Node* g_sphere_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere);
internal G_Node* g_sphere_node_cached(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere);
internal G_Node* g_cylinder_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom);
internal G_Node* g_cylinder_node_cached(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom);
internal G_Node* g_capsule_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings);
internal G_Node* g_capsule_node_cached(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings);

#define g_box_node_default(s) g_box_node_cached((s), v3f32(1,1,1), 2,2,2)
#define g_plane_node_default(s) g_plane_node_cached((s), v2f32(1,1), 1, 1)
#define g_sphere_node_default(s) g_sphere_node_cached((s), 0.5, 1, 30, 8, 0)
#define g_cylinder_node_default(s) g_cylinder_node_cached((s), 0.5, 1.0, 30, 8, 1, 1)
#define g_capsule_node_default(s) g_capsule_node_cached((s), 0.5, 2.5, 30, 8)

#endif
