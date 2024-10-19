internal void
g_update_and_render(G_Scene *scene, OS_EventList os_events, U64 dt, U64 hot_key)
{
    Rng2F32 window_rect = os_client_rect_from_window(g_state->os_wnd);
    Vec2F32 window_dim = dim_2f32(window_rect);

    // Remake bucket every frame
    g_state->bucket_rect = d_bucket_make();
    g_state->bucket_geo3d = d_bucket_make();
    g_state->hot_key = (G_Key){ hot_key };

    // UI Box for game viewport (Handle user interaction)
    UI_Rect(window_rect) 
    {
        UI_Box *box = ui_build_box_from_string(UI_BoxFlag_MouseClickable|
                                               UI_BoxFlag_ClickToFocus|
                                               UI_BoxFlag_Scroll,
                                               str8_lit("game_overlay"));
        g_state->sig = ui_signal_from_box(box);
    }

    G_Node *hot_node = g_node_from_key(scene->bucket, g_state->hot_key);
    G_Node *active_node = g_node_from_key(scene->bucket, g_state->active_key);

    // Stats pane
    ui_pane_begin(r2f32p(0, 700, 600, 1400), str8_lit("game"));
    ui_column_begin();
    F32 dt_sec = dt/1000000.0f;

    // Unpack camera
    G_Node *camera = scene->camera;
    Vec3F32 camera_pos = g_node_pos(camera);
        
    // TODO: add yaw from camera
    // g_kv("camera_pos", "%.2f %.2f %.2f", camera_pos.x, camera_pos.y, camera_pos.z);
    // g_kv("camera_yaw", "%.2f", camera->camera.yaw+camera->camera.yaw_delta);
    // g_kv("camera_pitch", "%.2f", camera->camera.pitch+camera->camera.pitch_delta);

    g_kv("camera_pos", "%.2g %.2g %.2g", camera_pos.x, camera_pos.y, camera_pos.z);
    QuatF32 quat = mul_quat_f32(camera->camera.rot, camera->camera.rot_delta);
    g_kv("camera_quat", "%.2g %.2g %.2g %.2g", quat.x, quat.y, quat.z, quat.w);
    F32 length = quat.x*quat.x + quat.y*quat.y + quat.z*quat.z + quat.w*quat.w;
    g_kv("camera_quat_length", "%.2g", length);

    Mat4x4F32 view_m = view_from_camera(camera_pos, &camera->camera);
    Mat4x4F32 proj_m = make_perspective_vulkan_4x4f32(camera->camera.fov, window_dim.x/window_dim.y, camera->camera.zn, camera->camera.zf);
    Mat4x4F32 xform_m = mul_4x4f32(proj_m, view_m);

    // Update hot/active
    {
        if(g_state->sig.f & UI_SignalFlag_LeftReleased)
        {
            g_state->is_dragging = 0;
        }

        if((g_state->sig.f & UI_SignalFlag_LeftReleased) && active_node != 0)
        {
            active_node->pos = add_3f32(active_node->pos, active_node->pos_delta);
            MemoryZeroStruct(&active_node->pos_delta);
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
            // Dragging
            if((g_state->sig.f & UI_SignalFlag_LeftDragging) &&
               g_state->is_dragging == 0 &&
               (g_state->hot_key.u64[0] == G_SpecialKeyKind_GizmosIhat ||
               g_state->hot_key.u64[0] == G_SpecialKeyKind_GizmosJhat ||
               g_state->hot_key.u64[0] == G_SpecialKeyKind_GizmosKhat))
            {
                Vec3F32 direction = {0};
                switch(g_state->hot_key.u64[0])
                {
                    default: {InvalidPath;}break;
                    case G_SpecialKeyKind_GizmosIhat: {direction.v[0]=1;}break;
                    case G_SpecialKeyKind_GizmosJhat: {direction.v[1]=1;}break;
                    case G_SpecialKeyKind_GizmosKhat: {direction.v[2]=1;}break;
                }
                g_state->is_dragging = 1;
                g_state->drag_start_direction = direction;
            }

            if(g_state->is_dragging)
            {
                Vec2F32 delta = ui_drag_delta();
                Vec4F32 direction = v4f32(g_state->drag_start_direction.x, g_state->drag_start_direction.y, g_state->drag_start_direction.z, 1.0);
                // TODO: this don't seem quite right, some axis is easier to move
                Vec4F32 projected_start = transform_4x4f32(xform_m, v4f32(0,0,0,1));
                // TODO: not sure if we need to divide by w here, check out later
                projected_start.x /= projected_start.w;
                projected_start.y /= projected_start.w;
                Vec4F32 projected_end = transform_4x4f32(xform_m, direction);
                projected_end.x /= projected_end.w;
                projected_end.y /= projected_end.w;
                Vec4F32 projected_direction = normalize_4f32(sub_4f32(projected_end, projected_start));
                F32 length = dot_2f32(delta, v2f32(projected_direction.x, projected_direction.y));
                // TODO: scale by some kind of fwidth, we want to scale larger when object is further
                F32 scale = 10.0f/window_dim.x;
                length *= scale;
                g_kv("moving length", "%.2f", length);
                g_kv("moving direction", "%.2f %.2f", projected_direction.x, projected_direction.y);
                active_node->pos_delta = scale_3f32(g_state->drag_start_direction, length);
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

    // Physics dynamic pass
    // g_physics_dynamic_root(scene->root, dt_sec);

    // Collision detection and response
    // g_collision_detection_response_root(scene->root);

    // Update every node2d which has a custom update fn
    D_BucketScope(g_state->bucket_geo3d)
    {
        B32 show_grid = 1;
        B32 show_gizmos = 0;
        Mat4x4F32 gizmos_xform = mat_4x4f32(1.0f);
        Vec3F32 gizmos_origin = v3f32(0,0,0);

        if(active_node != 0) 
        {
            Vec3F32 node_pos = g_node_pos(active_node);
            show_gizmos = 1;
            // TODO: change gizmos_coord based on active node's transform(translate, rotation)
            Mat4x4F32 rot_m = mat_4x4f32(1.0f);
            Mat4x4F32 tra_m = make_translate_4x4f32(node_pos);
            gizmos_xform = mul_4x4f32(tra_m, rot_m);
            gizmos_origin = node_pos;
        }
        R_PassParams_Geo3D *pass = d_geo3d_begin(window_rect, view_m, proj_m,
                                                 show_grid, show_gizmos,
                                                 gizmos_xform, gizmos_origin);
        G_Node *node = scene->root;
        while(node != 0)
        {
            // Abs position
            Vec3F32 pos = g_node_pos(node);
            // TODO: do the same thing with scale and rotation
            F32 dt_ms = dt / 1000.f;

            switch(node->kind)
            {
                default: {}break;
                case G_NodeKind_MeshInstance3D:
                {
                    R_Mesh3DInst *inst = d_mesh(node->mesh_inst3d.mesh->vertices, node->mesh_inst3d.mesh->indices,
                                                R_GeoTopologyKind_Triangles,
                                                R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                                                r_handle_zero(), mat_4x4f32(1.f), node->key.u64[0]);
                    // TODO: pos,rot,scale -> xform
                    // inst->xform = make_translate_4x4f32(v3f32(0, 0, +3));
                    Mat4x4F32 translate = make_translate_4x4f32(pos);
                    Mat4x4F32 scale = make_scale_4x4f32(node->scale);
                    // TODO: rotation
                    inst->xform = mul_4x4f32(translate, scale);
                }break;
            }
            if(node->update_fn != 0)
            {
                node->update_fn(node, scene, os_events, dt_sec);
            }
            // Next
            node = g_node_df_pre(node, 0);
        }
    }

    ui_column_end();
    ui_pane_end();
}

/////////////////////////////////
// Camera (main)

G_NODE_CUSTOM_UPDATE(camera_fn)
{
    G_Camera3D *camera = &node->camera;
    Rng2F32 window_rect = os_client_rect_from_window(g_state->os_wnd);
    Vec2F32 window_dim = dim_2f32(window_rect);
    // TODO: remove this
    if(g_state->sig.f & UI_SignalFlag_LeftDragging)
    {
        Vec2F32 drag_delta = ui_drag_delta();
        UI_Row
        {
            ui_label(str8_lit("Left dragging delta"));
            ui_spacer(ui_pct(1.0, 0.0));
            ui_labelf("%.2f %.2f", drag_delta.x, drag_delta.y);
        }
    }

    Vec3F32 f = {0};
    Vec3F32 s = {0};
    Vec3F32 u = {0};
    direction_from_camera(camera, &f, &s, &u);

    // TODO: do we really need these?
    Vec3F32 clean_f = f;
    clean_f.y = 0;
    Vec3F32 clean_s = s;
    clean_s.y = 0;
    Vec3F32 clean_u = u;
    clean_u.x = 0;
    clean_u.z = 0;

    g_kv("camera forward", "%.2f %.2f %.2f", f.x, f.y, f.z);
    g_kv("camera forward length", "%.2f", length_3f32(f));
    g_kv("camera side", "%.2f %.2f %.2f", s.x, s.y, s.z);
    g_kv("camera side length", "%.2f", length_3f32(s));
    g_kv("camera up", "%.2f %.2f %.2f", u.x, u.y, u.z);
    g_kv("camera up length", "%.2f", length_3f32(u));

    if(g_state->sig.f & UI_SignalFlag_MiddleDragging)
    {
        // TODO: still don't think it's right, come back later 
        Vec2F32 delta = ui_drag_delta();
        // Horizontal
        F32 h_pct = delta.x / window_dim.x;
        F32 h_dist = 2.0 * h_pct;
        // Vertical
        F32 v_pct = delta.y / window_dim.y;
        F32 v_dist = 2.0 * v_pct;
        node->pos_delta = scale_3f32(clean_s, -h_dist * 6);
        node->pos_delta = add_3f32(node->pos_delta, scale_3f32(clean_u, v_dist*6));
        g_kv("Middle dragging delta", "%.2f %.2f", delta.x, delta.y);
    }
    else 
    {
        // Commit pos delta
        node->pos = add_3f32(node->pos, node->pos_delta);
        MemoryZeroStruct(&node->pos_delta);
    }

    if(g_state->sig.f & UI_SignalFlag_RightDragging)
    {
        Vec2F32 delta = ui_drag_delta();
        g_kv("Right dragging delta", "%.2f %.2f", delta.x, delta.y);

        // TODO: float percision issue when v_turn_speed is higher, fix it later
        F32 v_turn_speed = 0.5f/(window_dim.y);
        F32 h_turn_speed = 2.0f/(window_dim.x);

        // TODO: we may need to clamp the turns to 0-1
        F32 v_turn = -delta.y * v_turn_speed;
        g_kv("v_turn", "%.2f", v_turn);
        F32 h_turn = delta.x * h_turn_speed;
        g_kv("h_turn", "%.2f", h_turn);

        QuatF32 v_quat = make_rotate_quat_f32(s, v_turn);
        QuatF32 h_quat = make_rotate_quat_f32(v3f32(0,1,0), h_turn);
        g_kv("v_quat", "%4.3f %4.3f %4.3f %4.3f", v_quat.x, v_quat.y, v_quat.z, v_quat.w);
        g_kv("h_quat", "%4.3g %4.3g %4.3g %4.3g", h_quat.x, h_quat.y, h_quat.z, h_quat.w);
        camera->rot_delta = mul_quat_f32(v_quat,h_quat);
        g_kv("rot_delta", "%4.3g %4.3g %4.3g %4.3g", camera->rot_delta.x, camera->rot_delta.y, camera->rot_delta.z, camera->rot_delta.w);
        g_kv("rot_delta_length", "%.2f", length_4f32(camera->rot_delta));

        // TEST: quaterion
        // QuatF32 nine_quat = make_rotate_quat_f32(v3f32(0,1,0), 0.25);
        // Vec3F32 p = {1,0,0};
        // Vec3F32 v_rotated = mul_quat_f32_v3f32(nine_quat, p);
        // Mat4x4F32 m = mat_4x4f32_from_quat_f32(nine_quat);
    }
    else
    {
        // Commit camera rot_delta
        camera->rot = mul_quat_f32(camera->rot_delta, camera->rot);
        camera->rot_delta = make_indentity_quat_f32();
    }

    // Scroll
    if(g_state->sig.scroll.x != 0 || g_state->sig.scroll.y != 0)
    {
        Vec3F32 dist = scale_3f32(f, g_state->sig.scroll.y/3.0f);
        node->pos = add_3f32(dist, node->pos);
    }

    if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Left))
    {
    }
    if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Right))
    {
    }
    if(os_key_is_down(OS_Key_Left))
    {
    }
    if(os_key_is_down(OS_Key_Right))
    { 
    }
}

/////////////////////////////////
// Player

typedef struct G_PlayerData G_PlayerData;
struct G_PlayerData
{
    Vec3F32 face;
};

G_NODE_CUSTOM_UPDATE(player_fn)
{
    G_PlayerData *data = (G_PlayerData *)node->custom_data;
    B32 grounded = 0;

    // Check if player is grounded or not

    // Zero out force

    // Zero out velocity on x axis

    // Zero out jump counter

    F32 h = 230;
    F32 foot_speed = 180;
    F32 xh = 60;
    F32 v0 = (-2*h*foot_speed) / xh;
    F32 g = (2*h*foot_speed*foot_speed) / (xh*xh);

    // Jump
    // if((os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Up) || os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Space) ) && (grounded || data->jump_counter == 1))
    // {
    // }
    if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Left))
    {
        node->pos.x -= 0.5f;
    }
    if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Right))
    {
        node->pos.x += 0.5f;
    }
    if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Up))
    {
        node->pos.z += 0.5f;
    }
    if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Down))
    {
        node->pos.z -= 0.5f;
    }
    if(os_key_is_down(OS_Key_Left))
    {
    }
    if(os_key_is_down(OS_Key_Right))
    { 
    }

    // Gravity
    if(!grounded)
    {
    }
    else
    {
    }

    // Friction 
    // Kinetic friction
    // if(body->velocity.x != 0) {
    //     F32 friction_at = friction_nf * dt_sec;
    //     if(friction_at > abs_f32(body->velocity.x)) {
    //         body->velocity.x = 0.0f;
    //     } else {
    //         // Apply friction force in the opposite direction of movement
    //         body->force.x += (friction_nf * (-velocity_direction.x));
    //     }
    // }

    // If the body is staionary, apply static friction
    // if(body->velocity.x == 0) {
    //    if(abs_f32(body->force.x) < friction_nf)  {
    //         body->force.x = 0.0f;
    //    }
    // }
}

/////////////////////////////////
// Scene build api

internal G_Scene *
g_scene_load()
{
    Arena *arena = arena_alloc();
    G_Scene *scene = push_array(arena, G_Scene, 1);
    scene->arena = arena;
    scene->bucket = g_bucket_make(arena, 3000);

    // Vertex, Normal, TexCoord, Color
    // 3+3+2+4=12 (row)
    const F32 vertices_src[8*12] =
    {
         // Front face
        -0.5f, -0.5f,  0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 0
         0.5f, -0.5f,  0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 1
         0.5f,  0.5f,  0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 2
        -0.5f,  0.5f,  0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 3
        
        // Back face
        -0.5f, -0.5f, -0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 4
         0.5f, -0.5f, -0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 5
         0.5f,  0.5f, -0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 6
        -0.5f,  0.5f, -0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 7
    };
    const U32 indices_src[3*12] = 
    {
        // Front face
        0, 1, 2,
        2, 3, 0,
        // Right face
        1, 5, 6,
        6, 2, 1,
        // Back face
        5, 4, 7,
        7, 6, 5,
        // Left face
        4, 0, 3,
        3, 7, 4,
        // Top face
        3, 2, 6,
        6, 7, 3,
        // Bottom face
        4, 5, 1,
        1, 0, 4
    };

    R_Handle vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(vertices_src), (void *)vertices_src);
    R_Handle indices = r_buffer_alloc(R_ResourceKind_Static, sizeof(indices_src), (void *)indices_src);

    G_Bucket_Scope(scene->bucket)
    {
        // Create the origin/world node
        G_Node *root = g_build_node_from_string(str8_lit("root"));
        root->pos = v3f32(0, 0, 0);
        scene->root = root;

        G_Parent_Scope(root)
        {
            G_Node *camera = g_node_camera3d_alloc(str8_lit("main_camera"));
            scene->camera = camera;
            camera->pos              = v3f32(0,-3,-3);
            camera->update_fn        = camera_fn;
            camera->camera.fov       = 0.25;
            camera->camera.zn        = 0.1f;
            camera->camera.zf        = 200.f;
            camera->camera.rot       = make_indentity_quat_f32();
            camera->camera.rot_delta = make_indentity_quat_f32();

            // Player
            G_Node *player = g_node_camera_mesh_inst3d_alloc(str8_lit("player"));
            G_Mesh *mesh = push_array(scene->bucket->arena, G_Mesh, 1);
            mesh->vertices = vertices;
            mesh->indices = indices;
            mesh->kind = G_MeshKind_Box;
            player->pos = v3f32(0,-0.51,0);
            player->mesh_inst3d.mesh = mesh;
            player->update_fn = player_fn;
            player->custom_data = push_array(scene->bucket->arena, G_PlayerData, 1);
        }
    }
    return scene;
}
