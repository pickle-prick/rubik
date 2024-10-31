internal void
g_update_and_render(G_Scene *scene, OS_EventList os_events, U64 dt, U64 hot_key)
{
    arena_clear(g_state->frame_arena);
    Rng2F32 window_rect = os_client_rect_from_window(g_state->os_wnd);
    Vec2F32 window_dim = dim_2f32(window_rect);

    // Remake bucket every frame
    g_state->bucket_rect = d_bucket_make();
    g_state->bucket_geo3d = d_bucket_make();
    g_state->hot_key = (G_Key){ hot_key };

    // UI Box for game viewport (Handle user interaction)
    ui_push_rect(window_rect);
    UI_Box *box = ui_build_box_from_string(UI_BoxFlag_MouseClickable|
                                            UI_BoxFlag_ClickToFocus|
                                            UI_BoxFlag_Scroll,
                                            str8_lit("###game_overlay"));
    ui_push_parent(box);
    ui_pop_rect();

    // G_Node *hot_node = 0;
    G_Node *active_node = 0;

    // Stats pane

    UI_CornerRadius(9) ui_pane_begin(r2f32p(10, 700, 610, 1400), str8_lit("game"));
    ui_column_begin();
    F32 dt_sec = dt/1000000.0f;

    B32 show_grid   = 1;
    B32 show_gizmos = 0;
    // TODO: this is kind awkward, fix it later, make it pretty
    d_push_bucket(g_state->bucket_geo3d);
    R_PassParams_Geo3D *pass = d_geo3d_begin(window_rect, mat_4x4f32(1.f), mat_4x4f32(1.f),
                                             show_grid, show_gizmos,
                                             mat_4x4f32(1.f), v3f32(0,0,0));
    d_pop_bucket();

    // Update/draw node in the scene tree
    {
        G_Node *node = scene->root;
        while(node != 0)
        {
            F32 dt_ms = dt / 1000.f;
            if(g_key_match(g_state->active_key, node->key)) { active_node = node; }

            for(G_UpdateFnNode *fn = node->first_update_fn; fn != 0; fn = fn->next)
            {
                fn->f(node, scene, os_events, dt_sec);
            }

            D_BucketScope(g_state->bucket_geo3d)
            {
                switch(node->kind)
                {
                    default: {}break;
                    case G_NodeKind_Mesh:
                    {
                        Mat4x4F32 *joint_xforms = node->parent->v.mesh_grp.joint_xforms;
                        U64 joint_count = node->parent->v.mesh_grp.joint_count;
                        R_Mesh3DInst *inst = d_mesh(node->v.mesh.vertices, node->v.mesh.indices,
                                                    R_GeoTopologyKind_Triangles,
                                                    R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB, node->v.mesh.albedo_tex,
                                                    joint_xforms, joint_count,
                                                    mat_4x4f32(1.f), node->key.u64[0]);
                        inst->xform = node->fixed_xform;
                    }break;
                }
            }
            node = g_node_df_pre(node, 0);
        }
    }

    UI_Row
    {
        ui_label(str8_lit("test_buttonl"));
        ui_spacer(ui_pct(1.0, 0.0));
        ui_button(str8_lit("test button"));
    }

    // Unpack camera
    G_Node *camera = scene->camera;
    pass->view = g_view_from_node(camera);
    pass->projection = make_perspective_vulkan_4x4f32(camera->v.camera.fov, window_dim.x/window_dim.y, camera->v.camera.zn, camera->v.camera.zf);
    Mat4x4F32 xform_m = mul_4x4f32(pass->projection, pass->view);

    if(active_node != 0) 
    {
        pass->show_gizmos = 1;
        pass->gizmos_xform = active_node->fixed_xform;
        pass->gizmos_origin = v4f32(pass->gizmos_xform.v[3][0],
                                    pass->gizmos_xform.v[3][1],
                                    pass->gizmos_xform.v[3][2],
                                    1);
        Vec3F32 pos = g_pos_from_node(active_node);
        g_kv("active node", "%s", active_node->name.str);
        g_kv("active node pos", "%.2f %.2f %.2f", pos.x, pos.y, pos.z);
    }

    // Update hot/active
    {
        if(g_state->sig.f & UI_SignalFlag_LeftReleased)
        {
            g_state->is_dragging = 0;
        }

        if((g_state->sig.f & UI_SignalFlag_LeftReleased) && active_node != 0)
        {
            g_node_delta_commit(active_node);
        }

        if((g_state->sig.f & UI_SignalFlag_LeftPressed) &&
            g_state->hot_key.u64[0] != G_SpecialKeyKind_GizmosIhat &&
            g_state->hot_key.u64[0] != G_SpecialKeyKind_GizmosJhat &&
            g_state->hot_key.u64[0] != G_SpecialKeyKind_GizmosKhat)
        {
            g_state->active_key = g_state->hot_key;
        }

        if(active_node != 0)
        {
            // Gizmos dragging
            Vec3F32 local_i;
            Vec3F32 local_j;
            Vec3F32 local_k;
            g_local_coord_from_node(active_node, &local_k, &local_i, &local_j);

            // Dragging started
            if((g_state->sig.f & UI_SignalFlag_LeftDragging) &&
                g_state->is_dragging == 0 &&
                (g_state->hot_key.u64[0] == G_SpecialKeyKind_GizmosIhat || g_state->hot_key.u64[0] == G_SpecialKeyKind_GizmosJhat  || g_state->hot_key.u64[0] == G_SpecialKeyKind_GizmosKhat))
            {
                Vec3F32 direction = {0};
                switch(g_state->hot_key.u64[0])
                {
                    default: {InvalidPath;}break;
                    case G_SpecialKeyKind_GizmosIhat: {direction = local_i;}break;
                    case G_SpecialKeyKind_GizmosJhat: {direction = local_j;}break;
                    case G_SpecialKeyKind_GizmosKhat: {direction = local_k;}break;
                }
                g_state->is_dragging = 1;
                g_state->drag_start_direction = direction;
            }

            // Dragging
            if(g_state->is_dragging)
            {
                Vec2F32 delta = ui_drag_delta();
                Vec4F32 dir = v4f32(g_state->drag_start_direction.x,
                                    g_state->drag_start_direction.y,
                                    g_state->drag_start_direction.z,
                                    1.0);
                Vec4F32 projected_start = mat_4x4f32_transform_4f32(xform_m, v4f32(0,0,0,1.0));
                if(projected_start.w != 0)
                {
                    projected_start.x /= projected_start.w;
                    projected_start.y /= projected_start.w;
                }
                Vec4F32 projected_end = mat_4x4f32_transform_4f32(xform_m, dir);
                if(projected_end.w != 0)
                {
                    projected_end.x /= projected_end.w;
                    projected_end.y /= projected_end.w;
                }
                Vec2F32 projected_dir = sub_2f32(v2f32(projected_end.x, projected_end.y), v2f32(projected_start.x, projected_start.y));

                // TODO: negating direction could cause rapid changes on the position, we would want to avoid that 
                S32 n = dot_2f32(delta, projected_dir) > 0 ? 1 : -1;

                // 10.0 in word coordinate per pixel (scaled by the distance to the eye)
                F32 speed = 1.0/(window_dim.x*length_2f32(projected_dir));

                active_node->pst_pos_delta = scale_3f32(g_state->drag_start_direction, length_2f32(delta)*speed*n);
                g_kv("active pos_delta", "%.2f %.2f %.2f", active_node->pst_pos_delta.x, active_node->pst_pos_delta.y, active_node->pst_pos_delta.z);
                g_kv("active pos", "%.2f %.2f %.2f", active_node->pos.x, active_node->pos.y, active_node->pos.z);
            }
        }
    }

    // Process events
    OS_Event *os_evt_first = os_events.first;
    OS_Event *os_evt_opl = os_events.last + 1;
    for(OS_Event *os_evt = os_evt_first; os_evt < os_evt_opl; os_evt++)
    {
        if(os_evt == 0) continue;
        if(os_evt->kind == OS_EventKind_Text && os_evt->key == OS_Key_A) {}
        if(os_evt->kind == OS_EventKind_Text && os_evt->key == OS_Key_D) {}
        if(os_evt->kind == OS_EventKind_Text && os_evt->key == OS_Key_Space) {}
    }

    ui_column_end();
    ui_pane_end();
    ui_pop_parent();
    g_state->sig = ui_signal_from_box(box);
}

/////////////////////////////////
//~ Scripting

// Camera (editor camera)
G_NODE_CUSTOM_UPDATE(camera_fn)
{
    G_Camera3D *camera = &node->v.camera;
    Rng2F32 window_rect = os_client_rect_from_window(g_state->os_wnd);
    Vec2F32 window_dim = dim_2f32(window_rect);

    if(g_state->sig.f & UI_SignalFlag_LeftDragging)
    {
        Vec2F32 delta = ui_drag_delta();
    }

    Vec3F32 f = {0};
    Vec3F32 s = {0};
    Vec3F32 u = {0};
    g_local_coord_from_node(node, &f, &s, &u);

    // TODO: do we really need these?
    Vec3F32 clean_f = f;
    clean_f.y = 0;
    Vec3F32 clean_s = s;
    clean_s.y = 0;
    Vec3F32 clean_u = u;
    clean_u.x = 0;
    clean_u.z = 0;

    g_kv("camera forward", "%.2f %.2f %.2f", f.x, f.y, f.z);
    g_kv("camera side", "%.2f %.2f %.2f", s.x, s.y, s.z);
    g_kv("camera up", "%.2f %.2f %.2f", u.x, u.y, u.z);

    if(g_state->sig.f & UI_SignalFlag_MiddleDragging)
    {
        Vec2F32 delta = ui_drag_delta();
        // Horizontal
        F32 h_pct = delta.x / window_dim.x;
        F32 h_dist = 2.0 * h_pct;
        // Vertical
        F32 v_pct = delta.y / window_dim.y;
        F32 v_dist = -2.0 * v_pct;
        node->pre_pos_delta = scale_3f32(clean_s, -h_dist * 6);
        node->pre_pos_delta = add_3f32(node->pre_pos_delta, scale_3f32(clean_u, v_dist*6));
        g_kv("Middle dragging delta", "%.2f %.2f", delta.x, delta.y);
    }
    else if(g_state->sig.f & UI_SignalFlag_MiddleReleased)
    {
        // Commit pos delta
        g_node_delta_commit(node);
    }

    if(g_state->sig.f & UI_SignalFlag_RightDragging)
    {
        Vec2F32 delta = ui_drag_delta();
        g_kv("Right dragging delta", "%.2f %.2f", delta.x, delta.y);

        // TODO: float percision issue when v_turn_speed is too high, fix it later
        F32 v_turn_speed = 0.5f/(window_dim.y);
        F32 h_turn_speed = 2.0f/(window_dim.x);

        // TODO: we may need to clamp the turns to 0-1
        F32 v_turn = -delta.y * v_turn_speed;
        F32 h_turn = delta.x * h_turn_speed;

        QuatF32 v_quat = make_rotate_quat_f32(s, v_turn);
        QuatF32 h_quat = make_rotate_quat_f32(v3f32(0,1,0), h_turn);
        node->pre_rot_delta = mul_quat_f32(v_quat,h_quat);
    }
    else if(g_state->sig.f & UI_SignalFlag_RightReleased)
    {
        // Commit camera rot_delta
        g_node_delta_commit(node);
    }

    // Scroll
    if(g_state->sig.scroll.x != 0 || g_state->sig.scroll.y != 0)
    {
        Vec3F32 dist = scale_3f32(f, g_state->sig.scroll.y/3.0f);
        node->pos = add_3f32(dist, node->pos);
    }

    if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Left)) { }
    if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Right)) { }
    if(os_key_is_down(OS_Key_Left)) { }
    if(os_key_is_down(OS_Key_Right)) { }
}

// typedef struct G_PlayerData G_PlayerData;
// struct G_PlayerData
// { };

// G_NODE_CUSTOM_UPDATE(player_fn)
// {
//     G_PlayerData *data = (G_PlayerData *)node->custom_data;
//     B32 grounded = 0;
// 
//     F32 h = 230;
//     F32 foot_speed = 180;
//     F32 xh = 60;
//     F32 v0 = (-2*h*foot_speed) / xh;
//     F32 g = (2*h*foot_speed*foot_speed) / (xh*xh);
// 
//     // Jump
//     // if((os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Up) || os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Space) ) && (grounded || data->jump_counter == 1)) {}
// 
//     if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Left))
//     {
//         node->pos.x -= 0.5f;
//     }
//     if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Right))
//     {
//         node->pos.x += 0.5f;
//     }
//     if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Up))
//     {
//         node->pos.z += 0.5f;
//     }
//     if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Down))
//     {
//         node->pos.z -= 0.5f;
//     }
//     if(os_key_is_down(OS_Key_Left)) { }
//     if(os_key_is_down(OS_Key_Right)) { }
// 
//     // Gravity
//     if(!grounded) { } else { }
// }
