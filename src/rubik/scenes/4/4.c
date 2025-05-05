typedef struct S4_Scene S4_Scene;
struct S4_Scene
{
};

typedef struct S4_Camera S4_Camera;
struct S4_Camera
{
  Rng2F32 viewport_world;
};

RK_NODE_CUSTOM_UPDATE(s4_fn_person)
{
  RK_AnimatedSprite2D *sprite2d = node->animated_sprite2d;
  RK_SpriteSheet *sheet = sprite2d->sheet;
  RK_Transform2D *transform = &node->node2d->transform;

  Vec2F32 dir = {0};
  Dir2Flags dir_flag = 0;
  if(os_key_is_down(OS_Key_Left))
  {
    dir.x += -1.0;
  }
  if(os_key_is_down(OS_Key_Right))
  {
    dir.x += 1.0;
  }
  if(os_key_is_down(OS_Key_Up))
  {
    dir.y += -1.0;
  }
  if(os_key_is_down(OS_Key_Down))
  {
    dir.y += 1.0;
  }

  if(dir.x > 0)
  {
    dir_flag |= Dir2Flag_Right;
  }
  if(dir.x < 0)
  {
    dir_flag |= Dir2Flag_Left;
  }
  if(dir.y > 0)
  {
    dir_flag |= Dir2Flag_Down;
  }
  if(dir.y < 0)
  {
    dir_flag |= Dir2Flag_Up;
  }

  B32 is_moving = length_2f32(dir) > 0.0;

  if(is_moving)
  {
    // find tag based on the current direction
    String8 tag_name = {0};

    if(dir_flag == Dir2Flag_Left)
    {
      tag_name = str8_lit("running_left");
    }
    else if(dir_flag == Dir2Flag_Right)
    {
      tag_name = str8_lit("running_right");
    }
    else if(dir_flag == Dir2Flag_Up)
    {

      tag_name = str8_lit("running_up");
    }
    else if(dir_flag == Dir2Flag_Down)
    {

      tag_name = str8_lit("running_down");
    }
    else if(dir_flag == Dir2Flag_UpLeft)
    {

      tag_name = str8_lit("running_upleft");
    }
    else if(dir_flag == Dir2Flag_UpRight)
    {

      tag_name = str8_lit("running_upright");
    }
    else if(dir_flag == Dir2Flag_DownLeft)
    {
      tag_name = str8_lit("running_downleft");
    }
    else if(dir_flag== Dir2Flag_DownRight)
    {

      tag_name = str8_lit("running_downright");
    }
    else {assert(0);}

    for(U64 i = 0; i < sheet->tag_count; i++)
    {
      if(str8_match(sheet->tags[i].name, tag_name, 0))
      {
        sprite2d->curr_tag = i;
      }
    }
    sprite2d->loop = 1;
    sprite2d->is_animating = 1;
  }
  else
  {
    // NOTE(k): we don't have idle animation for now
    // stop animation
    sprite2d->is_animating = 0;
  }

  F32 v = 1.0;
  Vec2F32 dist = {v*dir.x, v*dir.y};
  transform->position = add_2f32(transform->position, dist);
}

RK_NODE_CUSTOM_UPDATE(s4_fn_tilemap)
{
  ProfBeginFunction();
  RK_TileMap *tilemap = node->tilemap;
  RK_Transform2D transform = node->node2d->transform;

  // mouse ndc pos
  F32 mox_ndc = (rk_state->cursor.x / rk_state->window_dim.x) * 2.f - 1.f;
  F32 moy_ndc = (rk_state->cursor.y / rk_state->window_dim.y) * 2.f - 1.f;
  Vec4F32 mouse_in_world_4 = transform_4x4f32(ctx->proj_view_inv_m, v4f32(mox_ndc, moy_ndc, 1., 1.));
  Vec2F32 mouse_in_world = v2f32(mouse_in_world_4.x, mouse_in_world_4.y);

  Vec2F32 mouse_relative = sub_2f32(mouse_in_world, transform.position);
  Vec2F32 map_coord_src = transform_2x2f32(tilemap->mat_inv, mouse_relative);
  Vec2U32 map_coord = {(U32)round_f32(map_coord_src.x), (U32)round_f32(map_coord_src.y)};

  if(map_coord.x >= 0 && map_coord.x < tilemap->size.x && map_coord.y >= 0 && map_coord.y < tilemap->size.y)
  {
    U64 idx = (map_coord.y*tilemap->size.x + map_coord.x);
    // TODO: we only have one layer for now
    RK_Node *tile = node->first->first;
    for(U64 i = 0; i < idx && tile != 0; i++)
    {
      tile = tile->next;
    }
    if(tile)
    {
      scene->hot_key = tile->key;
    }
  }
  ProfEnd();
}

RK_NODE_CUSTOM_UPDATE(s4_camera_fn)
{
  S4_Camera *camera = node->custom_data;

  if(rk_state->sig.f & UI_SignalFlag_MiddleDragging)
  {
    Vec2F32 mouse_delta = rk_state->cursor_delta;
    Vec2F32 v = scale_2f32(mouse_delta, 0.5);
    camera->viewport_world.p0 = sub_2f32(camera->viewport_world.p0, v);
    camera->viewport_world.p1 = sub_2f32(camera->viewport_world.p1, v);
    node->camera3d->orthographic.top    = camera->viewport_world.y0;
    node->camera3d->orthographic.bottom = camera->viewport_world.y1;
    node->camera3d->orthographic.left   = camera->viewport_world.x0;
    node->camera3d->orthographic.right  = camera->viewport_world.x1;
  }
  if(rk_state->sig.scroll.x != 0 || rk_state->sig.scroll.y != 0)
  {
    camera->viewport_world = pad_2f32(camera->viewport_world, -30.*rk_state->sig.scroll.y);
    // Vec3F32 dist = scale_3f32(f, rk_state->sig.scroll.y/3.f);
    // transform->position = add_3f32(dist, transform->position);
    node->camera3d->orthographic.top    = camera->viewport_world.y0;
    node->camera3d->orthographic.bottom = camera->viewport_world.y1;
    node->camera3d->orthographic.left   = camera->viewport_world.x0;
    node->camera3d->orthographic.right  = camera->viewport_world.x1;
  }

  // TODO(XXX): keep the viewport w/h ratio matchs the current window
  if(rk_state->window_res_changed)
  {
    F32 ratio = rk_state->last_window_dim.y / rk_state->last_window_dim.x;
    Vec2F32 viewport_dim = dim_2f32(camera->viewport_world);
    F32 new_height = viewport_dim.x * ratio;

    camera->viewport_world.y1 = camera->viewport_world.y0 + new_height;
    node->camera3d->orthographic.top    = camera->viewport_world.y0;
    node->camera3d->orthographic.bottom = camera->viewport_world.y1;
    node->camera3d->orthographic.left   = camera->viewport_world.x0;
    node->camera3d->orthographic.right  = camera->viewport_world.x1;
  }
}

RK_NODE_CUSTOM_UPDATE(s4_tile)
{
  if(rk_key_match(scene->hot_key, node->key))
  {
    RK_Parent_Scope(node)
    {
      RK_Node *n = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,
                                              RK_NodeFlag_Transient|RK_NodeFlag_Float,
                                              "tile_hover");
      n->sprite2d->anchor = node->sprite2d->anchor;
      n->sprite2d->size = node->sprite2d->size;
      n->sprite2d->color = v4f32(1,1,0,0.6);
      n->sprite2d->color.w = mix_1f32(0., 0.3, node->hot_t);
      n->sprite2d->omit_texture = 1;
      n->node2d->transform = node->node2d->transform;
      n->node2d->z_index = -1;
    }
  }
  // else
  // {
  //   if(node->first != 0)
  //   {
  //     RK_NodeBucket *node_bucket = rk_top_node_bucket();
  //     SLLStackPush(node_bucket->first_to_free_node, node->first);
  //   }
  // }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Update Functions

/////////////////////////////////////////////////////////////////////////////////////////
// Scene entry

internal RK_Scene *
rk_scene_entry__4()
{
  RK_Scene *ret = rk_scene_alloc(str8_lit("isometric"), str8_lit("./src/rubik/scenes/4/default.rscn"));
  rk_push_node_bucket(ret->node_bucket);
  rk_push_res_bucket(ret->res_bucket);

  /////////////////////////////////////////////////////////////////////////////////////
  // scene settings

  ret->omit_grid = 1;
  ret->omit_gizmo3d = 1;
  ret->omit_light = 1;

  // 2d viewport
  // Rng2F32 viewport_screen = {0,0,600,600};
  // Vec2F32 viewport_screen_dim = dim_2f32(viewport_screen);
  Rng2F32 viewport_world = rk_state->window_rect;
  // Vec2F32 viewport_world_dim = dim_2f32(viewport_world);

  RK_Node *root = rk_build_node3d_from_stringf(0,0, "root");

  /////////////////////////////////////////////////////////////////////////////////////
  // load resource

  U64 tile_texture_count = 0;
  RK_Texture2D *tile_textures = rk_tex2d_from_dir(str8_lit("./textures/isometric-tiles/"), &tile_texture_count);
  RK_Texture2D *floor_texture = 0;
  for(U64 i = 0; i < tile_texture_count; i++)
  {
    if(str8_ends_with(tile_textures[i].name, str8_lit("tile-1.png"), 0))
    {
      floor_texture = &tile_textures[i];
    }
  }

  RK_SpriteSheet *character_sheet = rk_spritesheet_from_image(str8_lit("./textures/Chibi-character/Chibi-character-template_skin3_part1_by_AxulArt.png"), str8_lit("./textures/Chibi-character/Chibi-character-template_skin3_part1_by_AxulArt.json"));

  /////////////////////////////////////////////////////////////////////////////////////
  // build node tree

  RK_Parent_Scope(root)
  {
    // create the orthographic camera 
    RK_Node *main_camera = rk_build_camera3d_from_stringf(0, 0, "camera2d");
    {
      // main_camera->camera3d->viewport = viewport_screen;
      main_camera->camera3d->projection = RK_ProjectionKind_Orthographic;
      main_camera->camera3d->viewport_shading = RK_ViewportShadingKind_Material;
      main_camera->camera3d->polygon_mode = R_GeoPolygonKind_Fill;
      main_camera->camera3d->hide_cursor = 0;
      main_camera->camera3d->lock_cursor = 0;
      main_camera->camera3d->is_active = 1;
      main_camera->camera3d->zn = -0.1;
      main_camera->camera3d->zf = 1000; // support 1000 layers
      main_camera->camera3d->orthographic.top    = viewport_world.y0;
      main_camera->camera3d->orthographic.bottom = viewport_world.y1;
      main_camera->camera3d->orthographic.left   = viewport_world.x0;
      main_camera->camera3d->orthographic.right  = viewport_world.x1;

      main_camera->node3d->transform.position = v3f32(0,0,0);
      main_camera->custom_data = rk_node_push_custom_data(S4_Camera, 1);
      ((S4_Camera*)main_camera->custom_data)->viewport_world = viewport_world;
      rk_node_push_fn(main_camera, s4_camera_fn);
    }
    ret->active_camera = rk_handle_from_node(main_camera);

    // create people
    {
      RK_Node *p = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_AnimatedSprite2D, 0, "person_1");
      p->node2d->transform.position = v2f32(3,3);
      p->animated_sprite2d->sheet = character_sheet;
      p->animated_sprite2d->is_animating = 1;
      p->animated_sprite2d->curr_tag = 0; // idle
      p->animated_sprite2d->loop = 1;
      p->node2d->z_index = -1;
      rk_node_push_fn(p, s4_fn_person);
      // S3_Dino *dino_data = (dino->custom_data = rk_node_push_custom_data(S3_Dino, 1));
      // dino_data->face_direction = Dir2_Right;
    }

    // tilemap
    Vec2F32 tile_size = v2f32(64, 32);
    Vec2U32 tilemap_size = {30,30};
    RK_Node *tilemap_node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D|RK_NodeTypeFlag_TileMap, 0, "tilemap");
    rk_node_push_fn(tilemap_node, s4_fn_tilemap);
    tilemap_node->sprite2d->color = v4f32(1.,0.,0.,1.);
    tilemap_node->sprite2d->size = v2f32(tile_size.x*tilemap_size.x, tile_size.y*tilemap_size.y);
    RK_TileMap *tilemap = tilemap_node->tilemap;
    {
      Mat2x2F32 mat = mat_2x2f32(0.);
      mat.v[0][0] = 0.5*tile_size.x;
      mat.v[0][1] = 0.5*tile_size.y;
      mat.v[1][0] = -0.5*tile_size.x;
      mat.v[1][1] = 0.5*tile_size.y;

      tilemap->size = tilemap_size;
      tilemap->tile_size = tile_size;
      tilemap->mat = mat;
      tilemap->mat_inv = inverse_2x2f32(mat);
    }
    RK_Parent_Scope(tilemap_node)
    {
      RK_Node *layer_node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_TileMapLayer, 0, "tilemap_layer_0");
      RK_Parent_Scope(layer_node)
      {
        U64 i,j;
        U64 idx = 0;
        for(j = 0; j < tilemap->size.y; j++)
        {
          for(i = 0; i < tilemap->size.x; i++)
          {
            Vec2F32 pos = transform_2x2f32(tilemap->mat, v2f32(i, j));
            // pos.x = origin.x + i*(tile_size.x/2.0) - j*(tile_size.x/2.0);
            // pos.y = origin.y + i*(tile_size.y/2.0) + j*(tile_size.y/2.0);

            // RK_Texture2D *tex = &tile_textures[idx%tile_texture_count];
            RK_Texture2D *tex = floor_texture;

            RK_Node *n = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D, 0, "tile-%I64d-%I64d", i, j);
            n->sprite2d->tex = tex;
            n->sprite2d->anchor = RK_Sprite2DAnchorKind_Center;
            n->sprite2d->size = tilemap->tile_size;
            n->sprite2d->color = v4f32(0.3,0.3,0.3,1);
            n->node2d->transform.position = pos;
            n->flags |= RK_NodeFlag_DrawBorder|RK_NodeFlag_DrawHotEffects;
            idx++;
            rk_node_push_fn(n, s4_tile);
          }
        }
      }
    }
  }

  ret->root = rk_handle_from_node(root);
  rk_pop_node_bucket();
  rk_pop_res_bucket();
  return ret;
}
