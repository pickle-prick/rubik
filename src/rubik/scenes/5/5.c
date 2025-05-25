/////////////////////////////////////////////////////////////////////////////////////////
// Basic Type/Enum

typedef struct S5_Camera S5_Camera;
struct S5_Camera
{
  Rng2F32 viewport_world;
  Rng2F32 viewport_world_target;
};

typedef struct S5_Scene S5_Scene;
struct S5_Scene
{
  RK_Handle sea;
  RK_Handle submarine;

  // editor view
  struct
  {
    Rng2F32 rect;
    B32 show;
  } debug_ui;
};

typedef struct S5_Submarine S5_Submarine;
struct S5_Submarine
{
  Vec2F32 v;
  F32 density;
  F32 min_density;
  F32 max_density;
  F32 drag;
};

typedef struct S5_Fish S5_Fish;
struct S5_Fish
{
  F32 max_depth;
  F32 min_depth;
  Vec2F32 target_position;
};

#define S5_SEA_FLUID_DENSITY 1025.0
#define S5_SCALE_WORLD_TO_METER 0.001

/////////////////////////////////////////////////////////////////////////////////////////
// Update Functions

RK_NODE_CUSTOM_UPDATE(s5_fn_game_ui)
{
  S5_Scene *s = scene->custom_data;
  RK_Node *sea_node = rk_node_from_handle(&s->sea);
  RK_Node *submarine_node = rk_node_from_handle(&s->submarine);
  S5_Submarine *submarine = submarine_node->custom_data;

  D_BucketScope(rk_state->bucket_rect)
  {
    UI_Box *container;
    UI_Rect(rk_state->window_rect) UI_ChildLayoutAxis(Axis2_Y)
    {
      container = ui_build_box_from_stringf(0, "game_ui");
    }

    UI_Parent(container) UI_PrefWidth(ui_pct(1.0,0.0))
    {
      ui_spacer(ui_px(9.0, 0.0));
      UI_Row
      {
        ui_spacer(ui_pct(1.0,0.));
        ui_set_next_pref_width(ui_text_dim(3, 1.0));
        ui_labelf("density: %f", submarine->density);
      }
      UI_Row
      {
        ui_spacer(ui_pct(1.0,0.));
        ui_set_next_pref_width(ui_text_dim(3, 1.0));
        ui_labelf("v: %f %f", submarine->v.x, submarine->v.y);
      }
    }
  }
}

RK_NODE_CUSTOM_UPDATE(s5_fn_submarine)
{
  // TODO(XXX): ajust these settings to make control feel more natural
  S5_Scene *s = scene->custom_data;
  S5_Submarine *submarine = node->custom_data;
  RK_Transform2D *transform = &node->node2d->transform;
  RK_Node *sea_node = rk_node_from_handle(&s->sea);
  RK_Sprite2D *sprite2d = node->sprite2d;

  F32 y_in = transform->position.y+sprite2d->size.y-sea_node->node2d->transform.position.y;
  y_in = Clamp(0, y_in, sprite2d->size.y);
  F32 volume = (sprite2d->size.x*S5_SCALE_WORLD_TO_METER) * (sprite2d->size.y*S5_SCALE_WORLD_TO_METER);
  F32 volume_displaced = sprite2d->size.x*y_in*S5_SCALE_WORLD_TO_METER*S5_SCALE_WORLD_TO_METER;

  F32 g = 9.8*1000.0; /* gravitational acceleration */
  F32 mass = submarine->density * volume;
  F32 mass_displaced = S5_SEA_FLUID_DENSITY * volume_displaced;

  // force accmulator
  Vec2F32 F = {0};

  // thrust
  if(os_key_is_down(OS_Key_Up))
  {
    F.y = -1000000;
  }
  if(os_key_is_down(OS_Key_Left))
  {
    F.x = -1000000;
  }
  if(os_key_is_down(OS_Key_Right))
  {
    F.x = 1000000;
  }
  if(os_key_is_down(OS_Key_Down))
  {
    F.y = 1000000;
  }

  // TODO(XXX): we should move these physics update into fixed_update
  F32 density = submarine->density;
  if(os_key_is_down(OS_Key_W))
  {
    density -= 300*ctx->dt_sec;
  }
  if(os_key_is_down(OS_Key_S))
  {
    density += 300*ctx->dt_sec;
  }
  density = Clamp(submarine->min_density, density, submarine->max_density);

  F32 drag = 6000;
  Vec2F32 v = submarine->v;
  F.y += (mass-mass_displaced)*g;
  F.x += -drag*v.x;
  F.y += -drag*v.y;
  Vec2F32 acc = {F.x/mass, F.y/mass};
  v.x += acc.x * ctx->dt_sec;
  v.y += acc.y * ctx->dt_sec;
  Vec2F32 pos = transform->position;
  pos.x += v.x * ctx->dt_sec;
  pos.y += v.y * ctx->dt_sec;

  submarine->v = v;
  transform->position = pos;
  submarine->density = density;
}

RK_NODE_CUSTOM_UPDATE(s5_fn_fish)
{
  S5_Scene *s = scene->custom_data;
  S5_Fish *fish = node->custom_data;
  RK_Transform2D *transform = &node->node2d->transform;
  RK_Node *submarine_node = rk_node_from_handle(&s->submarine);

  Vec2F32 ray = sub_2f32(submarine_node->node2d->transform.position, node->node2d->transform.position);
  Vec2F32 dir = normalize_2f32(ray);
  F32 dist = length_2f32(ray);

  F32 speed = 600.0;
  if(dist < 600.0)
  {
    Vec2F32 to_move = scale_2f32(dir, speed*ctx->dt_sec);
    transform->position = add_2f32(to_move, transform->position);
  }
  else
  {
    // pick a random direction to move
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Scene entry

internal RK_Scene *
rk_scene_entry__5()
{
  RK_Scene *ret = rk_scene_alloc();
  ret->name = str8_lit("submarine");
  ret->save_path = str8_lit("./src/rubik/scenes/4/default.tscn");
  ret->reset_fn = str8_lit("rk_scene_entry__5");

  rk_push_scene(ret);
  rk_push_node_bucket(ret->node_bucket);
  rk_push_res_bucket(ret->res_bucket);
  rk_push_handle_seed(ret->handle_seed);

  /////////////////////////////////////////////////////////////////////////////////////
  // scene settings

  ret->omit_grid = 1;
  ret->omit_gizmo3d = 1;
  ret->omit_light = 1;

  // scene data
  S5_Scene *scene = rk_scene_push_custom_data(ret, S5_Scene);
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

  // root node
  RK_Node *root = rk_build_node3d_from_stringf(0,0, "root");
  rk_node_push_fn(root, str8_lit("s5_fn_game_ui"));

  ///////////////////////////////////////////////////////////////////////////////////////
  // load resource

  RK_Handle tileset = rk_tileset_from_dir(str8_lit("./textures/isometric-tiles-2"), rk_key_zero());

  ///////////////////////////////////////////////////////////////////////////////////////
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
      S5_Camera *camera = rk_node_push_custom_data(main_camera, S5_Camera);
      camera->viewport_world = viewport_world;
      camera->viewport_world_target = viewport_world;
      rk_node_push_fn(main_camera, str8_lit("s4_fn_camera"));
    }
    ret->active_camera = rk_handle_from_node(main_camera);

    // sea
    RK_Node *sea_node = 0;
    {
      RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D, 0, "sea");
      node->node2d->transform.position = v2f32(0., 0.);
      node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
      node->sprite2d->size = v2f32(3000,3000);
      node->sprite2d->color = v4f32(0.,0.1,0.5,1.);
      node->sprite2d->omit_texture = 1;
      sea_node = node;
    }
    scene->sea = rk_handle_from_node(sea_node);

    // submarine
    RK_Node *submarine_node = 0;
    {
      RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D, 0, "submarine");
      node->node2d->transform.position = v2f32(900., -150.);
      node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
      node->sprite2d->size = v2f32(900,300);
      node->sprite2d->color = v4f32(0.,0.5,0.1,1.);
      node->sprite2d->omit_texture = 1;
      rk_node_push_fn(node, str8_lit("s5_fn_submarine"));
      submarine_node = node;

      S5_Submarine *submarine = rk_node_push_custom_data(node, S5_Submarine);
      submarine->drag = 0.96;
      submarine->density = S5_SEA_FLUID_DENSITY/2.0;
      submarine->min_density = S5_SEA_FLUID_DENSITY/2.0;
      submarine->max_density = S5_SEA_FLUID_DENSITY;
    }
    scene->submarine = rk_handle_from_node(submarine_node);

    // fish
    Rng1U32 spwan_range_x = {0, 3000};
    Rng1U32 spwan_range_y = {0, 3000};
    U32 spwan_dim_x = dim_1u32(spwan_range_x);
    U32 spwan_dim_y = dim_1u32(spwan_range_y);
    for(U64 i = 0; i < 300; i++)
    {
      F32 x = (rand()%spwan_dim_x) + spwan_range_x.min;
      F32 y = (rand()%spwan_dim_y) + spwan_range_y.min;
      RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D, 0, "fish_%I64u", i);
      node->node2d->transform.position = v2f32(x, y);
      node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
      node->sprite2d->size = v2f32(30,30);
      node->sprite2d->color = v4f32(0.6,0.0,0.1,1.);
      node->sprite2d->omit_texture = 1;
      rk_node_push_fn(node, str8_lit("s5_fn_fish"));
      submarine_node = node;
      S5_Fish *fish = rk_node_push_custom_data(node, S5_Fish);
      fish->min_depth = y - 30;
      fish->max_depth = y + 30;
    }
  }

  ret->root = rk_handle_from_node(root);
  rk_pop_scene();
  rk_pop_node_bucket();
  rk_pop_res_bucket();
  rk_pop_handle_seed();
  return ret;
}
