typedef struct G_PlayerData G_PlayerData;
struct G_PlayerData {};

G_NODE_CUSTOM_UPDATE(player_fn)
{
    G_PlayerData *data = (G_PlayerData *)node->custom_data;

    Vec2F32 mouse_delta = sub_2f32(g_state->cursor, g_state->last_cursor);

    Vec3F32 f = {0};
    Vec3F32 s = {0};
    Vec3F32 u = {0};
    g_local_coord_from_node(node, &f, &s, &u);

    if(1)
    {
        // TODO: float percision issue when v_turn_speed is too high, fix it later
        F32 h_turn_speed = 1.5f/(g_state->window_dim.x);
        F32 v_turn_speed = 0.5f/(g_state->window_dim.y);

        // TODO: we may need to clamp the turns to 0-1
        F32 v_turn = -mouse_delta.y * v_turn_speed;
        F32 h_turn = -mouse_delta.x * h_turn_speed;

        QuatF32 h_quat = make_rotate_quat_f32(v3f32(0, -1, 0), h_turn);
        Vec3F32 s_after_h = mul_quat_f32_v3f32(h_quat, s);
        QuatF32 v_quat = make_rotate_quat_f32(s_after_h, v_turn);
        QuatF32 rot_quat = mul_quat_f32(v_quat,h_quat);
        node->rot = mul_quat_f32(rot_quat, node->rot);
    }

    // F32 h = 230;
    // F32 foot_speed = 180;
    // F32 xh = 60;
    // F32 v0 = (-2*h*foot_speed) / xh;
    // F32 g = (2*h*foot_speed*foot_speed) / (xh*xh);

    Vec3F32 move_dir = {0,0,0};
    if(os_key_is_down(OS_Key_Left) || os_key_is_down(OS_Key_A)) 
    {
        move_dir = sub_3f32(move_dir, s);
    }
    if(os_key_is_down(OS_Key_Right) || os_key_is_down(OS_Key_D))
    { 
        move_dir = add_3f32(move_dir, s);
    }
    if(os_key_is_down(OS_Key_Up) || os_key_is_down(OS_Key_W))
    { 
        move_dir = add_3f32(move_dir, f);
    }
    if(os_key_is_down(OS_Key_Down) || os_key_is_down(OS_Key_S))
    { 
        move_dir = sub_3f32(move_dir, f);
    }
    if(os_key_is_down(OS_Key_Space))
    {
        node->rot = make_indentity_quat_f32();
    }

    move_dir.y = 0;

    F32 velocity = 6.f;
    if(move_dir.x != 0 && move_dir.y != 0 && move_dir.z != 0) move_dir = normalize_3f32(move_dir);
    node->pos = add_3f32(node->pos, scale_3f32(move_dir, velocity*dt_sec));
}

G_NODE_CUSTOM_UPDATE(mesh_grp_fn)
{
    if(node->v.mesh_grp.is_skinned)
    {
        for(U64 i = 0; i < node->v.mesh_grp.joint_count; i++)
        {
            G_Node *joint_node = node->v.mesh_grp.joints[i];
            Mat4x4F32 joint_xform = joint_node->v.mesh_joint.inverse_bind_matrix;
            joint_xform = mul_4x4f32(joint_node->fixed_xform, joint_xform);
            // NOTE(k): we can either set the root joint float or we multiply with the inverse of mesh's global transform
            // joint_xform = mul_4x4f32(inverse_4x4f32(node->fixed_xform), joint_xform);
            node->v.mesh_grp.joint_xforms[i] = joint_xform;
        }
    }
}

// Camera (editor camera)
G_NODE_CUSTOM_UPDATE(editor_camera_fn)
{
    G_Camera3D *camera = &node->v.camera;

    if(g_state->sig.f & UI_SignalFlag_LeftDragging)
    {
        Vec2F32 delta = ui_drag_delta();
    }

    Vec3F32 f = {0};
    Vec3F32 s = {0};
    Vec3F32 u = {0};
    g_local_coord_from_node(node, &f, &s, &u);

    if(g_state->sig.f & UI_SignalFlag_MiddleDragging)
    {
        Vec2F32 delta = ui_drag_delta();
        // Horizontal
        F32 h_pct = delta.x / g_state->window_dim.x;
        F32 h_dist = 2.0 * h_pct;
        // Vertical
        F32 v_pct = delta.y / g_state->window_dim.y;
        F32 v_dist = -2.0 * v_pct;
        node->pre_pos_delta = scale_3f32(s, -h_dist * 6);
        node->pre_pos_delta = add_3f32(node->pre_pos_delta, scale_3f32(u, v_dist*6));
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
        F32 h_turn_speed = 2.0f/(g_state->window_dim.x);
        F32 v_turn_speed = 0.5f/(g_state->window_dim.y);

        // TODO: we may need to clamp the turns to 0-1
        F32 v_turn = -delta.y * v_turn_speed;
        F32 h_turn = -delta.x * h_turn_speed;

        QuatF32 h_quat = make_rotate_quat_f32(v3f32(0,-1,0), h_turn);
        Vec3F32 side = mul_quat_f32_v3f32(node->rot, v3f32(1,0,0));
        // NOTE: we should use s after h, but it doesn't seem to work as expected
        Vec3F32 s_after_h = mul_quat_f32_v3f32(h_quat, side);
        QuatF32 v_quat = make_rotate_quat_f32(s_after_h, v_turn);
        node->pre_rot_delta = mul_quat_f32(v_quat,h_quat);
    }
    else if(g_state->sig.f & UI_SignalFlag_RightReleased)
    {
        g_node_delta_commit(node);
    }

    // Scroll
    if(g_state->sig.scroll.x != 0 || g_state->sig.scroll.y != 0)
    {
        Vec3F32 dist = scale_3f32(f, g_state->sig.scroll.y/3.0f);
        node->pos = add_3f32(dist, node->pos);
    }
}
