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
    rk_scene_camera_push(ret, editor_camera);
    {
        RK_FunctionNode *fn = rk_function_from_string(str8_lit("editor_camera_fn"));
        rk_node_push_fn(ret->bucket->arena, editor_camera, fn->ptr, fn->alias);
    }

    // spawn a orthographic camera
    RK_Node *main_camera = rk_camera_orthographic(0,0,1,0, -4.f, 4.f, -4.f, 4.f, 0.1f, 200.f, str8_lit("main_camera"));
    rk_scene_camera_push(ret, main_camera);

    // grid table
    {

    }
        
    rk_pop_parent();
    rk_pop_bucket();
    return ret;
}
