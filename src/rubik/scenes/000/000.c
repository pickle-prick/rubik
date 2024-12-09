typedef struct RK_PlayerData RK_PlayerData;
struct RK_PlayerData {};

RK_NODE_CUSTOM_UPDATE(player_fn)
{
    RK_PlayerData *data = (RK_PlayerData *)node->custom_data;

    Vec2F32 mouse_delta = sub_2f32(rk_state->cursor, rk_state->last_cursor);

    Vec3F32 f = {0};
    Vec3F32 s = {0};
    Vec3F32 u = {0};
    rk_local_coord_from_node(node, &f, &s, &u);

    if(1)
    {
        // TODO: float percision issue when v_turn_speed is too high, fix it later
        F32 h_turn_speed = 1.5f/(rk_state->window_dim.x);
        F32 v_turn_speed = 0.5f/(rk_state->window_dim.y);

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

RK_NODE_CUSTOM_UPDATE(mesh_grp_fn)
{
    if(node->v.mesh_grp.is_skinned)
    {
        for(U64 i = 0; i < node->v.mesh_grp.joint_count; i++)
        {
            RK_Node *joint_node = node->v.mesh_grp.joints[i];
            Mat4x4F32 joint_xform = joint_node->v.mesh_joint.inverse_bind_matrix;
            joint_xform = mul_4x4f32(joint_node->fixed_xform, joint_xform);
            // NOTE(k): we can either set the root joint float or we multiply with the inverse of mesh's global transform
            // joint_xform = mul_4x4f32(inverse_4x4f32(node->fixed_xform), joint_xform);
            node->v.mesh_grp.joint_xforms[i] = joint_xform;
        }
    }
}

// Camera (editor camera)
RK_NODE_CUSTOM_UPDATE(editor_camera_fn)
{
    RK_Camera *camera = &node->v.camera;

    if(rk_state->sig.f & UI_SignalFlag_LeftDragging)
    {
        Vec2F32 delta = ui_drag_delta();
    }

    Vec3F32 f = {0};
    Vec3F32 s = {0};
    Vec3F32 u = {0};
    rk_local_coord_from_node(node, &f, &s, &u);

    if(rk_state->sig.f & UI_SignalFlag_MiddleDragging)
    {
        Vec2F32 delta = ui_drag_delta();
        // Horizontal
        F32 h_pct = delta.x / rk_state->window_dim.x;
        F32 h_dist = 2.0 * h_pct;
        // Vertical
        F32 v_pct = delta.y / rk_state->window_dim.y;
        F32 v_dist = -2.0 * v_pct;
        node->pre_pos_delta = scale_3f32(s, -h_dist * 6);
        node->pre_pos_delta = add_3f32(node->pre_pos_delta, scale_3f32(u, v_dist*6));
    }
    else if(rk_state->sig.f & UI_SignalFlag_MiddleReleased)
    {
        // Commit pos delta
        rk_node_delta_commit(node);
    }

    if(rk_state->sig.f & UI_SignalFlag_RightDragging)
    {
        Vec2F32 delta = ui_drag_delta();

        // TODO: float percision issue when v_turn_speed is too high, fix it later
        F32 h_turn_speed = 2.0f/(rk_state->window_dim.x);
        F32 v_turn_speed = 0.5f/(rk_state->window_dim.y);

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
    else if(rk_state->sig.f & UI_SignalFlag_RightReleased)
    {
        rk_node_delta_commit(node);
    }

    // Scroll
    if(rk_state->sig.scroll.x != 0 || rk_state->sig.scroll.y != 0)
    {
        Vec3F32 dist = scale_3f32(f, rk_state->sig.scroll.y/3.0f);
        node->pos = add_3f32(dist, node->pos);
    }
}

// default editor scene
internal RK_Scene *
rk_scene_default(void)
{
    RK_Scene *scene = rk_scene_alloc();

    // Fill base info
    scene->name             = str8_lit("default");
    scene->path             = str8_lit("./src/rubik/scenes/000/default.scene");
    scene->viewport_shading = RK_ViewportShadingKind_Wireframe;
    scene->global_light     = v3f32(0,0,1);
    scene->polygon_mode     = R_GeoPolygonKind_Fill;

    RK_Bucket_Scope(scene->bucket) RK_MeshCacheTable_Scope(&scene->mesh_cache_table)
    {
        // Create the origin/world node
        RK_Node *root = rk_build_node_from_stringf(0, "root");
        root->pos = v3f32(0, 0, 0);
        scene->root = root;

        RK_Parent_Scope(root)
        {
            RK_Node *camera = rk_camera_perspective(0,0,1,1, 0.25f, 0.1f, 200.f, str8_lit("editor_camera"));
            {
                camera->pos                 = v3f32(0,-3,-3);
                RK_FunctionNode *fn = rk_function_from_string(str8_lit("editor_camera_fn"));
                rk_node_push_fn(scene->bucket->arena, camera, fn->ptr, fn->alias);
            }
            RK_CameraNode *camera_node = push_array(scene->arena, RK_CameraNode, 1);
            camera_node->v = camera;
            DLLPushBack(scene->first_camera, scene->last_camera, camera_node);
            scene->active_camera = camera_node;
            
            // TODO(k): testing for orthographic projection
            {
                RK_Node *test_camera = rk_camera_orthographic(0,0,0,1, -4.f, 4.f, -4.f, 4.f, 0.1f, 200.f, str8_lit("test_camera"));
                test_camera->pos.z = -3.f;
                {
                    RK_FunctionNode *fn = rk_function_from_string(str8_lit("editor_camera_fn"));
                    rk_node_push_fn(scene->bucket->arena, test_camera, fn->ptr, fn->alias);

                    RK_CameraNode *camera_node = push_array(scene->arena, RK_CameraNode, 1);
                    camera_node->v = test_camera;
                    DLLPushBack(scene->first_camera, scene->last_camera, camera_node);
                }
            }

            // Player
            RK_Node *player = rk_build_node_from_stringf(0, "player");
            {
                player->flags |= RK_NodeFlags_NavigationRoot;
                player->pos = v3f32(6,-1.5,0);
                RK_FunctionNode *fn = rk_function_from_string(str8_lit("player_fn"));
                rk_node_push_fn(scene->arena, player, fn->ptr, fn->alias);

                RK_Parent_Scope(player)
                {
                    //- k: mesh
                    RK_Node *body = rk_build_node_from_stringf(0, "body");
                    body->kind = RK_NodeKind_MeshRoot;
                    body->v.mesh_root.kind = RK_MeshKind_Capsule;
                    RK_Parent_Scope(body)
                    {
                        rk_capsule_node_default(str8_lit("capsule"));
                    }
                    //- k: player first person camera
                    RK_Node *camera = rk_camera_perspective(1,1,1,1, 0.25f, 0.1f, 200.f, str8_lit("player_pov_camera"));
                    RK_CameraNode *camera_node = push_array(scene->arena, RK_CameraNode, 1);
                    camera_node->v = camera;
                    DLLPushBack(scene->first_camera, scene->last_camera, camera_node);
                }
            }

            // Dummy1
            {
                RK_Model *m;
                String8 model_path = str8_lit("./models/dancing_stormtrooper/scene.gltf");
                String8 model_directory = str8_chop_last_slash(model_path);
                RK_Path_Scope(model_directory)
                {
                    m = rk_model_from_gltf_cached(model_path);
                }
                RK_Node *dummy = rk_build_node_from_stringf(0, "dummy1");
                dummy->kind             = RK_NodeKind_MeshRoot;
                dummy->skeleton_anims   = m->anims;
                dummy->flags            = RK_NodeFlags_NavigationRoot|RK_NodeFlags_Animated|RK_NodeFlags_AnimatedSkeleton;
                dummy->pos              = v3f32(6,0,0);
                dummy->v.mesh_root.path = push_str8_copy(scene->arena, model_path);
                dummy->v.mesh_root.kind = RK_MeshKind_Model;
                QuatF32 flip_y = make_rotate_quat_f32(v3f32(1,0,0), 0.5);
                dummy->rot = mul_quat_f32(flip_y, dummy->rot);

                RK_Parent_Scope(dummy)
                {
                    rk_node_from_model(m, rk_active_seed_key());
                }
            }

            {
                RK_Model *m;
                String8 model_path = str8_lit("./models/free_droide_de_seguridad_k-2so_by_oscar_creativo/scene.gltf");
                String8 model_directory = str8_chop_last_slash(model_path);
                RK_Path_Scope(model_directory)
                {
                    m = rk_model_from_gltf_cached(model_path);
                }
                RK_Node *n = rk_build_node_from_stringf(0, "dummy2");
                n->kind             = RK_NodeKind_MeshRoot;
                n->skeleton_anims   = m->anims;
                n->flags            = RK_NodeFlags_NavigationRoot|RK_NodeFlags_Animated|RK_NodeFlags_AnimatedSkeleton;
                n->pos              = v3f32(0,0,0);
                n->v.mesh_root.path = push_str8_copy(scene->arena, model_path);
                n->v.mesh_root.kind = RK_MeshKind_Model;
                QuatF32 flip_y = make_rotate_quat_f32(v3f32(1,0,0), 0.5);
                n->rot = mul_quat_f32(flip_y, n->rot);

                RK_Parent_Scope(n)
                {
                    rk_node_from_model(m, rk_active_seed_key());
                }
            }
        }
    }
    return scene;
}
