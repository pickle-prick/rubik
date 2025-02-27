// camera
RK_NODE_CUSTOM_UPDATE(fn_editor_camera2d)
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

typedef struct Dino Dino;
struct Dino
{
    Dir2 face_direction;
};

RK_NODE_CUSTOM_UPDATE(fn_player_dino)
{
    RK_AnimatedSprite2D *sprite2d = node->animated_sprite2d;
    Dino *dino = node->custom_data;
    RK_Transform2D *transform = &node->node2d->transform;

    B32 moving = 0;
    if(os_key_is_down(OS_Key_Left))
    {
        moving = 1;
        transform->position.x -= 1;
        dino->face_direction = Dir2_Left;
    }
    if(os_key_is_down(OS_Key_Right))
    {
        moving = 1;
        transform->position.x += 1;
        dino->face_direction = Dir2_Right;
    }
    if(os_key_is_down(OS_Key_Up))
    // if(os_key_press(&os_events, rk_state->os_wnd, 0, OS_Key_Up))
    {
        moving = 1;
        transform->position.y -= 1;
        dino->face_direction = Dir2_Up;
    }
    if(os_key_is_down(OS_Key_Down))
    {
        moving = 1;
        transform->position.y += 1;
        dino->face_direction = Dir2_Down;
    }

    if(moving)
    {
        sprite2d->curr_tag = 1;
    }
    else
    {
        sprite2d->curr_tag = 0;
        sprite2d->loop = 1;
    }

    sprite2d->flipped = dino->face_direction == Dir2_Left;
}

RK_NODE_CUSTOM_UPDATE(fn_cell)
{
    RK_Sprite2D *sprite2d = node->sprite2d;
    if(rk_key_match(scene->hot_key, node->key))
    {
        sprite2d->color.w = 0.3;
    }
    else
    {
        sprite2d->color.w = 1;
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

    // 2d viewport
    Rng2F32 viewport = {0,0,300,300};
    Vec2F32 viewport_dim = dim_2f32(viewport);

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
            main_camera->camera3d->zn = -0.1;
            main_camera->camera3d->zf = 1000; // support 1000 layers
            main_camera->camera3d->orthographic.top    = viewport.y0;
            main_camera->camera3d->orthographic.bottom = viewport.y1;
            main_camera->camera3d->orthographic.left   = viewport.x0;
            main_camera->camera3d->orthographic.right  = viewport.x1;

            main_camera->node3d->transform.position = v3f32(0,0,0);
            rk_node_push_fn(main_camera, fn_editor_camera2d);
        }
        ret->active_camera = rk_handle_from_node(main_camera);

        // load resource
        RK_SpriteSheet *doux_spritesheet = rk_spritesheet_from_image(str8_lit("./textures/DinoSprites - doux.png"), str8_lit("./textures/DinoSprites.json"));

        Vec2F32 cell_count = v2f32(10,10);
        Vec2F32 cell_size = v2f32(10,10);
        Vec2F32 grid_size = v2f32(cell_size.x*cell_count.x, cell_size.y*cell_count.y);

        // grid
        RK_Node *grid = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,0, "grid");
        grid->sprite2d->size = grid_size;
        grid->node2d->transform.position = v2f32(viewport_dim.x/2.f, viewport_dim.y/2.0f);
        grid->node2d->transform.depth = 2;
        grid->sprite2d->color = v4f32(0.3,1,1,1);

        F32 y = grid->node2d->transform.position.y - (grid_size.y/2.0) + (cell_size.y/2.0);
        // stones
        for(U64 j = 0; j < cell_count.y; j++)
        {
            F32 x = grid->node2d->transform.position.x - (grid_size.x/2.0) + (cell_size.x/2.0);
            for(U64 i = 0; i < cell_count.x; i++)
            {
                RK_Node *cell = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,0, "cell_%d_%d", i,j);
                cell->sprite2d->size = cell_size;
                cell->node2d->transform.position = v2f32(x,y);
                cell->node2d->transform.scale = v2f32(0.9,0.9);
                cell->node2d->transform.depth = 1;
                cell->sprite2d->color = v4f32(0.1,0.1,1,1);
                rk_node_push_fn(cell, fn_cell);
                x += cell_size.x;
            }
            y += cell_size.y;
        }

        RK_Node *dino = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_AnimatedSprite2D,0, "dino");
        dino->node2d->transform.position = v2f32(3,3);
        dino->node2d->transform.depth = 1.0;
        dino->animated_sprite2d->sheet = doux_spritesheet;
        dino->animated_sprite2d->is_animating = 1;
        dino->animated_sprite2d->curr_tag = 0; // idle
        dino->animated_sprite2d->curr_tag = 0;
        dino->animated_sprite2d->loop = 0;
        rk_node_push_fn(dino, fn_player_dino);
        Dino *dino_data = rk_node_push_custom_data(dino, Dino);
        dino_data->face_direction = Dir2_Right;
    }

    ret->root = rk_handle_from_node(root);
    rk_pop_node_bucket();
    rk_pop_res_bucket();
    return ret;
}
