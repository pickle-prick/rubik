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

    B32 is_player_camera = 0;
    {
        G_Key camera_key = g_key_from_string(g_top_seed(), str8_lit("player_camera"));
        if(g_key_match(scene->active_camera->v->key, camera_key))
        {
            is_player_camera = 1;
        }
    }

    if(1)
    {

        Vec3F32 clean_f = f;
        clean_f.y = 0;
        Vec3F32 clean_s = s;
        clean_s.y = 0;
        Vec3F32 clean_u = u;
        clean_u.x = 0;
        clean_u.z = 0;

        // TODO: float percision issue when v_turn_speed is too high, fix it later
        F32 h_turn_speed = 1.5f/(g_state->window_dim.x);
        F32 v_turn_speed = 0.5f/(g_state->window_dim.y);

        // TODO: we may need to clamp the turns to 0-1
        F32 v_turn = -mouse_delta.y * v_turn_speed;
        F32 h_turn = -mouse_delta.x * h_turn_speed;

        QuatF32 h_quat = make_rotate_quat_f32(v3f32(0, -1, 0), h_turn);
        Vec3F32 s_after_h = mul_quat_f32_v3f32(h_quat, s);
        // QuatF32 h_quat = make_indentity_quat_f32();
        QuatF32 v_quat = make_rotate_quat_f32(s_after_h, v_turn);
        // QuatF32 v_quat = make_indentity_quat_f32();
        QuatF32 rot_quat = mul_quat_f32(v_quat,h_quat);
        node->rot = mul_quat_f32(rot_quat, node->rot);
    }

    // Check if player is grounded or not
    // Zero out force
    // Zero out velocity on x axis
    // Zero out jump counter

    F32 h = 230;
    F32 foot_speed = 180;
    F32 xh = 60;
    F32 v0 = (-2*h*foot_speed) / xh;
    F32 g = (2*h*foot_speed*foot_speed) / (xh*xh);

    // TODO: Jump
    // if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Space) ) && (grounded || data->jump_counter == 1)) {}
    // if(os_key_press(&os_events, g_state->os_wnd, 0, OS_Key_Left)) {}

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

    // TODO: Gravity
}

internal G_Scene *
g_scene_000_load()
{
    Arena *arena = arena_alloc(.reserve_size = MB(64), .commit_size = MB(64));
    G_Scene *scene = push_array(arena, G_Scene, 1);
    {
        scene->arena = arena;
        scene->bucket = g_bucket_make(arena, 3000);
    }

    // Fill base info
    scene->viewport_shading = G_ViewportShadingKind_Wireframe;
    scene->global_light     = v3f32(0,0,1);
    scene->polygon_mode     = R_GeoPolygonKind_Fill;

    G_Scene_Scope(scene)
    {
        G_Bucket_Scope(scene->bucket)
        {
            // Create the origin/world node
            G_Node *root = g_build_node_from_string(str8_lit("root"));
            root->pos = v3f32(0, 0, 0);
            scene->root = root;

            G_Parent_Scope(root)
            {
                G_Node *camera = g_node_camera3d_alloc(str8_lit("editor_camera"));
                {
                    camera->pos          = v3f32(0,-3,-3);
                    camera->v.camera.fov = 0.25;
                    camera->v.camera.zn  = 0.1f;
                    camera->v.camera.zf  = 200.f;
                    g_node_push_fn(scene->bucket->arena, camera, editor_camera_fn);
                }
                G_CameraNode *camera_node = push_array(arena, G_CameraNode, 1);
                camera_node->v = camera;
                DLLPushBack(scene->first_camera, scene->last_camera, camera_node);
                scene->active_camera = camera_node;

                // Player
                G_Node *player = g_build_node_from_stringf("player");
                {
                    player->flags |= G_NodeFlags_NavigationRoot;
                    player->kind = G_NodeKind_Mesh;
                    player->pos = v3f32(6,-1.5,0);
                    // Mesh
                    {
                        Temp temp = scratch_begin(0,0);
                        R_Vertex *vertices_src = 0;
                        U64 vertices_count     = 0;
                        U32 *indices_src       = 0;
                        U64 indices_count      = 0;
                        // g_mesh_primitive_sphere(temp.arena, &vertices_src, &vertices_count, &indices_src, &indices_count, 3, 6, 30, 16, 0);
                        g_mesh_primitive_capsule(temp.arena, &vertices_src, &vertices_count, &indices_src, &indices_count, 0.7, 3, 30, 16);
                        player->v.mesh.vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertices_count, (void *)vertices_src);
                        player->v.mesh.indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indices_count, (void *)indices_src);
                        scratch_end(temp);
                    }
                    g_node_push_fn(arena, player, player_fn);
                    player->custom_data = push_array(scene->bucket->arena, G_PlayerData, 1);

                    // Player camera
                    {
                        g_push_parent(player);
                        G_Node *camera = g_node_camera3d_alloc(str8_lit("player_camera"));
                        {
                            camera->pos                  = v3f32(0,-3,0);
                            camera->v.camera.fov         = 0.25;
                            camera->v.camera.zn          = 0.1f;
                            camera->v.camera.zf          = 200.f;
                            camera->v.camera.hide_cursor = 1;
                            camera->v.camera.lock_cursor = 1;
                            // g_node_push_fn(scene->bucket->arena, camera, pov_camera_fn);
                        }
                        g_pop_parent();
                        G_CameraNode *camera_node = push_array(arena, G_CameraNode, 1);
                        camera_node->v = camera;
                        DLLPushBack(scene->first_camera, scene->last_camera, camera_node);
                    }
                }

                // Cube
                G_Node *cube1 = g_build_node_from_stringf("cube1");
                {
                    cube1->flags |= G_NodeFlags_NavigationRoot;
                    cube1->kind = G_NodeKind_Mesh;
                    cube1->pos = v3f32(-6,-1.5,3);
                    // Mesh
                    {
                        Temp temp = scratch_begin(0,0);
                        R_Vertex *vertices_src = 0;
                        U64 vertices_count     = 0;
                        U32 *indices_src       = 0;
                        U64 indices_count      = 0;
                        // g_mesh_primitive_sphere(temp.arena, &vertices_src, &vertices_count, &indices_src, &indices_count, 3, 6, 30, 16, 0);
                        g_mesh_primitive_box(temp.arena, v3f32(3,3,3), 2, 2, 2, &vertices_src, &vertices_count, &indices_src, &indices_count);
                        cube1->v.mesh.vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertices_count, (void *)vertices_src);
                        cube1->v.mesh.indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indices_count, (void *)indices_src);
                        scratch_end(temp);
                    }
                }

                // Sphere1
                G_Node *sphere1 = g_build_node_from_stringf("sphere1");
                {
                    sphere1->flags |= G_NodeFlags_NavigationRoot;
                    sphere1->kind = G_NodeKind_Mesh;
                    sphere1->pos = v3f32(0,-9.5,3);
                    // Mesh
                    {
                        Temp temp = scratch_begin(0,0);
                        R_Vertex *vertices_src = 0;
                        U64 vertices_count     = 0;
                        U32 *indices_src       = 0;
                        U64 indices_count      = 0;
                        g_mesh_primitive_sphere(temp.arena, &vertices_src, &vertices_count, &indices_src, &indices_count, 3, 6, 30, 16, 0);
                        sphere1->v.mesh.vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertices_count, (void *)vertices_src);
                        sphere1->v.mesh.indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indices_count, (void *)indices_src);
                        scratch_end(temp);
                    }
                }

                // Cylinder1
                G_Node *cylinder1 = g_build_node_from_stringf("cylinder1");
                {
                    cylinder1->flags |= G_NodeFlags_NavigationRoot;
                    cylinder1->kind = G_NodeKind_Mesh;
                    cylinder1->pos = v3f32(6,-9.5,3);
                    // Mesh
                    {
                        Temp temp = scratch_begin(0,0);
                        R_Vertex *vertices_src = 0;
                        U64 vertices_count     = 0;
                        U32 *indices_src       = 0;
                        U64 indices_count      = 0;
                        g_mesh_primitive_cylinder(temp.arena, &vertices_src, &vertices_count, &indices_src, &indices_count, 3, 6, 30, 16, 1, 1);
                        cylinder1->v.mesh.vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertices_count, (void *)vertices_src);
                        cylinder1->v.mesh.indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indices_count, (void *)indices_src);
                        scratch_end(temp);
                    }
                }

                // Dummy1
                {
                    G_Model *m;
                    G_Path_Scope(str8_lit("./models/free_droide_de_seguridad_k-2so_by_oscar_creativo/"))
                    {
                        m = g_model_from_gltf(scene->arena, str8_lit("./models/free_droide_de_seguridad_k-2so_by_oscar_creativo/scene.gltf"));
                    }
                    for(U64 i = 0; i < 1; i++)
                    {
                        G_Node *dummy = g_build_node_from_stringf("dummy%d", i);
                        dummy->pos.x = i;
                        dummy->flags |= G_NodeFlags_NavigationRoot;
                        G_Parent_Scope(dummy) G_Seed_Scope(dummy->key) 
                        {
                            G_Node *n = g_node_from_model(m);
                            QuatF32 flip_y = make_rotate_quat_f32(v3f32(1,0,0), 0.5);
                            n->rot = mul_quat_f32(flip_y, n->rot);

                            dummy->skeleton_anims = m->anims;
                            dummy->flags |= (G_NodeFlags_Animated | G_NodeFlags_AnimatedSkeleton);
                        }
                    }
                }

                // Dummy2
                {
                    G_Model *m;
                    G_Path_Scope(str8_lit("./models/blackguard/"))
                    {
                        m = g_model_from_gltf(scene->arena, str8_lit("./models/blackguard/scene.gltf"));
                    }
                    G_Node *dummy = g_build_node_from_stringf("dummy2");
                    dummy->flags |= G_NodeFlags_NavigationRoot;
                    G_Parent_Scope(dummy) G_Seed_Scope(dummy->key) 
                    {
                        G_Node *n = g_node_from_model(m);
                        QuatF32 flip_y = make_rotate_quat_f32(v3f32(1,0,0), 0.5);
                        n->rot = mul_quat_f32(flip_y, n->rot);

                        dummy->skeleton_anims = m->anims;
                        dummy->flags |= (G_NodeFlags_Animated | G_NodeFlags_AnimatedSkeleton);
                    }
                    dummy->pos = v3f32(3,0,0);
                }

                // Dummy3
                {
                    G_Model *m;
                    G_Path_Scope(str8_lit("./models/dancing_stormtrooper/"))
                    {
                        m = g_model_from_gltf(scene->arena, str8_lit("./models/dancing_stormtrooper/scene.gltf"));
                    }
                    G_Node *dummy = g_build_node_from_stringf("dummy3");
                    dummy->flags |= G_NodeFlags_NavigationRoot;
                    G_Parent_Scope(dummy) G_Seed_Scope(dummy->key) 
                    {
                        G_Node *n = g_node_from_model(m);
                        QuatF32 flip_y = make_rotate_quat_f32(v3f32(1,0,0), 0.5);
                        n->rot = mul_quat_f32(flip_y, n->rot);

                        dummy->skeleton_anims = m->anims;
                        dummy->flags |= (G_NodeFlags_Animated | G_NodeFlags_AnimatedSkeleton);
                    }
                    dummy->pos = v3f32(6,0,0);
                }
            }
        }
    }
    return scene;
}
