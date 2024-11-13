internal G_Scene *
g_scene_000_load()
{
    Arena *arena = arena_alloc(.reserve_size = MB(64), .commit_size = MB(64));
    G_Scene *scene = push_array(arena, G_Scene, 1);
    {
        scene->arena = arena;
        scene->bucket = g_bucket_make(arena, 3000);
    }

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
                G_Node *camera = g_node_camera3d_alloc(str8_lit("main_camera"));
                scene->camera = camera;
                camera->pos          = v3f32(0,-3,-3);
                camera->v.camera.fov = 0.25;
                camera->v.camera.zn  = 0.1f;
                camera->v.camera.zf  = 200.f;
                g_node_push_fn(scene->bucket->arena, camera, camera_fn);

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

                    // player->update_fn   = player_fn;
                    // player->custom_data = push_array(scene->bucket->arena, G_PlayerData, 1);
                }

                // Cube
                G_Node *cube1 = g_build_node_from_stringf("cube1");
                {
                    cube1->flags |= G_NodeFlags_NavigationRoot;
                    cube1->kind = G_NodeKind_Mesh;
                    cube1->pos = v3f32(0,-1.5,3);
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
                    cylinder1->pos = v3f32(0,-9.5,3);
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
                    G_Node *dummy = g_build_node_from_stringf("dummy");
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

                // Dummy2
                // {
                //     G_Model *m;
                //     G_Path_Scope(str8_lit("./models/blackguard/"))
                //     {
                //         m = g_model_from_gltf(scene->arena, str8_lit("./models/blackguard/scene.gltf"));
                //     }
                //     G_Node *dummy = g_build_node_from_stringf("dummy2");
                //     dummy->flags |= G_NodeFlags_NavigationRoot;
                //     G_Parent_Scope(dummy) G_Seed_Scope(dummy->key) 
                //     {
                //         G_Node *n = g_node_from_model(m);
                //         QuatF32 flip_y = make_rotate_quat_f32(v3f32(1,0,0), 0.5);
                //         n->rot = mul_quat_f32(flip_y, n->rot);

                //         dummy->skeleton_anims = m->anims;
                //         dummy->flags |= (G_NodeFlags_Animated | G_NodeFlags_AnimatedSkeleton);
                //     }
                //     dummy->pos = v3f32(3,0,0);
                // }

                // Dummy3
                // {
                //     G_Model *m;
                //     G_Path_Scope(str8_lit("./models/dancing_stormtrooper/"))
                //     {
                //         m = g_model_from_gltf(scene->arena, str8_lit("./models/dancing_stormtrooper/scene.gltf"));
                //     }
                //     G_Node *dummy = g_build_node_from_stringf("dummy3");
                //     dummy->flags |= G_NodeFlags_NavigationRoot;
                //     G_Parent_Scope(dummy) G_Seed_Scope(dummy->key) 
                //     {
                //         G_Node *n = g_node_from_model(m);
                //         QuatF32 flip_y = make_rotate_quat_f32(v3f32(1,0,0), 0.5);
                //         n->rot = mul_quat_f32(flip_y, n->rot);

                //         dummy->skeleton_anims = m->anims;
                //         dummy->flags |= (G_NodeFlags_Animated | G_NodeFlags_AnimatedSkeleton);
                //     }
                //     dummy->pos = v3f32(6,0,0);
                // }
            }
        }
    }
    return scene;
}
