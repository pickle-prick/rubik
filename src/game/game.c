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

    // G_Node *hot_node = 0;
    G_Node *active_node = 0;

    // Stats pane
    ui_pane_begin(r2f32p(0, 700, 600, 1400), str8_lit("game"));
    ui_column_begin();
    F32 dt_sec = dt/1000000.0f;

    B32 show_grid   = 1;
    B32 show_gizmos = 0;
    // TODO: this is kind awkward
    d_push_bucket(g_state->bucket_geo3d);
    R_PassParams_Geo3D *pass = d_geo3d_begin(window_rect, mat_4x4f32(1.f), mat_4x4f32(1.f),
                                             show_grid, show_gizmos,
                                             mat_4x4f32(1.f), v3f32(0,0,0));
    d_pop_bucket();

    // Update node
    {
        G_Node *node = scene->root;
        while(node != 0)
        {
            F32 dt_ms = dt / 1000.f;
            if(g_key_match(g_state->active_key, node->key)) { active_node = node; }

            // Update xfrom (Top-down)
            Mat4x4F32 xform = g_xform_from_node(node);
            node->fixed_xform = xform;

            D_BucketScope(g_state->bucket_geo3d)
            {
                switch(node->kind)
                {
                    default: {}break;
                    case G_NodeKind_MeshInstance3D:
                    {
                        R_Mesh3DInst *inst = d_mesh(node->mesh_inst3d.mesh->vertices, node->mesh_inst3d.mesh->indices,
                                                    R_GeoTopologyKind_Triangles,
                                                    R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                                                    r_handle_zero(), mat_4x4f32(1.f), node->key.u64[0]);
                        inst->xform = xform;
                    }break;
                }
            }
            if(node->update_fn != 0)
            {
                node->update_fn(node, scene, os_events, dt_sec);
            }
            node = g_node_df_pre(node, 0);
        }
    }

    // Unpack camera
    G_Node *camera = scene->camera;
    pass->view = g_view_from_node(camera);
    pass->projection = make_perspective_vulkan_4x4f32(camera->camera.fov, window_dim.x/window_dim.y, camera->camera.zn, camera->camera.zf);
    Mat4x4F32 xform_m = mul_4x4f32(pass->projection, pass->view);

    if(active_node != 0) 
    {
        pass->show_gizmos = 1;
        pass->gizmos_xform = active_node->fixed_xform;
        pass->gizmos_origin = v4f32(pass->gizmos_xform.v[3][0],
                                    pass->gizmos_xform.v[3][1],
                                    pass->gizmos_xform.v[3][2],
                                    1);
        g_kv("active node", "%s", active_node->name.str);
    }

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
            Vec3F32 local_i;
            Vec3F32 local_j;
            Vec3F32 local_k;
            g_local_coord_from_node(active_node, &local_k, &local_i, &local_j);

            // Dragging started
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
                    case G_SpecialKeyKind_GizmosIhat: { direction = local_i; }break;
                    case G_SpecialKeyKind_GizmosJhat: { direction = local_j; }break;
                    case G_SpecialKeyKind_GizmosKhat: { direction = local_k; }break;
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
                g_kv("projected_dir", "%.2f %.2f", projected_dir.x, projected_dir.y);

                // TODO: negating direction could cause rapid changes on direction, we would want to avoid that 
                S32 n = dot_2f32(delta, projected_dir) > 0 ? 1 : -1;

                // 10.0 in word coordinate per pixel (scaled by the distance to the eye)
                F32 speed = 1.0/(window_dim.x*length_2f32(projected_dir));

                // g_kv("drag_start_direction", "%.2f %.2f %.2f", g_state->drag_start_direction.x, g_state->drag_start_direction.y, g_state->drag_start_direction.z);
                // g_kv("drag_length", "%.2f", length_2f32(delta)*speed*n);
                active_node->pos_delta = scale_3f32(g_state->drag_start_direction, length_2f32(delta)*speed*n);
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
        F32 v_dist = -2.0 * v_pct;
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
        node->rot_delta = mul_quat_f32(v_quat,h_quat);
        g_kv("rot_delta", "%4.3g %4.3g %4.3g %4.3g", node->rot_delta.x, node->rot_delta.y, node->rot_delta.z, node->rot_delta.w);
        g_kv("rot_delta_length", "%.2f", length_4f32(node->rot_delta));

        // TEST: quaterion
        // QuatF32 nine_quat = make_rotate_quat_f32(v3f32(0,1,0), 0.25);
        // Vec3F32 p = {1,0,0};
        // Vec3F32 v_rotated = mul_quat_f32_v3f32(nine_quat, p);
        // Mat4x4F32 m = mat_4x4f32_from_quat_f32(nine_quat);
    }
    else
    {
        // Commit camera rot_delta
        node->rot = mul_quat_f32(node->rot_delta, node->rot);
        node->rot_delta = make_indentity_quat_f32();
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
}

G_NODE_CUSTOM_UPDATE(dummy_fn)
{
    Vec3F32 pos = {0,0,0};
    pos = mat_4x4f32_transform_3f32(node->fixed_xform, pos);
    g_kv("dummy pos", "%.2f %.2f %.2f", pos.x, pos.y, pos.z);
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

    // TODO: searilize scene data using yaml or something

    R_Handle vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(vertices_src), (void *)vertices_src);
    R_Handle indices = r_buffer_alloc(R_ResourceKind_Static, sizeof(indices_src), (void *)indices_src);

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
                camera->pos              = v3f32(0,-3,-3);
                camera->update_fn        = camera_fn;
                camera->camera.fov       = 0.25;
                camera->camera.zn        = 0.1f;
                camera->camera.zf        = 200.f;

                // Player
                G_Node *player = g_node_camera_mesh_inst3d_alloc(str8_lit("player"));
                G_Mesh *mesh = push_array(scene->bucket->arena, G_Mesh, 1);
                mesh->vertices = vertices;
                mesh->indices  = indices;
                mesh->kind     = G_MeshKind_Box;
                player->pos              = v3f32(0,-0.51,0);
                player->mesh_inst3d.mesh = mesh;
                player->update_fn        = player_fn;
                player->custom_data      = push_array(scene->bucket->arena, G_PlayerData, 1);

                // Dummy
                G_Node *n = node_from_gltf(scene->arena, str8_lit("./models/free_droide_de_seguridad_k-2so_by_oscar_creativo/scene.gltf"), str8_lit("dummy"));
                n->pos       = v3f32(3,0,0);
                n->scale     = v3f32(0.01,0.01,0.01);
                n->rot       = make_rotate_quat_f32(v3f32(1,0,0), 0.25);
                n->update_fn = dummy_fn;
            }
        }
    }
    return scene;
}

internal G_Node *
node_from_gltf(Arena *arena, String8 gltf_path, String8 string)
{
    G_Node *node = 0;
    // TODO: Create a model loader layer to support different model format (gltf, blend, obj, fbx ...)
    cgltf_options opts = {0};
    cgltf_data *data;
    cgltf_result ret = cgltf_parse_file(&opts, (char *)gltf_path.str, &data);
    if(ret == cgltf_result_success)
    {
        cgltf_options buf_ld_opts = {0};
        cgltf_result buf_ld_ret = cgltf_load_buffers(&buf_ld_opts, data, (char *)gltf_path.str);
        AssertAlways(buf_ld_ret == cgltf_result_success);

        AssertAlways(data->scenes_count == 1);
        cgltf_scene *root_scene = &data->scenes[0];
        AssertAlways(root_scene->nodes_count == 1);

        cgltf_node *root= root_scene->nodes[0];
        node = node_from_gltf_node(arena, root);
        cgltf_free(data);
    }
    else
    { InvalidPath; }

    return node;
}
internal G_Node *
node_from_gltf_node(Arena *arena, cgltf_node *cn)
{
    cgltf_mesh *mesh = cn->mesh;
    Temp temp = scratch_begin(0,0);
    String8 prefix = push_str8_cat(temp.arena,
                                   cn->parent ? str8_cstring(cn->parent->name) : str8_lit(""),
                                   cn->parent ? str8_lit("-") : str8_lit(""));
    String8 node_name = push_str8_cat(arena, prefix, str8_cstring(cn->name));
    G_Node *node = g_build_node_from_string(node_name);

    if(mesh != 0)
    {
        G_Parent_Scope(node)
        {
            for(U64 i = 0; i < cn->mesh->primitives_count; i++)
            {
                String8 mesh_node_name = push_str8_cat(arena, node_name, push_str8f(temp.arena, "#%d", i));
                G_Node *mesh_node = g_node_camera_mesh_inst3d_alloc(mesh_node_name);
                G_Mesh *mesh = g_mesh_alloc();
                mesh->kind = G_MeshKind_Custom;
                mesh_node->mesh_inst3d.mesh = mesh;

                cgltf_primitive *primitive = &cn->mesh->primitives[i];
                AssertAlways(primitive->type == cgltf_primitive_type_triangles);

                // Mesh Indices
                {
                    cgltf_accessor *accessor = primitive->indices;
                    AssertAlways(accessor->type == cgltf_type_scalar);
                    AssertAlways(accessor->stride == 4);
                    U64 offset = accessor->offset+accessor->buffer_view->offset;
                    void *indices_src = (U8 *)accessor->buffer_view->buffer->data+offset;
                    U64 size = sizeof(U32) * accessor->count;
                    R_Handle indices = r_buffer_alloc(R_ResourceKind_Static, size, indices_src);
                    mesh->indices = indices;
                }

                // Mesh vertex attributes
                {
                    cgltf_attribute *attrs = primitive->attributes;
                    cgltf_size attr_count = primitive->attributes_count;
                    U64 vertex_count = attrs[0].data->count;
                    R_Vertex vertices_src[vertex_count];

                    // position, normal, color, texcoord (TODO: joints, weights, tangent)
                    for(U64 i = 0; i < vertex_count; i++)
                    {
                        R_Vertex *v = &vertices_src[i];
                        for(U64 j = 0; j < attr_count; j++)
                        {
                            cgltf_attribute *attr = &attrs[j];
                            cgltf_accessor *accessor = attr->data;
                            // cgltf_accessor *accessor = &attr->data[attr->index];

                            U64 src_offset = accessor->offset + accessor->buffer_view->offset + accessor->stride*i;
                            U8 *src = (U8 *)accessor->buffer_view->buffer->data + src_offset;
                            U64 dst_offset = 0;
                            U64 size = 0;
                            switch(attr->type)
                            {
                                case cgltf_attribute_type_position:
                                {
                                    dst_offset = OffsetOf(R_Vertex, pos);
                                    size = sizeof(((struct R_Vertex *)0)->pos);
                                }break;
                                case cgltf_attribute_type_normal:
                                {
                                    dst_offset = OffsetOf(R_Vertex, nor);
                                    size = sizeof(((struct R_Vertex *)0)->nor);
                                }break;
                                case cgltf_attribute_type_texcoord:
                                {
                                    dst_offset = OffsetOf(R_Vertex, tex);
                                    size = sizeof(((struct R_Vertex *)0)->tex);
                                }break;
                                case cgltf_attribute_type_color:
                                {
                                    dst_offset = OffsetOf(R_Vertex, col);
                                    size = sizeof(((struct R_Vertex *)0)->col);
                                }break;
                                default: {}break;
                            }
                            MemoryCopy((U8 *)v+dst_offset, src, size);
                        }
                    }
                    R_Handle vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(vertices_src), (void *)vertices_src);
                    mesh->vertices = vertices;
                }
            }
        }
    }

    scratch_end(temp);

    G_Parent_Scope(node)
    {
        for(U64 i = 0; i < cn->children_count; i++)
        {
            node_from_gltf_node(arena, cn->children[i]);
        }
    }
    return node;
}
