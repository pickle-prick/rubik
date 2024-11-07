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

    // Unpack camera
    G_Node *camera = scene->camera;

    // G_Node *hot_node = 0;
    G_Node *active_node = g_node_from_key(scene->bucket, g_state->active_key);

    // UI Box for game viewport (Handle user interaction)
    ui_set_next_rect(window_rect);
    ui_set_next_child_layout_axis(Axis2_X);
    UI_Box *overlay = ui_build_box_from_string(0, str8_lit("###game_overlay"));
    ui_push_parent(overlay);

    // TODO: move these cfg to some structure
    local_persist B32 show_scene_cfg  = 1;
    local_persist B32 show_camera_cfg = 1;
    local_persist B32 show_node_cfg   = 1;

    local_persist TxtPt cursor         = {0};
    local_persist TxtPt mark           = {0};
    local_persist U8 edit_buffer[30]   = {0};
    local_persist U8 edit_buffer_size  = 30;
    local_persist U64 edit_string_size = 0;

    ui_set_next_focus_hot(UI_FocusKind_Root);
    ui_set_next_focus_active(UI_FocusKind_Root);
    ui_set_next_flags(UI_BoxFlag_Clip);
    UI_Pane(r2f32p(0, 0, 610, window_rect.p1.y), str8_lit("###left_pane"))
    {
        // Scene
        ui_set_next_pref_size(Axis2_Y, ui_children_sum(1.0));
        ui_set_next_child_layout_axis(Axis2_Y);
        ui_set_next_flags(UI_BoxFlag_DrawBorder);
        UI_Box *scene_tree_box = ui_build_box_from_stringf(0, "###scene_tree");
        UI_Parent(scene_tree_box)
        {
            // Header
            ui_set_next_child_layout_axis(Axis2_X);
            ui_set_next_flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow);
            UI_Box *header_box = ui_build_box_from_stringf(0, "###header");
            UI_Parent(header_box)
            {
                ui_set_next_pref_size(Axis2_X, ui_px(39,0.0));
                if(ui_clicked(ui_expanderf(show_scene_cfg, "###scene_cfg")))
                {
                    show_scene_cfg = !show_scene_cfg;
                }
                ui_set_next_pref_size(Axis2_X, ui_text_dim(3, 0.0));
                ui_labelf("Scene");
            }

            // Content container
            F32 size = 900;
            if(!show_scene_cfg) size = 0;
            ui_set_next_pref_size(Axis2_Y, ui_px(size, 0.0));
            ui_set_next_child_layout_axis(Axis2_Y);
            UI_Box *container_box = ui_build_box_from_stringf(0, "###container");
            UI_Parent(container_box)
            {
                Vec2F32 dim = dim_2f32(container_box->rect);
                // TODO: no usage for now
                UI_ScrollPt pt = {0};
                ui_scroll_list_begin(v2f32(container_box->fixed_size.x, dim.y), &pt);
                G_Node *root = scene->root;
                while(root != 0)
                {
                    G_NodeRec rec = g_node_df_post(root, 0);
                    if(ui_clicked(ui_button(root->name)))
                    {
                        g_state->active_key = root->key;
                    }
                    root = rec.next;
                }
                ui_scroll_list_end();
            }
        }

        ui_spacer(ui_px(30, 1.0));

        // Camera
        ui_set_next_pref_size(Axis2_Y, ui_children_sum(1.0));
        ui_set_next_child_layout_axis(Axis2_Y);
        ui_set_next_flags(UI_BoxFlag_DrawBorder);
        UI_Box *camera_cfg_box = ui_build_box_from_stringf(0, "###camera_cfg");
        UI_Parent(camera_cfg_box)
        {
            // Header
            ui_set_next_child_layout_axis(Axis2_X);
            ui_set_next_flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow);
            UI_Box *header_box = ui_build_box_from_stringf(0, "###header");
            UI_Parent(header_box)
            {
                ui_set_next_pref_size(Axis2_X, ui_px(39,0.0));
                if(ui_clicked(ui_expanderf(show_camera_cfg, "###camera_cfg")))
                {
                    show_camera_cfg = !show_camera_cfg;
                }
                ui_labelf("Camera");
            }

            if(show_camera_cfg)
            {
                UI_Row
                {
                    ui_labelf("pos");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&camera->pos.x, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("X###pos_x"));
                    ui_f32_edit(&camera->pos.y, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("Y###pos_y"));
                    ui_f32_edit(&camera->pos.z, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("Z###pos_z"));
                }
                UI_Row
                {
                    ui_labelf("scale");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&camera->scale.x, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("X###scale_x"));
                    ui_f32_edit(&camera->scale.y, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("Y###scale_y"));
                    ui_f32_edit(&camera->scale.z, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("Z###scale_z"));
                }
                UI_Row
                {
                    ui_labelf("rot");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&camera->rot.x, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("X###rot_x"));
                    ui_f32_edit(&camera->rot.y, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("Y###rot_y"));
                    ui_f32_edit(&camera->rot.z, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("Z###rot_z"));
                    ui_f32_edit(&camera->rot.w, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("W###rot_w"));
                }
            }
        }

        ui_spacer(ui_px(30, 1.0));

        // Node Properties
        ui_set_next_pref_size(Axis2_Y, ui_children_sum(1.0));
        ui_set_next_child_layout_axis(Axis2_Y);
        UI_Box *node_cfg_box = ui_build_box_from_stringf(0, "###node_cfg");
        UI_Parent(node_cfg_box)
        {
            // Header
            ui_set_next_child_layout_axis(Axis2_X);
            ui_set_next_flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow);
            UI_Box *header_box = ui_build_box_from_stringf(0, "###header");
            UI_Parent(header_box)
            {
                ui_set_next_pref_size(Axis2_X, ui_px(39,0.0));
                if(ui_clicked(ui_expander(show_node_cfg, str8_lit("###node_cfg"))))
                {
                    show_node_cfg = !show_node_cfg;
                }
                ui_labelf("Node Properties");
            }

            if(show_node_cfg && active_node != 0)
            {
                UI_Row
                {
                    ui_labelf("name");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_labelf("%s", active_node->name.str);
                }
                UI_Row
                {
                    ui_labelf("pos");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&active_node->pos.x, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("X###pos_x"));
                    ui_f32_edit(&active_node->pos.y, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("Y###pos_y"));
                    ui_f32_edit(&active_node->pos.z, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("Z###pos_z"));
                }
                UI_Row
                {
                    ui_labelf("scale");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&active_node->scale.x, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("X###scale_x"));
                    ui_f32_edit(&active_node->scale.y, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("Y###scale_y"));
                    ui_f32_edit(&active_node->scale.z, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("Z###scale_z"));
                }
                UI_Row
                {
                    ui_labelf("rot");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&active_node->rot.x, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("X###rot_x"));
                    ui_f32_edit(&active_node->rot.y, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("Y###rot_y"));
                    ui_f32_edit(&active_node->rot.z, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("Z###rot_z"));
                    ui_f32_edit(&active_node->rot.w, -100, 100, &cursor, &mark, edit_buffer, edit_buffer_size, &edit_string_size, str8_lit("W###rot_w"));
                }
            }
        }
    }

    ui_set_next_pref_width(ui_pct(1.0, 0.0));
    ui_set_next_pref_height(ui_pct(1.0, 0.0));
    UI_Box *scene_overlay = ui_build_box_from_stringf(UI_BoxFlag_MouseClickable|UI_BoxFlag_Scroll, "###scene_overlay");
    ui_pop_parent();

    g_state->sig = ui_signal_from_box(scene_overlay);
    F32 dt_sec = dt/1000000.0f;

    B32 show_grid   = 1;
    B32 show_gizmos = 0;
    // TODO: this is kind awkward, fix it later, make it pretty
    d_push_bucket(g_state->bucket_geo3d);
    R_PassParams_Geo3D *pass = d_geo3d_begin(scene_overlay->rect, mat_4x4f32(1.f), mat_4x4f32(1.f),
                                             show_grid, show_gizmos,
                                             mat_4x4f32(1.f), v3f32(0,0,0));
    d_pop_bucket();

    // Update/draw node in the scene tree
    {
        G_Node *node = scene->root;
        while(node != 0)
        {
            G_NodeRec rec = g_node_df_pre(node, 0);
            F32 dt_ms = dt / 1000.f;
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
            node = rec.next;
        }
    }

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

    ui_pop_parent();
    g_state->sig = ui_signal_from_box(overlay);
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
    }
    else if(g_state->sig.f & UI_SignalFlag_MiddleReleased)
    {
        // Commit pos delta
        g_node_delta_commit(node);
    }

    if(g_state->sig.f & UI_SignalFlag_RightDragging)
    {
        Vec2F32 delta = ui_drag_delta();

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
