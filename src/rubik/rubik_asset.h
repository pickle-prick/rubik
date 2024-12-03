#ifndef RUBIK_ASSERT_H
#define RUBIK_ASSERT_H

/////////////////////////////////
//~ Scene
internal RK_Scene *rk_default_scene();

/////////////////////////////////
//- Scene serialization/deserialization

internal RK_Scene *rk_scene_from_file(String8 path);
internal void    rk_scene_to_file(RK_Scene *scene, String8 path);

/////////////////////////////////
// GLTF2.0

internal RK_Model*                 rk_model_from_gltf(Arena *arena, String8 path);
internal RK_Model*                 rk_model_from_gltf_cached(String8 gltf_path);
internal RK_Key                    rk_key_from_gltf_node(cgltf_node *cn);
internal RK_ModelNode*             rk_mnode_from_gltf_node(Arena *arena, cgltf_node *cn, RK_ModelNode *parent, RK_ModelNode_HashSlot *hash_table, U64 hash_table_size);
internal RK_MeshSkeletonAnimation* rk_skeleton_anim_from_gltf_animation(cgltf_animation *cgltf_anim);
internal RK_Node*                  rk_node_from_model(RK_Model *model, RK_Key seed_key);

/////////////////////////////////
// Mesh primitives

internal void rk_mesh_primitive_box(Arena *arena, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_plane(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, Vec2F32 size, U64 subdivide_w, U64 subdivide_d);
internal void rk_mesh_primitive_sphere(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere);
internal void rk_mesh_primitive_cylinder(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom);
internal void rk_mesh_primitive_capsule(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, F32 radius, F32 height, U64 radial_segments, U64 rings);

/////////////////////////////////
// Node building helpers

internal RK_Node* rk_box_node(String8 string, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d);
internal RK_Node* rk_box_node_cached(String8 string, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d);
internal RK_Node* rk_plane_node(String8 string, Vec2F32 size, U64 subdivide_w, U64 subdivide_d);
internal RK_Node* rk_plane_node_cached(String8 string, Vec2F32 size, U64 subdivide_w, U64 subdivide_d);
internal RK_Node* rk_sphere_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere);
internal RK_Node* rk_sphere_node_cached(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere);
internal RK_Node* rk_cylinder_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom);
internal RK_Node* rk_cylinder_node_cached(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom);
internal RK_Node* rk_capsule_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings);
internal RK_Node* rk_capsule_node_cached(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings);

#define rk_box_node_default(s) rk_box_node_cached((s), v3f32(1,1,1), 2,2,2)
#define rk_plane_node_default(s) rk_plane_node_cached((s), v2f32(1,1), 1, 1)
#define rk_sphere_node_default(s) rk_sphere_node_cached((s), 0.5, 1, 30, 8, 0)
#define rk_cylinder_node_default(s) rk_cylinder_node_cached((s), 0.5, 1.0, 30, 8, 1, 1)
#define rk_capsule_node_default(s) rk_capsule_node_cached((s), 0.5, 2.5, 30, 8)

#endif
