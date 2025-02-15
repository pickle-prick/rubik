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
    rk_ijk_from_xform(node->fixed_xform, &s, &u, &f);

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

typedef struct AnimatedSpotLight AnimatedSpotLight;
struct AnimatedSpotLight
{
    Vec3F32 rotation_axis;
    F32 target_intensity;
    F32 target_range;
};

RK_NODE_CUSTOM_UPDATE(rotating_spot_light)
{
    RK_SpotLight *light = node->spot_light;
    AnimatedSpotLight *data = node->custom_data;

    // Animation rate
    F32 slow_rate = 1 - pow_f32(2, (-90.f * dt_sec));
    F32 fast_rate = 1 - pow_f32(2, (-40.f * dt_sec));

    // animating intensity
    // light->intensity += fast_rate * (light->intensity > 0.999f ? 0 : 1 - light->intensity);
    // light->range += slow_rate * (data->target_range - light->range);

    QuatF32 r = make_rotate_quat_f32(data->rotation_axis, 0.3*dt_sec);
    node->node3d->transform.rotation = mul_quat_f32(r, node->node3d->transform.rotation);
}

// default editor scene
internal RK_Scene *
rk_scene_entry__0()
{
    RK_Scene *ret = rk_scene_alloc(str8_lit("default"), str8_lit("./src/rubik/scenes/0/default.rscn"));
    rk_push_node_bucket(ret->node_bucket);
    rk_push_res_bucket(ret->res_bucket);

    RK_Node *root = rk_build_node3d_from_stringf(0, 0, "root");
    RK_Parent_Scope(root)
    {
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
            main_camera->camera3d->zn = 0.1;
            main_camera->camera3d->zf = 200.f;
            main_camera->camera3d->perspective.fov = 0.25f;
            rk_node_push_fn(main_camera, editor_camera_fn);
            main_camera->node3d->transform.position = v3f32(0,-3,0);
        }
        ret->active_camera = rk_handle_from_node(main_camera);

        /////////////////////////////////////////////////////////////////////////////////
        // load resource

        rk_push_node_bucket(ret->res_node_bucket);
        rk_push_parent(0);
        RK_Handle dancing_stormtrooper = rk_packed_scene_from_gltf(str8_lit("./models/dancing_stormtrooper/scene.gltf"));
        RK_Handle blackguard = rk_packed_scene_from_gltf(str8_lit("./models/blackguard/scene.gltf"));
        // RK_Handle droide = rk_packed_scene_from_gltf(str8_lit("./models/free_droide_de_seguridad_k-2so_by_oscar_creativo/scene.gltf"));
        RK_Handle white_mat = rk_material_from_color(str8_lit("white"), v4f32(1,1,1,1));
        RK_Handle grid_prototype_material;
        {
            String8 png_path = str8_lit("./textures/gridbox-prototype-materials/prototype_512x512_blue2.png");
            RK_Key material_res_key = rk_res_key_from_string(RK_ResourceKind_Material, rk_key_zero(), png_path);
            grid_prototype_material = rk_resh_alloc(material_res_key, RK_ResourceKind_Material, 1);
            RK_Material *material = rk_res_data_from_handle(grid_prototype_material);

            // load texture
            RK_Key tex2d_key = rk_res_key_from_string(RK_ResourceKind_Texture2D, rk_key_zero(), png_path);
            RK_Handle tex2d_res = rk_resh_alloc(tex2d_key, RK_ResourceKind_Texture2D, 1);
            R_Tex2DSampleKind sample_kind = R_Tex2DSampleKind_Nearest;
            RK_Texture2D *tex2d = rk_res_data_from_handle(tex2d_res);
            {
                int x,y,n;
                U8 *image_data = stbi_load((char*)png_path.str, &x, &y, &n, 4);
                tex2d->tex = r_tex2d_alloc(R_ResourceKind_Static, sample_kind, v2s32(x,y), R_Tex2DFormat_RGBA8, image_data);
                stbi_image_free(image_data);
            }
            tex2d->sample_kind = sample_kind;

            material->name = push_str8_copy_static(str8_lit("prototype_512x512_blue2"), material->name_buffer, ArrayCount(material->name_buffer));
            material->textures[R_GeoTexKind_Diffuse] = tex2d_res;
            material->v.diffuse_color = v4f32(1,1,1,1);
            material->v.opacity = 1.0;
            material->v.has_diffuse_texture = 1;
        }

        rk_pop_parent();
        rk_pop_node_bucket();

        /////////////////////////////////////////////////////////////////////////////////
        // spwan node

        // reference plane
        RK_Node *floor = rk_build_node3d_from_stringf(0,0,"floor");
        rk_node_equip_plane(floor, v2f32(9,9), 0,0);

        floor->node3d->transform.position.y -= 0.01;
        floor->mesh_inst3d->material_override = white_mat;
        floor->mesh_inst3d->material_override = grid_prototype_material;
        floor->flags |= RK_NodeFlag_NavigationRoot;

        // RK_Node *n1 = rk_node_from_packed_scene(str8_lit("1"), droide);
        // {
        //     // flip y
        //     n1->node3d->transform.rotation = mul_quat_f32(make_rotate_quat_f32(v3f32(1,0,0), 0.5f), n1->node3d->transform.rotation);
        //     n1->node3d->transform.position = v3f32(-3,0,0);
        //     n1->flags |= RK_NodeFlag_NavigationRoot;
        // }

        // directional light
        {
            RK_Node *l1 = rk_build_node3d_from_stringf(RK_NodeTypeFlag_DirectionalLight, 0, "direction_light_1");
            l1->directional_light->color = v3f32(0.1,0.1,0.1);
            l1->directional_light->intensity = 0.1;
            l1->directional_light->direction = normalize_3f32(v3f32(1,1,0));
        }

        // point light 1
        {
            RK_Node *l2 = rk_build_node3d_from_stringf(RK_NodeTypeFlag_PointLight, 0, "point_light_1");
            rk_node_equip_sphere(l2, 0.1, 0.2, 9,9,0);
            l2->flags |= RK_NodeFlag_NavigationRoot;
            l2->node3d->transform.position.y -= 3.5;
            l2->mesh_inst3d->omit_light = 1;
            l2->mesh_inst3d->draw_edge = 1;
            l2->point_light->color = v3f32(1,1,1);
            l2->point_light->range = 3.9;
            l2->point_light->intensity = 1;
            l2->point_light->attenuation = v3f32(0,0,1);
        }

        // spot lights
        for(S64 i = 0; i < 3; i++)
        {
            RK_Node *l = rk_build_node3d_from_stringf(RK_NodeTypeFlag_SpotLight, 0, "spot_light_%I64d", i);
            rk_node_equip_box(l, v3f32(0.15, 0.15, 0.15), 0,0,0);
            l->flags |= RK_NodeFlag_NavigationRoot;
            l->node3d->transform.position.y -= 3;
            l->node3d->transform.position.x = -3 + i*3;
            l->mesh_inst3d->omit_light = 1;
            l->mesh_inst3d->draw_edge = 1;
            l->spot_light->color.v[i] = 1;
            l->spot_light->intensity = 1;
            l->spot_light->attenuation = v3f32(0,0,1);
            l->spot_light->direction = normalize_3f32(v3f32(1,1,0));
            l->spot_light->range = 9.9;
            l->spot_light->angle = radians_from_turns_f32(0.09);
            rk_node_push_fn(l, rotating_spot_light);

            AnimatedSpotLight *data = rk_node_custom_data_alloc(AnimatedSpotLight);
            data->rotation_axis.v[i] = 1;
            l->custom_data = data;
        }

        RK_Node *n2 = rk_node_from_packed_scene(str8_lit("2"), blackguard);
        {
            // flip y
            n2->node3d->transform.rotation = mul_quat_f32(make_rotate_quat_f32(v3f32(1,0,0), 0.5f), n2->node3d->transform.rotation);
            n2->node3d->transform.position = v3f32(0,0,0);
            n2->flags |= RK_NodeFlag_NavigationRoot;
        }

        RK_Node *n3 = rk_node_from_packed_scene(str8_lit("3"), dancing_stormtrooper);
        {
            // flip y
            n3->node3d->transform.rotation = mul_quat_f32(make_rotate_quat_f32(v3f32(1,0,0), 0.5f), n3->node3d->transform.rotation);
            n3->node3d->transform.position = v3f32(3,0,0);
            n3->flags |= RK_NodeFlag_NavigationRoot;
        }
    }

    ret->root = rk_handle_from_node(root);
    rk_pop_node_bucket();
    rk_pop_res_bucket();
    return ret;
}
