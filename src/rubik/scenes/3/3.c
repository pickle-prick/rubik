// camera
RK_NODE_CUSTOM_UPDATE(editor_camera2d_fn)
{
    RK_Transform3D *transform = &node->node3d->transform;
    RK_Camera3D *camera = node->camera3d;

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
        F32 scale = 0.1;
        transform->position.x = drag_data.start_transform.position.x - delta.x*scale;
        transform->position.y = drag_data.start_transform.position.y - delta.y*scale;
    }

    // Scroll
    if(rk_state->sig.scroll.x != 0 || rk_state->sig.scroll.y != 0)
    {
        F32 scale = 1 - rk_state->sig.scroll.y/10.0;
        camera->orthographic.top    *= scale;
        camera->orthographic.bottom *= scale;
        camera->orthographic.left   *= scale;
        camera->orthographic.right  *= scale;
    }
}

internal RK_Scene *
rk_scene_entry__3()
{
    RK_Scene *ret = rk_scene_alloc(str8_lit("2d_demo"), str8_lit("./src/rubik/scenes/3/default.rscn"));
    rk_push_node_bucket(ret->node_bucket);
    rk_push_res_bucket(ret->res_bucket);

    // scene settings
    {
        ret->omit_grid = 1;
        ret->omit_gizmo3d = 1;
    }

    RK_Node *root = rk_build_node3d_from_stringf(0, 0, "root");
    RK_Parent_Scope(root)
    {
        // create the orthographic camera 
        RK_Node *main_camera = rk_build_camera3d_from_stringf(0, 0, "camera2d");
        {
            main_camera->camera3d->projection = RK_ProjectionKind_Orthographic;
            main_camera->camera3d->viewport_shading = RK_ViewportShadingKind_Material;
            main_camera->camera3d->polygon_mode = R_GeoPolygonKind_Fill;
            main_camera->camera3d->hide_cursor = 0;
            main_camera->camera3d->lock_cursor = 0;
            main_camera->camera3d->is_active = 1;
            main_camera->camera3d->zn = 0;
            main_camera->camera3d->zf = 300;
            main_camera->camera3d->orthographic.top    = 0;
            main_camera->camera3d->orthographic.bottom = 300;
            main_camera->camera3d->orthographic.left   = 0;
            main_camera->camera3d->orthographic.right  = 300;

            main_camera->node3d->transform.position = v3f32(0,0,0);
            rk_node_push_fn(main_camera, editor_camera2d_fn);
        }
        ret->active_camera = rk_handle_from_node(main_camera);

        // load resource
        RK_SpriteSheet *doux_spritesheet = rk_spritesheet_from_image(str8_lit("./textures/DinoSprites - doux.png"), str8_lit("./textures/DinoSprites.json"));

        RK_Node *dino = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_AnimatedSprite2D,0, "dino");
        dino->node2d->transform.position = v2f32(3,3);
        dino->animated_sprite2d->sheet = doux_spritesheet;
        dino->animated_sprite2d->is_animating = 1;
        dino->animated_sprite2d->curr_tag = 1;
        dino->animated_sprite2d->loop = 1;
    }

    ret->root = rk_handle_from_node(root);
    rk_pop_node_bucket();
    rk_pop_res_bucket();
    return ret;
}
