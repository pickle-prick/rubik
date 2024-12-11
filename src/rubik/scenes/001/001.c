RK_NODE_CUSTOM_UPDATE(go_board)
{
    // spawn stone with mouse
}

internal RK_Scene *
rk_scene_go()
{
    RK_Scene *ret = rk_scene_alloc();

    // fill base info
    {
        ret->name             = str8_lit("go");
        ret->path             = str8_lit("./src/rubik/scenes/001/go.scene");
        ret->viewport_shading = RK_ViewportShadingKind_Material;
        ret->global_light     = v3f32(0,0,1);
        ret->polygon_mode     = R_GeoPolygonKind_Fill;
    }

    rk_push_bucket(ret->bucket);

    RK_Node *root = rk_build_node_from_stringf(0, "root");
    ret->root = root;
    rk_push_parent(root);

    // editor camera
    RK_Node *editor_camera = rk_camera_perspective(0,0,1,1, 0.25f, 0.1f, 200.f, str8_lit("editor_camera"));
    ret->active_camera = rk_scene_camera_push(ret, editor_camera);
    {
        RK_FunctionNode *fn = rk_function_from_string(str8_lit("editor_camera_fn"));
        rk_node_push_fn(ret->bucket->arena, editor_camera, fn->ptr, fn->alias);
    }

    // spawn a orthographic camera
    RK_Node *main_camera = rk_camera_orthographic(0,0,1,0, -4.f, 4.f, -4.f, 4.f, 0.1f, 200.f, str8_lit("main_camera"));
    rk_scene_camera_push(ret, main_camera);

    // grid table (go table 19x19)
    RK_Node *grid = 0;
    RK_MeshCacheTable_Scope(&ret->mesh_cache_table)
    {
        grid = rk_box_node_cached(str8_lit("grid"), v3f32(19,19,1), 18,18,0);
    }
    rk_node_push_fn(ret->arena, grid, go_board, str8_lit("go_board"));

    // stone
    RK_Parent_Scope(grid) RK_MeshCacheTable_Scope(&ret->mesh_cache_table) for(U64 i = 0; i < 3; i++)
    {
        for(U64 j = 0; j < 3; j++)
        {
            String8 string = push_str8f(ret->bucket->arena, "%d-%d", i, j);
            RK_Node *stone = rk_cylinder_node_cached(string, 0.5, 0.2, 6, 6, 1, 1);
            stone->rot = make_rotate_quat_f32(v3f32(1,0,0), 0.25);
            stone->pos = v3f32(i,0,j);
        }
    }

    rk_pop_parent();
    rk_pop_bucket();
    return ret;
}
