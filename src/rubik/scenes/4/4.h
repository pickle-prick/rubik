#ifndef RUBIK_SCENE_4_H
#define RUBIK_SCENE_4_H

internal Vec2U32   tile_coord_from_world_pos(Vec2F32 pos, Vec2F32 tilemap_pos, Mat2x2F32 mat_inv);
internal Vec2F32   world_pos_from_mouse(Mat4x4F32 proj_view_inv_m);
internal Vec2U32   tile_coord_from_mouse(Mat4x4F32 proj_view_inv_m, Mat2x2F32 mat_inv, Vec2F32 tilemap_origin);
internal RK_Scene* rk_scene_entry__4();

#endif
