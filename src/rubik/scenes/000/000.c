// Camera (editor camera)
RK_NODE_CUSTOM_UPDATE(editor_camera_fn)
{
    Assert(node->type_flags & RK_NodeTypeFlag_Camera3D);
    RK_Transform3D *transform = &node->node3d->transform;

    if(rk_state->sig.f & UI_SignalFlag_LeftDragging)
    {
        Vec2F32 delta = ui_drag_delta();
    }

    Vec3F32 f = {0};
    Vec3F32 s = {0};
    Vec3F32 u = {0};
    rk_ijk_from_matrix(node->fixed_xform, &s, &u, &f);

    typedef struct RK_CameraDragData RK_CameraDragData;
    struct RK_CameraDragData
    {
        RK_Transform3D start_transform;
    };

    if(rk_state->sig.f & UI_SignalFlag_MiddleDragging)
    {
        if(rk_state->sig.f & UI_SignalFlag_MiddlePressed)
        {
            RK_CameraDragData start_transform = {*transform};
            ui_store_drag_struct(&start_transform);
        }
        RK_CameraDragData drag_data = *ui_get_drag_struct(RK_CameraDragData);
        Vec2F32 delta = ui_drag_delta();

        // TODO: how to scale the moving distance
        F32 h_speed_per_screen_px = 4/rk_state->window_dim.x;
        F32 h_pct = -delta.x * h_speed_per_screen_px;
        Vec3F32 h_dist = scale_3f32(s, h_pct);

        F32 v_speed_per_screen_px = 4/rk_state->window_dim.y;
        F32 v_pct = -delta.y * v_speed_per_screen_px;
        Vec3F32 v_dist = scale_3f32(u, v_pct);

        Vec3F32 position = drag_data.start_transform.position;
        position = add_3f32(position, h_dist);
        position = add_3f32(position, v_dist);
        transform->position = position;
    }

    if(rk_state->sig.f & UI_SignalFlag_RightDragging)
    {
        if(rk_state->sig.f & UI_SignalFlag_RightPressed)
        {
            RK_CameraDragData start_transform = {*transform};
            ui_store_drag_struct(&start_transform);
        }
        RK_CameraDragData drag_data = *ui_get_drag_struct(RK_CameraDragData);
        Vec2F32 delta = ui_drag_delta();

        F32 h_turn = -delta.x * (1.f) * (1.f/rk_state->window_dim.x);
        F32 v_turn = -delta.y * (0.5f) * (1.f/rk_state->window_dim.y);

        QuatF32 rotation = drag_data.start_transform.rotation;
        QuatF32 h_q = make_rotate_quat_f32(v3f32(0,-1,0), h_turn);
        rotation = mul_quat_f32(h_q, rotation);

        Vec3F32 side = mul_quat_f32_v3f32(rotation, v3f32(1,0,0));
        QuatF32 v_q = make_rotate_quat_f32(side, v_turn);
        rotation = mul_quat_f32(v_q, rotation);

        transform->rotation = rotation;
    }

    // Scroll
    if(rk_state->sig.scroll.x != 0 || rk_state->sig.scroll.y != 0)
    {
        Vec3F32 dist = scale_3f32(f, rk_state->sig.scroll.y/3.f);
        transform->position = add_3f32(dist, transform->position);
    }
}

// default editor scene
internal RK_Scene *
rk_scene_000(void)
{
    RK_Scene *ret = rk_scene_alloc(str8_lit("default"), str8_lit("./src/rubik/scenes/000/default.rscn"));
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
