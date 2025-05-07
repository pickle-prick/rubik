#define SCENE Scene_3_

/////////////////////////////////////////////////////////////////////////////////////////
// Enums

typedef enum S3_Texture2DKind
{
  S3_Texture2DKind_BlackStone,
  S3_Texture2DKind_WhiteStone,
  S3_Texture2DKind_COUNT,
} S3_Texture2DKind;

typedef enum S3_CellKind
{
  S3_CellKind_Black,
  S3_CellKind_White,
  S3_CellKind_COUNT,
} S3_CellKind;

/////////////////////////////////////////////////////////////////////////////////////////
// Scene types

typedef struct S3_Cell S3_Cell;
typedef struct S3_Grid S3_Grid;
struct S3_Cell
{
  U64 x;
  U64 y;
  U64 idx;
  S3_CellKind kind;
  B32 is_revealed;
  U64 liberties;
  U64 strength;
  S3_Grid *grid;
  RK_Node *node;
};

typedef struct S3_Kernal S3_Kernal;
struct S3_Kernal
{
  S3_Kernal *next;
  S3_Kernal *prev;

  // Vec2U64 atom_size; // 1x1, 2x2, 3x3 e.g.
  Vec2U64 size; // 2x2, 2x1 or 1x1 e.g.
  Vec2U64 step;
  B32 disabled;

  U32 match[4*4]; // support max 4x4 matrix
  U32 values[4*4]; // support max 4x4 matrix 
};

struct S3_Grid
{
  Vec2U32 size;
  U64 cell_count;
  S3_Cell *cells;

  U64 kernal_count;
  S3_Kernal *first_kernal;
  S3_Kernal *last_kernal;
};

typedef struct S3_Scene S3_Scene;
struct S3_Scene
{
  RK_Texture2D *textures[S3_Texture2DKind_COUNT];
  S3_Grid *grid;
  // ui stuffs
  struct
  {
    Rng2F32 rect;
    B32 show;
  } debug_ui;
};

typedef struct S3_Camera S3_Camera;
struct S3_Camera
{
  Vec2F32 viewport_size;
};

typedef struct S3_Dino S3_Dino;
struct S3_Dino
{
  Dir2 face_direction;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Helper Functions

internal S3_Kernal *
s3_push_kernal(S3_Grid *g)
{
  Arena *arena = rk_top_node_bucket()->arena_ref;
  S3_Kernal *ret = push_array(arena, S3_Kernal, 1);
  DLLPushBack(g->first_kernal, g->last_kernal, ret);
  g->kernal_count++;
  return ret;
}

internal void
s3_run_grid_kernals(S3_Grid *g)
{
  Vec2U32 grid_size = g->size;

  // run kernals
  for(S3_Kernal *k = g->first_kernal; k != 0; k = k->next)
  {
    for(U64 y = 0; y <= (grid_size.y-k->step.y); y += k->step.y)
    {
      for(U64 x = 0; x <= (grid_size.x-k->step.x); x += k->step.x)
      {
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Update Functions

RK_NODE_CUSTOM_UPDATE(s3_fn_debug_ui)
{
  S3_Scene *s = node->custom_data;
  S3_Grid *grid = s->grid;

  // TODO(XXX): gather some data (only for test, we should cached these values)
  U64 black_count = 0;
  U64 white_count = 0;
  for(U64 i = 0; i < grid->cell_count; i++)
  {
    S3_Cell *cell = &grid->cells[i];
    if(cell->is_revealed)
    {
      if(cell->kind == S3_CellKind_Black)
      {
        black_count++;
      }
      else
      {
        white_count++;
      }
    }
  }

  RK_UI_Pane(&s->debug_ui.rect, &s->debug_ui.show, str8_lit("GAME3"))
  {
    RK_UI_Tab(str8_lit("scene"), &s->debug_ui.show, ui_em(0.3,0), ui_em(0.3,0))
    {

      ui_set_next_flags(UI_BoxFlag_DrawDropShadow);
      if(ui_clicked(ui_buttonf("reset")))
      {
        // TODO(XXX)
      }

      ui_spacer(ui_em(0.3,0));

      ui_set_next_flags(UI_BoxFlag_DrawDropShadow);
      if(ui_clicked(ui_buttonf("run")))
      {
        s3_run_grid_kernals(s->grid);
      }

      ui_spacer(ui_em(0.3,0));

      UI_Row
      {
        ui_labelf("points");
        ui_spacer(ui_pct(1.0, 0.0));
        ui_labelf("%I64u / %I64u", black_count, grid->cell_count);
        ui_labelf("%.2f / 100", ((F32)black_count*100.0f)/grid->cell_count);
      }
    }
  }

  // highlight current operated cell 
  {
    // TODO(XXX): gez, this is cubesome, I wish there is a better way
    U64 hot_cell_idx = 0;
    S3_Cell *cell = &grid->cells[hot_cell_idx];
    Rng2F32 rect = rk_rect_from_sprite2d(cell->node->sprite2d);
    // NOTE(k): we are assuming orthographic projection here, so we don't divide xyz by w
    Vec4F32 p0_src = {rect.x0, rect.y0, 0, 1.0};
    Vec4F32 p0_dst = transform_4x4f32(ctx->proj_view_m, p0_src);
    // ndc to screen
    Vec2F32 half_window_dim = scale_2f32(rk_state->window_dim, 0.5);
    Vec2F32 half_viewport_dim = scale_2f32(v2f32(1800,1800), 0.5);
    p0_dst.x = p0_dst.x * half_viewport_dim.x + half_viewport_dim.x;
    p0_dst.y = p0_dst.y * half_viewport_dim.y + half_viewport_dim.y;
    Vec2F32 off = {p0_src.x-p0_dst.x, p0_src.y-p0_dst.y};
    off = add_2f32(off, v2f32(half_window_dim.x-half_viewport_dim.x, half_window_dim.y-half_viewport_dim.y));
    for(U64 i = 0; i < 4; i++)
    {
      rect.v[i] = add_2f32(off, rect.v[i]);
    }
    UI_Flags(UI_BoxFlag_Floating|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground)
      UI_Rect(rect)
      {
        ui_build_box_from_stringf(0, "hot");
      }
  }
}

RK_NODE_CUSTOM_UPDATE(s3_fn_camera)
{
  RK_Transform3D *transform = &node->node3d->transform;
  RK_Camera3D *camera = node->camera3d;

  Vec2F32 viewport_screen_dim = v2f32(300*6,300*6);
  Vec2F32 half_viewport_screen_dim = scale_2f32(viewport_screen_dim, 0.5);
  Vec2F32 window_dim = rk_state->window_dim;
  Vec2F32 half_window_dim = scale_2f32(window_dim, 0.5);

  // reset camera screen viewport
  camera->viewport.x0 = half_window_dim.x-half_viewport_screen_dim.x;
  camera->viewport.x1 = camera->viewport.x0+viewport_screen_dim.x;
  camera->viewport.y0 = half_window_dim.y-half_viewport_screen_dim.y;
  camera->viewport.y1 = camera->viewport.y0+viewport_screen_dim.y;

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
  }
}

RK_NODE_CUSTOM_UPDATE(s3_fn_grid)
{
  S3_Grid *grid = node->custom_data;

  for(U64 i = 0; i < grid->cell_count; i++)
  {
    S3_Cell *cell = &grid->cells[i];
    if(cell->is_revealed)
    {
      S3_CellKind kind = cell->kind;
      B32 revealed = 0;
      U64 row_start_idx = (cell->idx/grid->size.x) * grid->size.x;
      U64 row_end_idx = row_start_idx + grid->size.x - 1;

      // collect neighbors
      S64 up_idx = (S64)cell->idx - grid->size.x;
      S64 down_idx = (S64)cell->idx + grid->size.x;
      S64 left_idx = (S64)cell->idx - 1;
      S64 right_idx = (S64)cell->idx + 1;

      S3_Cell *up = up_idx > 0 ? &grid->cells[up_idx] : 0;
      S3_Cell *down = (down_idx < grid->cell_count) ? &grid->cells[down_idx] : 0;
      S3_Cell *left = (row_start_idx <= left_idx && left_idx <= row_end_idx) ? &grid->cells[left_idx] : 0;
      S3_Cell *right = (row_start_idx <= right_idx && right_idx <= row_end_idx) ? &grid->cells[right_idx] : 0;

#if 0
      if(cell->is_revealed)
      {
        S3_Cell *neighbors[4] = {up,left,right,down};
        for(U64 j = 0; j < 4; j++)
        {
          S3_Cell *neighbor = neighbors[j];
          if(neighbor &&
              neighbor->is_revealed &&
              neighbor->kind != cell->kind &&
              (neighbor->strength-cell->strength) > 2)
          {
            cell->kind = neighbor->kind;
          }
        }
      }
#endif
    }
  }
}

RK_NODE_CUSTOM_UPDATE(s3_fn_cell)
{
  S3_Scene *scene_data = rk_node_from_handle(scene->root)->custom_data;
  RK_Sprite2D *sprite2d = node->sprite2d;
  S3_Cell *cell = node->custom_data;

  S3_Grid *grid = cell->grid;

  U64 row_start_idx = (cell->idx/grid->size.x) * grid->size.x;
  U64 row_end_idx = row_start_idx + grid->size.x - 1;

  // check neighbors
  S64 up_idx = (S64)cell->idx - grid->size.x;
  S64 down_idx = (S64)cell->idx + grid->size.x;
  S64 left_idx = (S64)cell->idx - 1;
  S64 right_idx = (S64)cell->idx + 1;

  S3_Cell *up = up_idx > 0 ? &grid->cells[up_idx] : 0;
  S3_Cell *down = (down_idx < grid->cell_count) ? &grid->cells[down_idx] : 0;
  S3_Cell *left = (row_start_idx <= left_idx && left_idx <= row_end_idx) ? &grid->cells[left_idx] : 0;
  S3_Cell *right = (row_start_idx <= right_idx && right_idx <= row_end_idx) ? &grid->cells[right_idx] : 0;
  S3_Cell *neighbors[4] = {up,down,left,right};

  F32 fast_rate = 1 - pow_f32(2, (-50.f * rk_state->dt_sec));
  F32 vast_rate = 1 - pow_f32(2, (-60.f * rk_state->dt_sec));
  F32 fish_rate = 1 - pow_f32(2, (-40.f * rk_state->dt_sec));
  F32 slow_rate = 1 - pow_f32(2, (-30.f * rk_state->dt_sec));
  F32 slug_rate = 1 - pow_f32(2, (-15.f * rk_state->dt_sec));
  F32 slaf_rate = 1 - pow_f32(2, (-8.f  * rk_state->dt_sec));

  if(rk_key_match(scene->hot_key, node->key))
  {
    sprite2d->color.w += fast_rate * (0.3-sprite2d->color.w);
  }
  else
  {
    sprite2d->color.w = 1;
  }

  if(rk_key_match(scene->hot_key, node->key) && rk_state->sig.f & UI_SignalFlag_RightPressed)
  {
    cell->is_revealed = 1;
    cell->kind = !cell->kind;
    sprite2d->tex = scene_data->textures[cell->kind];
  }

  if(cell->is_revealed)
  {
    sprite2d->tex = scene_data->textures[cell->kind];
  }

  if(rk_key_match(scene->hot_key, node->key) &&
      rk_state->sig.f & UI_SignalFlag_LeftPressed &&
      !cell->is_revealed)
  {
    // node->hot_t = 1;
    S3_CellKind kind = cell->kind;

    S32 black_count = 0;
    S32 white_count = 0;
    if(up && up->is_revealed)
    {
      if(up->kind == S3_CellKind_Black) 
      {
        black_count++;
      }
      else
      {
        white_count++;
      }
    }
    if(down && down->is_revealed)
    {
      if(down->kind == S3_CellKind_Black) 
      {
        black_count++;
      }
      else
      {
        white_count++;
      }
    }
    if(left && left->is_revealed)
    {
      if(left->kind == S3_CellKind_Black) 
      {
        black_count++;
      }
      else
      {
        white_count++;
      }
    }
    if(right && right->is_revealed)
    {
      if(right->kind == S3_CellKind_Black) 
      {
        black_count++;
      }
      else
      {
        white_count++;
      }
    }
    // if((black_count-white_count) >= 2)
    // {
    //     kind = S3_CellKind_Black;
    // }
    // if((white_count-black_count) >= 2)
    // {
    //     kind = S3_CellKind_White;
    // }
    cell->kind = kind;
    cell->is_revealed = 1;
  }

  U64 neighbor_count = 0;
  for(U64 i = 0; i < 4; i++)
  {
    S3_Cell *neighbor = neighbors[i];
    if(neighbor && neighbor->is_revealed && neighbor->kind == cell->kind)
    {
      neighbor_count++;
    }
  }

  U64 strength = neighbor_count+1;
  cell->strength = strength;

  Temp scratch = scratch_begin(0,0);
  if(cell->is_revealed)
  {

    rk_sprite2d_equip_string(rk_frame_arena(), sprite2d,
        push_str8f(scratch.arena, "%I64u", strength),
        rk_state->cfg_font_tags[RK_FontSlot_Game],
        sprite2d->size.y, v4f32(1,1,1,1), 4, F_RasterFlag_Smooth);
  }
  else
  {
    sprite2d->string.size = 0;
  }
  scratch_end(scratch);
}

RK_NODE_CUSTOM_UPDATE(fn_player_dino)
{
  RK_AnimatedSprite2D *sprite2d = node->animated_sprite2d;
  S3_Dino *dino = node->custom_data;
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

/////////////////////////////////////////////////////////////////////////////////////////
// Scene entry

internal RK_Scene *
rk_scene_entry__3()
{
  RK_Scene *ret = rk_scene_alloc(str8_lit("2d_demo"), str8_lit("./src/rubik/scenes/3/default.rscn"));
  rk_push_node_bucket(ret->node_bucket);
  rk_push_res_bucket(ret->res_bucket);

  /////////////////////////////////////////////////////////////////////////////////////
  // scene settings

  ret->omit_grid = 1;
  ret->omit_gizmo3d = 1;
  ret->omit_light = 1;

  // 2d viewport
  Rng2F32 viewport_screen = {0,0,300,300};
  Vec2F32 viewport_screen_dim = dim_2f32(viewport_screen);
  Rng2F32 viewport_world = {0,0,300,300};
  Vec2F32 viewport_world_dim = dim_2f32(viewport_world);

  /////////////////////////////////////////////////////////////////////////////////////
  // load resource

  RK_SpriteSheet *doux_spritesheet = rk_spritesheet_from_image(str8_lit("./textures/DinoSprites - doux.png"), str8_lit("./textures/DinoSprites.json"));
  RK_Texture2D *white_stone_tex = rk_tex2d_from_image(str8_lit("./textures/white_stone.png"));
  RK_Texture2D *black_stone_tex = rk_tex2d_from_image(str8_lit("./textures/black_stone.png"));

  RK_Node *root = rk_build_node3d_from_stringf(0,0, "root");
  S3_Scene *scene_data = rk_node_push_custom_data(root, S3_Scene);
  scene_data->textures[S3_Texture2DKind_BlackStone] = black_stone_tex;
  scene_data->textures[S3_Texture2DKind_WhiteStone] = white_stone_tex;
  scene_data->debug_ui.rect = r2f32p(rk_state->window_rect.x1/2.0,
      rk_state->window_rect.y1/2.0,
      rk_state->window_rect.x1/2.0 + 300,
      rk_state->window_rect.y1/2.0 + 300);

  scene_data->debug_ui.show = 1;
  // allocate kernals 
  rk_node_push_fn(root, s3_fn_debug_ui);

  /////////////////////////////////////////////////////////////////////////////////////
  // build node tree

  RK_Parent_Scope(root)
  {
    // create the orthographic camera 
    RK_Node *main_camera = rk_build_camera3d_from_stringf(0, 0, "camera2d");
    {
      main_camera->camera3d->viewport = viewport_screen;
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
      rk_node_push_fn(main_camera, s3_fn_camera);
    }
    ret->active_camera = rk_handle_from_node(main_camera);

    Vec2U32 cell_count = {9,9};
    Vec2F32 cell_size = v2f32(300.f/cell_count.x,300.f/cell_count.y);
    Vec2F32 grid_world_size = {cell_size.x*cell_count.x, cell_size.y*cell_count.y};

    // grid
    RK_Node *grid = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,0, "grid");
    grid->sprite2d->size = grid_world_size;
    grid->node2d->transform.position = v2f32(viewport_world_dim.x/2.f, viewport_world_dim.y/2.0f);
    grid->sprite2d->color = v4f32(0.9,0.9,0.9,1);
    grid->sprite2d->omit_texture = 1;
    grid->sprite2d->anchor = RK_Sprite2DAnchorKind_Center;
    rk_node_push_fn(grid, s3_fn_grid);
    S3_Grid *grid_data = rk_node_push_custom_data(grid, S3_Grid);
    grid_data->size = cell_count;
    grid_data->cells = push_array(ret->arena, S3_Cell, cell_count.x*cell_count.y);
    grid_data->cell_count = cell_count.x * cell_count.y;

    {
      S3_Kernal *k    = s3_push_kernal(grid_data);
      k->size         = (Vec2U64){2,1};
      k->step         = (Vec2U64){1,1};
    }
    scene_data->grid = grid_data;

    // stones
    F32 y = -(grid_world_size.y/2.0)+cell_size.y/2.0;
    RK_Parent_Scope(grid) for(U64 j = 0; j < cell_count.y; j++)
    {
      F32 x = -(grid_world_size.x/2.0) + cell_size.x/2.0;
      for(U64 i = 0; i < cell_count.x; i++)
      {
        RK_Node *cell = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,0, "cell_%d_%d", i,j);
        cell->sprite2d->size = cell_size;
        cell->node2d->transform.position = v2f32(x,y);
        cell->node2d->transform.scale = v2f32(0.9,0.9);
        cell->sprite2d->color = v4f32(0.3,0.3,0.3,1);
        cell->sprite2d->tex = 0;
        cell->sprite2d->anchor = RK_Sprite2DAnchorKind_Center;
        cell->flags |= RK_NodeFlag_DrawBorder|RK_NodeFlag_DrawHotEffects;
        rk_node_push_fn(cell, s3_fn_cell);
        x += cell_size.x;
        S3_Cell *cell_data = &grid_data->cells[j*cell_count.x + i];
        cell_data->x = i;
        cell_data->y = j;
        cell_data->idx = grid_data->size.x*j + i;
        cell_data->kind = (rand() % 2) == 1;
        cell_data->grid = grid_data;
        cell_data->node = cell;
        cell->custom_data = cell_data;
      }
      y += cell_size.y;
    }

    RK_Node *dino = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_AnimatedSprite2D,0, "dino");
    dino->node2d->transform.position = v2f32(3,3);
    dino->animated_sprite2d->sheet = doux_spritesheet;
    dino->animated_sprite2d->is_animating = 1;
    dino->animated_sprite2d->curr_tag = 0; // idle
    dino->animated_sprite2d->loop = 0;
    dino->node2d->z_index = -1;
    rk_node_push_fn(dino, fn_player_dino);
    S3_Dino *dino_data = rk_node_push_custom_data(dino, S3_Dino);
    dino_data->face_direction = Dir2_Right;
  }

  ret->root = rk_handle_from_node(root);
  rk_pop_node_bucket();
  rk_pop_res_bucket();
  return ret;
}

#undef SCENE
