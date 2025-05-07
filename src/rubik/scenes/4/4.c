typedef U64 S4_TileFlags;
#define S4_TileFlag_Water (S4_TileFlags)(1ull<<0)
#define S4_TileFlag_Ice   (S4_TileFlags)(1ull<<1)
#define S4_TileFlag_Dirty (S4_TileFlags)(1ull<<2)

typedef struct S4_Guy S4_Guy;
struct S4_Guy
{
  // states
  F32 social_drive;
  // basic needs
  F32 hunger;
  F32 thirsty;
  // wealth
  F32 money;
  F32 iq;
  F32 integraty;
  F32 ennery;
};

typedef struct S4_Tile S4_Tile;
struct S4_Tile
{
  S4_TileFlags flag;
  U64 i;
  U64 j;
};

typedef struct S4_Scene S4_Scene;
struct S4_Scene
{
  RK_Texture2D *textures[S3_Texture2DKind_COUNT];
  S3_Grid *grid;

  RK_Texture2D *tile_textures;
  U64 tile_texture_count;
  U64 curr_tile_texture_idx;

  S4_Guy *guys;
  U64 guy_count;

  RK_TileMap *tilemap;
  RK_TileMapLayer *tilemap_layers;
  U64 *tilemap_layer_count;
  S4_Tile **tiles;
  U64 tile_count;

  // editor view
  struct
  {
    Rng2F32 rect;
    B32 show;
  } debug_ui;
};

typedef struct S4_Camera S4_Camera;
struct S4_Camera
{
  Rng2F32 viewport_world;
};

internal Vec2U32
tile_coord_from_mouse(Mat4x4F32 proj_view_inv_m, Mat2x2F32 mat_inv, Vec2F32 tilemap_origin)
{
  // mouse ndc pos
  F32 mox_ndc = (rk_state->cursor.x / rk_state->window_dim.x) * 2.f - 1.f;
  F32 moy_ndc = (rk_state->cursor.y / rk_state->window_dim.y) * 2.f - 1.f;
  Vec4F32 mouse_in_world_4 = transform_4x4f32(proj_view_inv_m, v4f32(mox_ndc, moy_ndc, 1., 1.));
  Vec2F32 mouse_in_world = v2f32(mouse_in_world_4.x, mouse_in_world_4.y);

  Vec2F32 mouse_relative = sub_2f32(mouse_in_world, tilemap_origin);
  Vec2F32 map_coord_src = transform_2x2f32(mat_inv, mouse_relative);
  Vec2U32 ret = {(U32)round_f32(map_coord_src.x), (U32)round_f32(map_coord_src.y)};
  return ret;
}

RK_NODE_CUSTOM_UPDATE(s4_fn_tile_editor)
{
  S4_Scene *s = scene->custom_data;
  RK_Node *tilemap_node = rk_ptr_from_fat(s->tilemap);
  RK_TileMap *tilemap = tilemap_node->tilemap;
  Vec2U32 tile_coord = tile_coord_from_mouse(ctx->proj_view_inv_m, s->tilemap->mat_inv, tilemap_node->node2d->transform.position);
  RK_UI_Pane(&s->debug_ui.rect, &s->debug_ui.show, str8_lit("TileEditor"))
  {
    RK_UI_Tab(str8_lit("tile"), &s->debug_ui.show, ui_em(0.3,0), ui_em(0.3,0))
    {
      // ui_set_next_flags(UI_BoxFlag_DrawDropShadow);
      // if(ui_clicked(ui_buttonf("reset")))
      // {
      // }

      // ui_spacer(ui_em(0.3,0));

      // ui_set_next_flags(UI_BoxFlag_DrawDropShadow);
      // if(ui_clicked(ui_buttonf("run")))
      // {
      // }

      ui_spacer(ui_em(0.3,0));

      UI_Row
      {
        ui_labelf("textures");
        ui_spacer(ui_pct(1.0, 0.0));
        rk_ui_dropdown_begin(s->tile_textures[s->curr_tile_texture_idx].name);
        for(U64 i = 0; i < s->tile_texture_count; i++)
        {

          UI_Signal sig = ui_button(s->tile_textures[i].name);
          if(ui_hovering(sig))
          {
            UI_FixedX(sig.box->fixed_position.x+sig.box->fixed_size.x*(3./4.))
            UI_FixedY(sig.box->fixed_position.y+sig.box->fixed_size.y+1.0)
            UI_Flags(UI_BoxFlag_Floating)
            {
              rk_ui_img(str8_lit("preview"), v2f32(sig.box->fixed_size.x/4.0, sig.box->fixed_size.x/4.0), s->tile_textures[i].tex, s->tile_textures[i].size);
            }
          }

          if(ui_clicked(sig))
          {
            rk_ui_dropdown_hide();
            s->curr_tile_texture_idx = i;
          }
        }
        rk_ui_dropdown_end();
      }

      UI_Row
      {
        ui_labelf("coord");
        ui_spacer(ui_pct(1.0, 0.0));
        ui_labelf("%u %u", tile_coord.x, tile_coord.y);
      }
    }
  }

  // set hot_key
  if(tile_coord.x >= 0 && tile_coord.x < tilemap->size.x && tile_coord.y >= 0 && tile_coord.y < tilemap->size.y)
  {
    U64 idx = (tile_coord.y*tilemap->size.x + tile_coord.x);
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

    RK_Texture2D *tex = &s->tile_textures[s->curr_tile_texture_idx];
    U64 tile_idx = tile_coord.y*tilemap->size.x + tile_coord.x;
    RK_Node *tile_node = tilemap_node->first->first;
    // TODO(XXX): we should store these into array stored in tilemap
    for(U64 i = 0; i < idx; i++,tile_node=tile_node->next);

    RK_Parent_Scope(tile_node)
    {
      Vec2F32 pos = transform_2x2f32(tilemap->mat, v2f32(tile_coord.x, tile_coord.y));

      // TODO(XXX): still won't work
      // draw an overlay
      // {
      //   RK_Node *n = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,
      //                                           RK_NodeFlag_Transient|RK_NodeFlag_Float,
      //                                           "tile_hover_overlay");
      //   n->sprite2d->anchor = RK_Sprite2DAnchorKind_Center;
      //   n->sprite2d->size = tilemap->tile_size;
      //   n->sprite2d->color = v4f32(0,0,0,1);
      //   // n->sprite2d->color.w = mix_1f32(0., 0.3, node->hot_t);
      //   n->sprite2d->omit_texture = 1;
      //   n->node2d->transform.position = pos;
      //   n->node2d->z_index = n->node2d->z_index;
      // }

      // draw current tile selection
      {
        RK_Node *n = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,
                                                RK_NodeFlag_Transient|RK_NodeFlag_Float,
                                                "tile_hover");
        n->sprite2d->anchor = RK_Sprite2DAnchorKind_Center;
        n->sprite2d->size = tilemap->tile_size;
        // n->sprite2d->color = v4f32(1,1,0,0.6);
        // n->sprite2d->color.w = mix_1f32(0., 0.3, node->hot_t);
        n->sprite2d->tex = tex;
        n->sprite2d->omit_texture = 0;
        n->node2d->transform.position = pos;
        n->node2d->z_index = n->node2d->z_index;
      }
    }

    // if(rk_state->sig.f & UI_SignalFlag_Clicked)
    if(os_key_is_down(OS_Key_LeftMouseButton) && rk_state->sig.f&UI_SignalFlag_Hovering)
    {
      tile_node->sprite2d->tex = tex;
    }
  }

}

internal void s4_move_guy(RK_Node *node, Vec2F32 dir, F32 delta_secs)
{
  RK_AnimatedSprite2D *sprite2d = node->animated_sprite2d;
  RK_SpriteSheet *sheet = sprite2d->sheet;
  RK_Transform2D *transform = &node->node2d->transform;

  Dir2Flags dir_flag = 0;
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

  F32 v = 100;
  if(dir.x != 0 && dir.y != 0) dir = normalize_2f32(dir);
  Vec2F32 dist = {v*dir.x*delta_secs, v*dir.y*delta_secs};
  transform->position = add_2f32(transform->position, dist);
}

RK_NODE_CUSTOM_UPDATE(s4_fn_guy)
{
  RK_AnimatedSprite2D *sprite2d = node->animated_sprite2d;
  RK_SpriteSheet *sheet = sprite2d->sheet;
  RK_Transform2D *transform = &node->node2d->transform;
  // s4_move_guy(node, dir, ctx->dt_sec);
}

RK_NODE_CUSTOM_UPDATE(s4_fn_guy_controlled)
{
  RK_AnimatedSprite2D *sprite2d = node->animated_sprite2d;
  RK_SpriteSheet *sheet = sprite2d->sheet;
  RK_Transform2D *transform = &node->node2d->transform;

  Vec2F32 dir = {0};
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
}

RK_NODE_CUSTOM_UPDATE(s4_fn_tilemap)
{
  ProfBeginFunction();
  RK_TileMap *tilemap = node->tilemap;
  RK_Transform2D transform = node->node2d->transform;
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
    // TODO(XXX): padding won't keep the ratio intact
    camera->viewport_world = pad_2f32(camera->viewport_world, -30.*rk_state->sig.scroll.y);
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

  // scene data
  S4_Scene *scene = rk_scene_push_custom_data(ret, S4_Scene);
  {
    scene->debug_ui.rect = r2f32p(rk_state->window_rect.x1/2.0,
                                  rk_state->window_rect.y1/2.0,
                                  rk_state->window_rect.x1/2.0 + 900,
                                  rk_state->window_rect.y1/2.0 + 600);
    scene->debug_ui.show = 1;
  }

  // 2d viewport
  // Rng2F32 viewport_screen = {0,0,600,600};
  // Vec2F32 viewport_screen_dim = dim_2f32(viewport_screen);
  Rng2F32 viewport_world = rk_state->window_rect;
  // Vec2F32 viewport_world_dim = dim_2f32(viewport_world);

  RK_Node *root = rk_build_node3d_from_stringf(0,0, "root");
  rk_node_push_fn(root, s4_fn_tile_editor);

  /////////////////////////////////////////////////////////////////////////////////////
  // load resource

  U64 tile_texture_count = 0;
  RK_Texture2D *tile_textures = rk_tex2d_from_dir(str8_lit("./textures/isometric-tiles-2/"), &tile_texture_count);
  RK_Texture2D *floor_texture = 0;
  for(U64 i = 0; i < tile_texture_count; i++)
  {
    if(str8_ends_with(tile_textures[i].name, str8_lit("tile_000.png"), 0))
    {
      floor_texture = &tile_textures[i];
    }
  }
  scene->tile_textures = tile_textures;
  scene->tile_texture_count = tile_texture_count;

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
      rk_node_push_custom_data(main_camera, S4_Camera);
      ((S4_Camera*)main_camera->custom_data)->viewport_world = viewport_world;
      rk_node_push_fn(main_camera, s4_camera_fn);
    }
    ret->active_camera = rk_handle_from_node(main_camera);
    // tilemap
    Vec2U32 tilemap_size = {30,30};
    U64 tile_count = tilemap_size.x*tilemap_size.y;
    Vec2F32 tile_size = v2f32(100, 100); /* 2:1 ratio */
    RK_Node *tilemap_node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_TileMap, 0, "tilemap");
    rk_node_push_fn(tilemap_node, s4_fn_tilemap);
    RK_TileMap *tilemap = tilemap_node->tilemap;
    scene->tilemap = tilemap;
    scene->tiles = push_array(ret->arena, S4_Tile*, tile_count);
    scene->tile_count = tile_count;
    {
      Mat2x2F32 mat = mat_2x2f32(0.);
      mat.v[0][0] =  0.5*tile_size.x;
      mat.v[0][1] =  0.25*tile_size.y;
      mat.v[1][0] = -0.5*tile_size.x;
      mat.v[1][1] =  0.25*tile_size.y;

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
        U64 tile_idx = 0;
        for(j = 0; j < tilemap->size.y; j++)
        {
          for(i = 0; i < tilemap->size.x; i++)
          {
            Vec2F32 pos = transform_2x2f32(tilemap->mat, v2f32(i, j));
            // Vec2F32 pos = {0};
            // Vec2F32 origin = {0,0};
            // pos.x = origin.x + i*(tile_size.x/2.0) - j*(tile_size.x/2.0);
            // pos.y = origin.y + i*(tile_size.y/2.0) + j*(tile_size.y/2.0);

            // RK_Texture2D *tex = &tile_textures[idx%tile_texture_count];
            RK_Texture2D *tex = floor_texture;

            RK_Node *n = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D, 0, "tile-%I64d-%I64d", i, j);
            n->sprite2d->tex = tex;
            n->sprite2d->anchor = RK_Sprite2DAnchorKind_Center;
            n->sprite2d->size = tile_size;
            n->sprite2d->color = v4f32(0.3,0.3,0.3,1);
            n->node2d->transform.position = pos;
            // TODO(XXX): won't work for now
            n->flags |= RK_NodeFlag_DrawBorder|RK_NodeFlag_DrawHotEffects;
            rk_node_push_fn(n, s4_tile);
            S4_Tile *tile = rk_node_push_custom_data(n, S4_Tile);
            tile->i = i;
            tile->j = j;

            scene->tiles[i] = tile;
            tile_idx++;
          }
        }
      }
    }

    // create people
    for(U64 k = 0; k < 19; k++)
    {
      F32 i = ceil_f32(rand() % tilemap_size.x);
      F32 j = ceil_f32(rand() % tilemap_size.y);
      Vec2F32 pos = transform_2x2f32(tilemap->mat, v2f32(i, j));

      RK_Node *p = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_AnimatedSprite2D, 0, "person_1");
      p->node2d->transform.position = pos;
      p->animated_sprite2d->sheet = character_sheet;
      p->animated_sprite2d->is_animating = 1;
      p->animated_sprite2d->curr_tag = 0; // idle
      p->animated_sprite2d->loop = 1;
      p->animated_sprite2d->size = v2f32(character_sheet->size.x*0.3,character_sheet->size.y*0.3);
      p->node2d->z_index = -1;
      rk_node_push_fn(p, s4_fn_guy);
      S4_Guy *guy = rk_node_push_custom_data(p, S4_Guy);
      guy->hunger = 0;
      guy->integraty = 0;
      guy->iq = 0;
      guy->money = 0;
      guy->social_drive = 0;
      guy->thirsty = 0;
    }

  }

  ret->root = rk_handle_from_node(root);
  rk_pop_node_bucket();
  rk_pop_res_bucket();
  return ret;
}
