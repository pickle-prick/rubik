#ifndef RUBIK_ASSERT_H
#define RUBIK_ASSERT_H

/////////////////////////////////
// Scene serialization/deserialization

// internal RK_Scene*       rk_scene_from_rscn(String8 path);
// internal void            rk_scene_to_rscn(RK_Scene *scene, String8 path);
// internal RK_PackedScene* rk_packed_scene_from_rscn(String8 path);

/////////////////////////////////
//~ GLTF2.0

internal RK_Handle rk_tex_from_gltf_tex(cgltf_texture *tex_src, cgltf_data *data, String8 gltf_directory, RK_Key seed_key);
internal RK_Node*  rk_node_from_gltf_node(cgltf_node *cn, cgltf_data *data, RK_Key seed_key, String8 gltf_directory);
internal RK_Handle rk_packed_scene_from_gltf(String8 path);
internal RK_Handle rk_animation_from_gltf_animation(cgltf_data *data, cgltf_animation *animation_src ,RK_Key seed);

//~ Node building helper

internal RK_Node* rk_node_from_packed_scene_node(RK_Node *node, RK_Key seed_key);
internal RK_Node* rk_node_from_packed_scene(String8 string, RK_Handle packed_scene);

/////////////////////////////////
//~ Mesh primitives

internal void rk_mesh_primitive_box(Arena *arena, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_plane(Arena *arena, Vec2F32 size, U64 subdivide_w, U64 subdivide_d, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_sphere(Arena *arena, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_cylinder(Arena *arena, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_capsule(Arena *arena, F32 radius, F32 height, U64 radial_segments, U64 rings, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);

//~ Node building helper

internal RK_Node* rk_box_node(String8 string, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d);
internal RK_Node* rk_plane_node(String8 string, Vec2F32 size, U64 subdivide_w, U64 subdivide_d);
internal RK_Node* rk_sphere_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere);
internal RK_Node* rk_cylinder_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom);
internal RK_Node* rk_capsule_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings);

#endif
