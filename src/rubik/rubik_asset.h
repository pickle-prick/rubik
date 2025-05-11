#ifndef RUBIK_ASSERT_H
#define RUBIK_ASSERT_H

/////////////////////////////////
// Scene serialization/deserialization

internal String8 rk_scene_to_tscn(RK_Scene *scene);

/////////////////////////////////
//~ GLTF2.0

internal RK_Handle rk_tex2d_from_gltf_tex(cgltf_texture *tex_src, String8 gltf_directory);
internal RK_Node*  rk_node_from_gltf_node(cgltf_node *cn, cgltf_data *data, RK_Key seed_key, String8 gltf_directory);
internal RK_Handle rk_packed_scene_from_gltf(String8 path);
internal RK_Handle rk_animation_from_gltf_animation(cgltf_data *data, cgltf_animation *animation_src ,RK_Key seed);

/////////////////////////////////
//~ Other resources

internal RK_Handle  rk_tex2d_from_path(String8 path);
internal RK_Handle* rk_tex2d_from_dir(Arena *arena, String8 dir, U64 *count);
internal RK_Handle  rk_spritesheet_from_image(String8 path, String8 meta_path);
internal RK_Handle  rk_material_from_color(String8 name, Vec4F32 color);
internal RK_Handle  rk_material_from_image(String8 name, String8 path);

//~ Node building helper

internal RK_Node* rk_node_from_packed_scene_node(RK_Node *node, RK_Key seed_key);
internal RK_Node* rk_node_from_packed_scene(String8 string, RK_PackedScene *packed);

/////////////////////////////////
//~ Mesh primitives

internal void rk_mesh_primitive_box(Arena *arena, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_plane(Arena *arena, Vec2F32 size, U64 subdivide_w, U64 subdivide_d, B32 both_face, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_sphere(Arena *arena, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_cylinder(Arena *arena, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_cone(Arena *arena, F32 radius, F32 height, U64 radial_segments, B32 cap_bottom, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_capsule(Arena *arena, F32 radius, F32 height, U64 radial_segments, U64 rings, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_torus(Arena *arena, F32 inner_radius, F32 outer_radius, U64 rings, U64 ring_segments, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_circle_line(Arena *arena, F32 radius, U64 segments, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);
internal void rk_mesh_primitive_arc_filled(Arena *arena, F32 radius, F32 pct, U64 segments, B32 both_face, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out);

//~ Node building helper

internal void rk_node_equip_box_node(RK_Node *node, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d);
internal void rk_node_equip_plane(RK_Node *node, Vec2F32 size, U64 subdivide_w, U64 subdivide_d);
internal void rk_node_equip_sphere(RK_Node *node, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere);
internal void rk_node_equip_cylinder(RK_Node *node, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom);
internal void rk_node_equip_capsule(RK_Node *node, F32 radius, F32 height, U64 radial_segments, U64 rings);
internal void rk_node_equip_torus(RK_Node *node, F32 inner_radius, F32 outer_radius, U64 rings, U64 ring_segments);

//~ Drawlist building helper

internal RK_DrawNode* rk_drawlist_push_rect(Arena *arena, RK_DrawList *drawlist, Rng2F32 dst, Rng2F32 src);
internal RK_DrawNode* rk_drawlist_push_string(Arena *arena, RK_DrawList *drawlist, Rng2F32 dst, String8 string, D_FancyRunList *fancy_run_list, F_Tag font, F32 font_size, U64 tab_size, F_RasterFlags text_raster_flags);
internal RK_DrawNode* rk_drawlist_push_plane(Arena *arena, RK_DrawList *drawlist, Vec2F32 size, B32 both_face);
internal RK_DrawNode* rk_drawlist_push_box(Arena *arena, RK_DrawList *drawlist, Vec3F32 size);
internal RK_DrawNode* rk_drawlist_push_sphere(Arena *arena, RK_DrawList *drawlist, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere);
internal RK_DrawNode* rk_drawlist_push_cone(Arena *arena, RK_DrawList *drawlist, F32 radius, F32 height, U64 radial_segments);
internal RK_DrawNode* rk_drawlist_push_line(Arena *arena, RK_DrawList *drawlist, Vec3F32 start, Vec3F32 end);
internal RK_DrawNode* rk_drawlist_push_circle(Arena *arena, RK_DrawList *drawlist, F32 radius, U64 segments);
internal RK_DrawNode* rk_drawlist_push_arc(Arena *arena, RK_DrawList *drawlist, Vec3F32 origin, Vec3F32 a, Vec3F32 b, U64 segments, B32 both_face);

#endif
