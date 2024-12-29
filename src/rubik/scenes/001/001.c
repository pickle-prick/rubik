RK_NODE_CUSTOM_UPDATE(go_board)
{
    // spawn stone with mouse
}

internal RK_Scene *
rk_scene_go()
{
    RK_Scene *ret = rk_scene_alloc(str8_lit("go"), str8_lit("./src/rubik/scenes/001/default.rscn"));
    rk_push_node_bucket(ret->node_bucket);
    rk_push_res_bucket(ret->res_bucket);

    RK_Node *root = rk_build_node3d_from_stringf(0, 0, "root");
    Arena *arena = rk_top_node_bucket()->arena_ref;
    RK_Parent_Scope(root)
    {
        RK_Node *box_1 = rk_box_node(str8_lit("box1"), v3f32(1,1,1), 1,1,1);
        RK_Node *box_2 = rk_box_node(str8_lit("box2"), v3f32(1,1,1), 1,1,1);
        box_2->node3d->transform.position.y = -3;

        // create the editor camera
        RK_Node *main_camera = rk_build_camera3d_from_stringf(0, 0, "main_camera");
        {
            main_camera->camera3d->projection = RK_ProjectionKind_Perspective;
            main_camera->camera3d->viewport_shading = RK_ViewportShadingKind_Solid;
            main_camera->camera3d->polygon_mode = R_GeoPolygonKind_Fill;
            main_camera->camera3d->hide_cursor = 0;
            main_camera->camera3d->lock_cursor = 0;
            main_camera->camera3d->show_grid = 1;
            main_camera->camera3d->show_gizmos = 1;
            main_camera->camera3d->is_active = 1;
            main_camera->camera3d->perspective.zn = 0.1;
            main_camera->camera3d->perspective.zf = 200.f;
            main_camera->camera3d->perspective.fov = 0.25f;
            rk_node_push_fn(main_camera, editor_camera_fn, str8_lit("editor_camera"));
            main_camera->node3d->transform.position = v3f32(0,-3,0);
        }
        ret->active_camera = rk_handle_from_node(main_camera);

        rk_push_node_bucket(ret->res_node_bucket);
        rk_push_parent(0);
        RK_Handle dancing_stormtrooper = rk_packed_scene_from_gltf(str8_lit("./models/dancing_stormtrooper/scene.gltf"));
        // RK_Handle blackguard = rk_packed_scene_from_gltf(str8_lit("./models/blackguard/scene.gltf"));
        // RK_Handle droide = rk_packed_scene_from_gltf(str8_lit("./models/free_droide_de_seguridad_k-2so_by_oscar_creativo/scene.gltf"));

        rk_pop_parent();
        rk_pop_node_bucket();

        // RK_Node *n1 = rk_node_from_packed_scene(str8_lit("1"), droide);
        // {
        //     // flip y
        //     n1->node3d->transform.rotation = mul_quat_f32(make_rotate_quat_f32(v3f32(1,0,0), 0.5f), n1->node3d->transform.rotation);
        // }

        // RK_Node *n2 = rk_node_from_packed_scene(str8_lit("2"), blackguard);
        // {
        //     // flip y
        //     n2->node3d->transform.rotation = mul_quat_f32(make_rotate_quat_f32(v3f32(1,0,0), 0.5f), n2->node3d->transform.rotation);
        //     n2->node3d->transform.position = v3f32(3,0,0);
        // }

        RK_Node *n3 = rk_node_from_packed_scene(str8_lit("3"), dancing_stormtrooper);
        {
            // flip y
            n3->node3d->transform.rotation = mul_quat_f32(make_rotate_quat_f32(v3f32(1,0,0), 0.5f), n3->node3d->transform.rotation);
            n3->node3d->transform.position = v3f32(6,0,0);
        }
    }

    ret->root = rk_handle_from_node(root);
    rk_pop_node_bucket();
    rk_pop_res_bucket();
    return ret;
}
