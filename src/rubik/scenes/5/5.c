/////////////////////////////////////////////////////////////////////////////////////////
// Basic Type/Enum

typedef U64 S5_EntityFlags;
#define S5_EntityFlag_Boid (S5_EntityFlags)(1ull<<0)

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

  // per-frame build artifacts
  Vec2F32 world_mouse;
  RK_Node **fishes;

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

  Vec2F32 vel;
  Vec2F32 acc;
  Vec2F32 target_vel;

  F32 motivation;
  F32 last_motivation_update_time;
};

#define S5_SEA_FLUID_DENSITY 1025.0
#define S5_SCALE_WORLD_TO_METER 0.001

/////////////////////////////////////////////////////////////////////////////////////////
// Helper Functions

internal Vec2F32
s5_world_position_from_mouse(Vec2F32 mouse, Vec2F32 resolution_dim, Mat4x4F32 proj_view_inv_m)
{
  // mouse ndc pos
  F32 mox_ndc = (mouse.x / resolution_dim.x) * 2.f - 1.f;
  F32 moy_ndc = (mouse.y / resolution_dim.y) * 2.f - 1.f;
  Vec4F32 mouse_in_world_4 = transform_4x4f32(proj_view_inv_m, v4f32(mox_ndc, moy_ndc, 1., 1.));
  Vec2F32 ret = v2f32(mouse_in_world_4.x, mouse_in_world_4.y);
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Update Functions

RK_NODE_CUSTOM_UPDATE(s5_fn_scene_begin)
{

  S5_Scene *s = scene->custom_data;
  RK_NodeBucket *node_bucket = scene->node_bucket;

  RK_Node **fishes = 0;
  for(U64 slot_idx = 0; slot_idx < node_bucket->hash_table_size; slot_idx++)
  {
    RK_NodeBucketSlot *slot = &node_bucket->hash_table[slot_idx];
    for(RK_Node *n = slot->first; n != 0; n = n->hash_next)
    {
      if(n->custom_flags & S5_EntityFlag_Boid)
      {
        darray_push(rk_frame_arena(), fishes, n);
      }
    }
  }
  s->fishes = fishes;

  s->world_mouse = s5_world_position_from_mouse(rk_state->cursor, rk_state->window_dim, ctx->proj_view_inv_m);

  for(U64 j = 0; j < darray_size(fishes); j++)
  {
    RK_Node *fish_node = fishes[j];
    S5_Fish *fish = fish_node->custom_data;
    RK_Transform2D *transform = &fish_node->node2d->transform;
    Vec2F32 size = fish_node->sprite2d->size;
    Vec2F32 position = fish_node->node2d->transform.position;

    Vec2F32 acc = {0};

    // move to cursor
    {
      F32 dist_self_to_mouse = length_2f32(sub_2f32(s->world_mouse, position));
      Vec2F32 dir_self_to_mouse = normalize_2f32(sub_2f32(s->world_mouse, position));

      F32 angle_rad = ((rand()/(F32)RAND_MAX) * (1.0/2) - (1.0/4));
      F32 cos_a = cos_f32(angle_rad);
      F32 sin_a = cos_f32(angle_rad);

      Vec2F32 dir = {
        dir_self_to_mouse.x*cos_a - dir_self_to_mouse.y*sin_a,
        dir_self_to_mouse.x*sin_a + dir_self_to_mouse.y*cos_a,
      };

      if(dist_self_to_mouse < size.x*6.3)
      {
        F32 w = 109000;
        Vec2F32 uf = scale_2f32(dir_self_to_mouse, -w);
        acc = add_2f32(acc, uf);
      }
      else
      {
        F32 w = 100000;
        Vec2F32 uf = scale_2f32(dir, w);
        acc = add_2f32(acc, uf);
      }
    }

    // separation
    for(U64 i = 0; i < darray_size(fishes); i++)
    {
      RK_Node *rhs_node = fishes[i];
      if(rhs_node != fish_node)
      {
        S5_Fish *rhs_fish = rhs_node->custom_data;
        Vec2F32 rhs_position = rhs_node->node2d->transform.position;

        Vec2F32 rhs_to_self = sub_2f32(position, rhs_position);
        F32 dist = length_2f32(rhs_to_self);

        F32 constraint_dist = size.x*1.5;
        if(dist < constraint_dist)
        {
          F32 w = 100000 * (dist/constraint_dist);
          // unit force (assuming mass is 1)
          Vec2F32 uf = scale_2f32(normalize_2f32(rhs_to_self), w);
          acc = add_2f32(acc, uf);
        }
      }
    }

    // alignment
    {
      Vec2F32 alignment_direction = fish->vel;
      U64 nearby_count = 0;
      for(U64 i = 0; i < darray_size(fishes); i++)
      {
        RK_Node *rhs_node = fishes[i];

        if(rhs_node != fish_node)
        {
          S5_Fish *rhs_fish = rhs_node->custom_data;
          Vec2F32 rhs_position = rhs_node->node2d->transform.position;

          Vec2F32 rhs_to_self = sub_2f32(position, rhs_position);
          F32 dist = length_2f32(rhs_to_self);

          if(dist < size.x*6)
          {
            nearby_count++;
            alignment_direction.x += rhs_fish->vel.x;
            alignment_direction.y += rhs_fish->vel.y;
          }
        }
      }
      if(nearby_count > 0)
      {
        alignment_direction.x /= (nearby_count+1);
        alignment_direction.y /= (nearby_count+1);

        if(alignment_direction.x != 0 || alignment_direction.y != 0)
        {
          Vec2F32 steer_alignment = normalize_2f32(sub_2f32(alignment_direction, fish->vel));
          F32 w = 2000;
          Vec2F32 uf = scale_2f32(steer_alignment, w);
          acc = add_2f32(acc, uf);
        }
      }
    }

    // cohesion
    {
      Vec2F32 center_of_mass = position;
      U64 nearby_count = 0;
      for(U64 i = 0; i < darray_size(fishes); i++)
      {
        RK_Node *rhs_node = fishes[i];

        if(rhs_node != fish_node)
        {
          S5_Fish *rhs_fish = rhs_node->custom_data;
          Vec2F32 rhs_position = rhs_node->node2d->transform.position;

          Vec2F32 rhs_to_self = sub_2f32(position, rhs_position);
          F32 dist = length_2f32(rhs_to_self);

          if(dist < size.x*6)
          {
            nearby_count++;
            center_of_mass.x += rhs_position.x;
            center_of_mass.y += rhs_position.y;
          }
        }
      }
      if(nearby_count > 0)
      {
        center_of_mass.x /= (nearby_count+1);
        center_of_mass.y /= (nearby_count+1);

        Vec2F32 steer_cohesion = normalize_2f32(sub_2f32(center_of_mass, position));
        F32 w = 15000;
        Vec2F32 uf = scale_2f32(steer_cohesion, w);
        acc = add_2f32(acc, uf);
      }
    }

    // F32 scale = 1.0 * (rand()/(F32)RAND_MAX) * (1);
    Vec2F32 vel = scale_2f32(acc, ctx->dt_sec*fish->motivation);
    fish->acc = acc;
    fish->target_vel = vel;
  }
}

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

    // TODO(XXX)
    // sea level indicator
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
  RK_Node **fishes = s->fishes;
  S5_Fish *fish = node->custom_data;
  RK_Transform2D *transform = &node->node2d->transform;
  Vec2F32 size = node->sprite2d->size;
  Vec2F32 position = node->node2d->transform.position;

  // smooth velocity
  // animating
  // TODO(XXX): compute this once at the beginning of the frame
  // F32 vast_rate = 1 - pow_f32(2, (-60.f * ui_state->animation_dt));
  // F32 fast_rate = 1 - pow_f32(2, (-50.f * ui_state->animation_dt));
  // F32 fish_rate = 1 - pow_f32(2, (-40.f * ui_state->animation_dt));
  // F32 slow_rate = 1 - pow_f32(2, (-30.f * ui_state->animation_dt));
  // F32 slug_rate = 1 - pow_f32(2, (-15.f * ui_state->animation_dt));
  // F32 slaf_rate = 1 - pow_f32(2, (-8.f * ui_state->animation_dt));
  // fish->vel.x += vast_rate * (fish->target_vel.x-fish->vel.x);
  // fish->vel.y += vast_rate * (fish->target_vel.y-fish->vel.y);

  fish->vel = mix_2f32(fish->target_vel, fish->vel, 0.7);
  // fish->vel = fish->target_vel;

  // fish->vel = fish->target_vel;
  // Vec2F32 vel_normalized = normalize_2f32(fish->vel);
  // F32 vel_mag = length_2f32(fish->vel);
  // printf("vel_mag: %f\n", vel_mag);
  // F32 speed = Clamp(0, vel_mag, 900);
  // fish->vel = scale_2f32(vel_normalized, speed);

  Vec2F32 d_position = scale_2f32(fish->vel, ctx->dt_sec);
  position = add_2f32(position, d_position);
  node->node2d->transform.position = position;

  F32 update_dt = os_now_microseconds()/1000000.0 - fish->last_motivation_update_time;
  if(update_dt > 6.0)
  {
    fish->motivation = 1 + (rand()/(F32)RAND_MAX) * 1;
  }
  return;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Scene Entry

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

  // root node
  RK_Node *root = rk_build_node3d_from_stringf(0,0, "root");
  rk_node_push_fn(root, str8_lit("s5_fn_scene_begin"));
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
    for(U64 i = 0; i < 230; i++)
    {
      F32 x = (rand()%spwan_dim_x) + spwan_range_x.min;
      F32 y = (rand()%spwan_dim_y) + spwan_range_y.min;
      RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D, 0, "fish_%I64u", i);
      node->node2d->transform.position = v2f32(x, y);
      node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
      node->sprite2d->size = v2f32(10,10);
      node->sprite2d->color = v4f32(0.9,0.0,0.1,1.);
      node->sprite2d->omit_texture = 1;
      node->custom_flags |= S5_EntityFlag_Boid;
      rk_node_push_fn(node, str8_lit("s5_fn_fish"));
      submarine_node = node;
      S5_Fish *fish = rk_node_push_custom_data(node, S5_Fish);
      fish->min_depth = y - 30;
      fish->max_depth = y + 30;
      fish->motivation = 1.0;
    }
  }

  ret->root = rk_handle_from_node(root);
  rk_pop_scene();
  rk_pop_node_bucket();
  rk_pop_res_bucket();
  rk_pop_handle_seed();
  return ret;
}
