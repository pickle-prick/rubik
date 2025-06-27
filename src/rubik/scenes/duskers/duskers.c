/////////////////////////////////////////////////////////////////////////////////////////
// Constants

#define WATER_DENSITY                    1025.0 // kg/m^3 for seawater
#define GRAVITY                          9.81
#define DRAG_COEFF                       0.9

#define BOID_MAX_VELOCITY                900

#define SUBMARINE_MASS                   1
// #define SUBMARINE_VOLUME                100
// #define SUBMARINE_MAX_DENSITY           100
// #define SUBMARINE_MIN_DENSITY           100
#define SUBMARINE_OXYGEN_CAP             300
#define SUBMARINE_HP_CAP                 100
#define SUBMARINE_BATTERY_CAP            600 // in joules
#define SUBMARINE_THRUST_FORCE_CAP       6
#define SUBMARINE_THRUST_FORCE_INC_STEP  2
#define SUBMARINE_PULSE_FORCE            100
#define SUBMARINE_PULSE_COOLDOWN         0.6  // in seconds
#define SUBMARINE_FORCE_TO_POWER_COEFF   0.1
// Warning/Critical level
#define SUBMARINE_OXYGEN_WARNING_LEVEL   SUBMARINE_OXYGEN_CAP*0.3
#define SUBMARINE_OXYGEN_CRITICAL_LEVEL  SUBMARINE_OXYGEN_CAP*0.1
#define SUBMARINE_BATTERY_WARNING_LEVEL  SUBMARINE_BATTERY_CAP*0.3
#define SUBMARINE_BATTERY_CRITICAL_LEVEL SUBMARINE_BATTERY_CAP*0.1
#define SUBMARINE_ANALYZE_SPEED          1.1

#define AssertVec2F32(v) Assert(!isnan(v.x) && !isnan(v.y))

/////////////////////////////////////////////////////////////////////////////////////////
// Basic Type/Enum

typedef U64 S5_EntityFlags;
#define S5_EntityFlag_Flock          (S5_EntityFlags)(1ull<<0)
#define S5_EntityFlag_Boid           (S5_EntityFlags)(1ull<<1)
#define S5_EntityFlag_Resource       (S5_EntityFlags)(1ull<<2)
#define S5_EntityFlag_Predator       (S5_EntityFlags)(1ull<<3)
#define S5_EntityFlag_Submarine      (S5_EntityFlags)(1ull<<4)
#define S5_EntityFlag_ExternalObject (S5_EntityFlags)(1ull<<5)
#define S5_EntityFlag_Detectable     (S5_EntityFlags)(1ull<<6)
#define S5_EntityFlag_GameCamera     (S5_EntityFlags)(1ull<<7)
#define S5_EntityFlag_DockingGate    (S5_EntityFlags)(1ull<<8)

typedef enum S5_ResourceKind
{
  S5_ResourceKind_AirBag,
  S5_ResourceKind_Battery,
  S5_ResourceKind_COUNT,
} S5_ResourceKind;

typedef enum S5_SoundKind
{
  S5_SoundKind_GainResource_AirBag,
  S5_SoundKind_GainResource_Battery,
  S5_SoundKind_Ambient,
  S5_SoundKind_COUNT,
} S5_SoundKind;

typedef enum S5_InstrumentKind
{
  S5_InstrumentKind_Beep,
  S5_InstrumentKind_Boop,
  S5_InstrumentKind_Gear,
  S5_InstrumentKind_Radiation,
  S5_InstrumentKind_AirPressor,
  S5_InstrumentKind_Ping,
  S5_InstrumentKind_Echo,
  S5_InstrumentKind_COUNT,
} S5_InstrumentKind;

typedef enum S5_SequencerKind
{
  S5_SequencerKind_SonarScan,
  S5_SequencerKind_Radiation,
  S5_SequencerKind_Info,
  S5_SequencerKind_Warning,
  S5_SequencerKind_Critical,
  S5_SequencerKind_COUNT,
} S5_SequencerKind;

typedef struct S5_Scene S5_Scene;
struct S5_Scene
{
  RK_Handle sea;
  RK_Handle submarine;

  // NOTE: not serializable (loaded from scene setup function)
  SY_Instrument *instruments[S5_InstrumentKind_COUNT];
  SY_Sequencer *sequencers[S5_SequencerKind_COUNT];
  // TODO: sounds should be loaded as resource  
  OS_Handle sounds[S5_SoundKind_COUNT];

  // per-frame build artifacts
  Vec2F32 world_mouse;
  QuadTree *root_quad;
  RK_Node **nodes;
  // TODO: to be implemented
  Rng2F32 world_bounds;

  F32 target_warning_t;
  F32 target_critical_t;
  F32 warning_t;
  F32 critical_t;
};

typedef struct S5_GameCamera S5_GameCamera;
struct S5_GameCamera
{
  Rng2F32 viewport_world;
  Rng2F32 viewport_world_target;
};

typedef struct S5_Resource S5_Resource;
struct S5_Resource
{
  S5_ResourceKind kind;
  union
  {
    struct
    {
      F32 oxygen;
    } airbag;

    struct
    {
      F32 joules;
    } battery;

    struct
    {
      U64 quantity;
    } part;

    F32 v[1];
  } value;
};

typedef struct S5_Submarine S5_Submarine;
struct S5_Submarine
{
  Vec2F32 v;
  F32 density;
  F32 viewport_radius;
  Vec2F32 thrust;

  // TODO: passive and active sonar
  // TODO: bearing graph
  // TODO: bearing graph analyzer (object detection, movement detection)

  // TODO: depth
  // TODO: pressure
  // TODO: tempature
  // TODO: salt content

  B32 is_scanning;
  F32 scan_radius;
  F32 scan_t;

  F32 magnetic_range;

  F32 pulse_cd_t;

  // resource
  F32 hp;
  F32 hp_mins;
  U64 part_count;
  U64 last_take_hit_us;
  F32 oxygen;
  F32 battery; // in joules
};

typedef struct S5_Flock S5_Flock;
struct S5_Flock
{
  F32 w_target;
  F32 w_separation;
  F32 w_alignment;
  F32 w_cohesion;
  Vec2F32 target_pos;
  Vec2F32 path_start;
  Vec2F32 path_end;
};

typedef struct S5_Boid S5_Boid;
struct S5_Boid
{
  // TODO: path plotting to be implemented
  F32 max_depth;
  F32 min_depth;

  Vec2F32 vel;
  Vec2F32 acc;
  Vec2F32 target_vel;

  F32 motivation;
  F32 last_motivation_update_time;
};

typedef struct S5_Predator S5_Predator;
struct S5_Predator
{
  F32 current_energy;
  // energy_cost = k_drag * speed²
  F32 max_energy;
  F32 energy_recovery_rate;
  F32 roaming_speed; // magnitude of velocity
  F32 burst_speed;
  F32 k_drag; // energy cost per (speed^2) per second
  B32 chasing;
};

typedef struct S5_Detectable S5_Detectable;
struct S5_Detectable
{
  U8 name[256];
  F32 analyze_t;
};

typedef struct S5_Entity S5_Entity;
struct S5_Entity
{
  S5_Submarine submarine;
  S5_Resource resource;
  S5_Flock flock;
  S5_Boid boid;
  S5_Predator predator;
  S5_Detectable detectable;
  S5_GameCamera game_camera;
  // TODO: we could have an editor camera
};

/////////////////////////////////////////////////////////////////////////////////////////
// Helper Macros

#define s5_submarine_from_node(n)   &(((S5_Entity*)n->custom_data)->submarine)
#define s5_resource_from_node(n)    &(((S5_Entity*)n->custom_data)->resource)
#define s5_flock_from_node(n)       &(((S5_Entity*)n->custom_data)->flock)
#define s5_boid_from_node(n)        &(((S5_Entity*)n->custom_data)->boid)
#define s5_predator_from_node(n)    &(((S5_Entity*)n->custom_data)->predator)
#define s5_detectable_from_node(n)  &(((S5_Entity*)n->custom_data)->detectable)
#define s5_game_camera_from_node(n) &(((S5_Entity*)n->custom_data)->game_camera)

/////////////////////////////////////////////////////////////////////////////////////////
// Helper Functions

internal Vec2F32
s5_world_position_from_mouse(Mat4x4F32 proj_view_inv_m)
{
  Vec2F32 mouse = rk_state->cursor;
  Vec2F32 resolution_dim = rk_state->window_dim;
  // mouse ndc pos
  F32 mox_ndc = (mouse.x / resolution_dim.x) * 2.f - 1.f;
  F32 moy_ndc = (mouse.y / resolution_dim.y) * 2.f - 1.f;
  Vec4F32 mouse_in_world_4 = transform_4x4f32(proj_view_inv_m, v4f32(mox_ndc, moy_ndc, 1., 1.));
  Vec2F32 ret = v2f32(mouse_in_world_4.x, mouse_in_world_4.y);
  return ret;
}
internal Vec2F32
s5_screen_pos_from_world(Vec2F32 world_pos, Mat4x4F32 proj_view_m)
{
  Vec4F32 src = v4f32(world_pos.x, world_pos.y, 0, 1.0);
  Vec4F32 ndc = transform_4x4f32(proj_view_m, src);
  F32 x = (ndc.x+1.0)/2.0;
  F32 y = (ndc.y+1.0)/2.0;
  x *= rk_state->window_dim.x;
  y *= rk_state->window_dim.y;
  Vec2F32 ret = {x, y};
  return ret;
}

// TODO(XXX): remove this function
internal Vec2F32
world_pos_from_mouse(Mat4x4F32 proj_view_inv_m)
{
  // mouse ndc pos
  F32 mox_ndc = (rk_state->cursor.x / rk_state->window_dim.x) * 2.f - 1.f;
  F32 moy_ndc = (rk_state->cursor.y / rk_state->window_dim.y) * 2.f - 1.f;
  Vec4F32 mouse_in_world_4 = transform_4x4f32(proj_view_inv_m, v4f32(mox_ndc, moy_ndc, 1., 1.));
  Vec2F32 ret = v2f32(mouse_in_world_4.x, mouse_in_world_4.y);
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Main Update

RK_SCENE_UPDATE(s5_update)
{
  S5_Scene *s = scene->custom_data;
  RK_NodeBucket *node_bucket = scene->node_bucket;
  s->world_mouse = s5_world_position_from_mouse(ctx->proj_view_inv_m);
  RK_Node *submarine_node = rk_node_from_handle(&s->submarine);
  S5_Submarine *submarine = s5_submarine_from_node(submarine_node);
  Vec2F32 submarine_center = rk_center_from_sprite2d(submarine_node->sprite2d, submarine_node->node2d->transform.position);

  s->target_warning_t = 0;
  s->target_critical_t = 0;

  ///////////////////////////////////////////////////////////////////////////////////////
  // collect world nodes to a flag array for easier looping

  RK_Node **nodes = 0;
  for(U64 slot_index = 0; slot_index < node_bucket->hash_table_size; slot_index++)
  {
    for(RK_Node *node = node_bucket->hash_table[slot_index].first;
        node != 0;
        node = node->hash_next)
    {
      darray_push(rk_frame_arena(), nodes, node);
    }
  }
  s->nodes = nodes;

  ///////////////////////////////////////////////////////////////////////////////////////
  // build quadtree for Collider2D

  // TODO(XXX): not ideal, how can find a resonal bounds for the world 
  // we may want to collect the 2d world bounds for sprite2d
  QuadTree *root_quad = quadtree_push(rk_frame_arena(), r2f32p(-400000,-400000,400000,400000));

  for(U64 i = 0; i < darray_size(nodes); i++)
  {
    RK_Node *n = nodes[i];
    // TODO(XXX): we should only add Collider2D instead of Sprite2D
    if(n->type_flags & RK_NodeTypeFlag_Collider2D)
    {
      Vec2F32 position = n->node2d->transform.position;
      Rng2F32 src_rect = rk_rect_from_sprite2d(n->sprite2d, position);
      quadtree_insert(rk_frame_arena(), root_quad, src_rect, n);
    }
  }
  s->root_quad = root_quad;

  ///////////////////////////////////////////////////////////////////////////////////////
  // draw a debug rect

  if(BUILD_DEBUG) RK_Parent_Scope(rk_node_from_handle(&scene->root))
  {
    Vec2F32 mouse_world = s5_world_position_from_mouse(ctx->proj_view_inv_m);
    RK_Node *n = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,
                                            RK_NodeFlag_Transient, "debug_rect");
    Vec2F32 position = mouse_world;
    n->sprite2d->anchor = RK_Sprite2DAnchorKind_Center;
    n->sprite2d->shape = RK_Sprite2DShapeKind_Rect;
    n->sprite2d->size.rect = v2f32(300,300);
    n->sprite2d->color = v4f32(1,0,0,0.1);
    n->sprite2d->omit_texture = 1;
    n->sprite2d->draw_edge = 1;
    n->node2d->transform.position = position;
    n->node2d->z_index = -1;

    Temp scratch = scratch_begin(0,0);
    {
      Rng2F32 debug_rect = rk_rect_from_sprite2d(n->sprite2d, position);
      RK_Node **nodes = 0;
      quadtree_query(scratch.arena, s->root_quad, debug_rect, (void***)&nodes);

      U64 count = 0;
      for(U64 i = 0; i < darray_size(nodes); i++)
      {
        if(nodes[i]->custom_flags & S5_EntityFlag_Boid)
        {
          count++;
        }
      }
      String8 string = push_str8f(rk_frame_arena(), "%I64u", count);
      rk_debug_gfx(30, add_2f32(rk_state->cursor, v2f32(30, 0)), v4f32(1,1,1,1),string);
    }
    scratch_end(scratch);
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // hide sprite2d if it's no within player's viewport

  ProfBegin("visiable check");
  for(U64 i = 0; i < darray_size(nodes); i++)
  {
    RK_Node *node = nodes[i];
    if(node->custom_flags&S5_EntityFlag_ExternalObject)
    {
      F32 dist_to_submarine = length_2f32(sub_2f32(submarine_center, node->node2d->transform.position));
      B32 is_visiable = dist_to_submarine < submarine->viewport_radius;
      if(!is_visiable)
      {
        node->sprite2d->color.w = 0;
        node->sprite2d->draw_edge = 0;
      }
      else
      {
        node->sprite2d->color.w = 1;
        node->sprite2d->draw_edge = 1;
      }
    }
  }
  ProfEnd();

  ///////////////////////////////////////////////////////////////////////////////////////
  // systems

  ProfBegin("system update");
  for(U64 i = 0; i < darray_size(nodes); i++)
  {
    RK_Node *node = nodes[i];
    if(node->custom_flags & (S5_EntityFlag_GameCamera))
    {
      s5_system_game_camera(node, scene, ctx);
    }
    if(node->custom_flags & (S5_EntityFlag_Submarine))
    {
      s5_system_submarine(node, scene, ctx);
    }
    if(node->custom_flags & (S5_EntityFlag_Flock))
    {
      s5_system_flock(node, scene, ctx);
    }
    if(node->custom_flags & (S5_EntityFlag_Boid))
    {
      s5_system_boid(node, scene, ctx);
    }
    if(node->custom_flags & (S5_EntityFlag_Predator))
    {
      s5_system_predator(node, scene, ctx);
    }
    if(node->custom_flags & (S5_EntityFlag_Detectable))
    {
      s5_system_detectable(node, scene, ctx);
    }
  }

  if(s->warning_t > 0 && s->critical_t == 0)
  {
    sy_sequencer_play(s->sequencers[S5_SequencerKind_Warning], 0);
  }
  else
  {
    sy_sequencer_pause(s->sequencers[S5_SequencerKind_Warning]);
  }

  if(s->critical_t > 0)
  {
    sy_sequencer_play(s->sequencers[S5_SequencerKind_Critical], 0);
  }
  else
  {
    sy_sequencer_pause(s->sequencers[S5_SequencerKind_Critical]);
  }
  ProfEnd();

  ///////////////////////////////////////////////////////////////////////////////////////
  // game ui

  D_BucketScope(rk_state->bucket_rect)
  {
    UI_Box *container = 0;
    UI_Rect(rk_state->window_rect)
    {
      container = ui_build_box_from_stringf(0, "###game_overlay");
    }

    UI_Parent(container)
      UI_FontSize(25.0)
    {
      UI_Column
        UI_Flags(UI_BoxFlag_DrawBorder)
        {
          ui_labelf("hp: %.2f", submarine->hp);
          ui_labelf("level: %.2f %.2f", submarine_node->node2d->transform.position.x, submarine_node->node2d->transform.position.y);
          ui_labelf("vel: %.2f %.2f", submarine->v.x, submarine->v.y);
          ui_labelf("density: %.2f", submarine->density);
          ui_labelf("scan_t: %.2f", submarine->scan_t);

          rk_capped_labelf(submarine->oxygen/SUBMARINE_OXYGEN_CAP, "oxygen: %.2f", submarine->oxygen);
          rk_capped_labelf(submarine->battery/SUBMARINE_BATTERY_CAP, "battery: %.2f", submarine->battery);
          ui_labelf("parts: %I64u", submarine->part_count);
          ui_labelf("thrust: %.2f %.2f",
                    submarine->thrust.x,
                    submarine->thrust.y);
          rk_capped_labelf(submarine->pulse_cd_t, "pulse_cd_t: %.2f", submarine->pulse_cd_t);
        }
    }

    // depth level
    UI_Parent(container)
    {
      UI_Box *body = 0;
      F32 width_px = 100.0;
      F32 height_px = rk_state->window_dim.y*0.5;
      UI_Flags(UI_BoxFlag_Floating|UI_BoxFlag_DrawBorder)
        UI_Rect(r2f32p(30, rk_state->window_dim.y/2.0 - height_px/2.0,
                       30+width_px, rk_state->window_dim.y/2.0 + height_px/2.0))
        UI_ChildLayoutAxis(Axis2_Y)
        {
          body = ui_build_box_from_stringf(0, "vertical_level");
        }

      UI_Parent(body)
        UI_PrefHeight(ui_pct(1.0, 0.0))
        UI_PrefWidth(ui_pct(1.0,0.0))
      {
        for(U64 i = 0; i < 31; i++)
        {
          if(i == 15)
          {
            ui_buttonf("%.2f##%I64u", submarine_node->node2d->transform.position.y, i);
          }
          else
          {
            ui_buttonf("##%I64u", i);
          }
        }
      }
    }

    // horizontal level

    if(s->warning_t > 0)
      rk_debug_gfx(90, v2f32(300,300), v4f32(1,1,0,s->warning_t), str8_lit("WARNING"));
    if(s->critical_t > 0)
      rk_debug_gfx(90, v2f32(300,400), v4f32(1,0,0,s->critical_t), str8_lit("CRITICAL"));
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // animating

  s->warning_t += rk_state->animation.slug_rate * (s->target_warning_t - s->warning_t);
  s->critical_t += rk_state->animation.slug_rate * (s->target_critical_t - s->critical_t);

  if(abs_f32(s->warning_t) < 0.01)
  {
    s->warning_t = 0.0;
  }
  if(abs_f32(s->critical_t) < 0.01)
  {
    s->critical_t = 0.0;
  }
}


/////////////////////////////////////////////////////////////////////////////////////////
// Systems

internal void
s5_system_game_camera(RK_Node *node, RK_Scene *scene, RK_FrameContext *ctx)
{
  S5_Scene *s = scene->custom_data;
  RK_Node *submarine_node = rk_node_from_handle(&s->submarine);
  S5_GameCamera *camera = s5_game_camera_from_node(node);

  // center the view of submarine
  node->node3d->transform.position.x = submarine_node->node2d->transform.position.x;
  node->node3d->transform.position.y = submarine_node->node2d->transform.position.y;
  Vec2F32 dim = dim_2f32(camera->viewport_world_target);
  node->node3d->transform.position.x -= dim.x/2.0;
  node->node3d->transform.position.y -= dim.y/2.0;

  if(rk_state->sig.f & UI_SignalFlag_MiddleDragging)
  {
    Vec2F32 mouse_delta = rk_state->cursor_delta;
    Vec2F32 v = scale_2f32(mouse_delta, 1);
    camera->viewport_world_target.p0 = sub_2f32(camera->viewport_world.p0, v);
    camera->viewport_world_target.p1 = sub_2f32(camera->viewport_world.p1, v);
  }

  // zoom
  if(rk_state->sig.scroll.x != 0 || rk_state->sig.scroll.y != 0)
  {
    F32 d = 1 + 0.03*rk_state->sig.scroll.y;
    Vec2F32 dim = dim_2f32(camera->viewport_world_target);
    dim.x *= d;
    dim.y *= d;
    camera->viewport_world_target.x1 = camera->viewport_world_target.x0 + dim.x;
    camera->viewport_world_target.y1 = camera->viewport_world_target.y0 + dim.y;

    // some translation to make the cursor consistent in world
    Vec2F32 cursor_world_old = world_pos_from_mouse(ctx->proj_view_inv_m);
    Vec2F32 cursor_world_new = {0};
    {
      // mouse ndc pos
      F32 mox_normalized = rk_state->cursor.x / rk_state->window_dim.x;
      F32 moy_normalized = rk_state->cursor.y / rk_state->window_dim.y;
      F32 mox_world = mox_normalized * dim.x;
      F32 moy_world = moy_normalized * dim.y;
      // cursor_world_new.x = camera->viewport_world_target.x0 + mox_normalized*dim.x;
      // cursor_world_new.y = camera->viewport_world_target.y0 + mox_normalized*dim.y;
      Vec2F32 camera_pos = v2f32(node->node3d->transform.position.x, node->node3d->transform.position.y);
      cursor_world_new = add_2f32(camera_pos, v2f32(mox_world, moy_world));
    }
    Vec2F32 pos_delta = sub_2f32(cursor_world_new, cursor_world_old);
    // node->node3d->transform.position.x -= pos_delta.x;
    // node->node3d->transform.position.y -= pos_delta.y;
  }

  // TODO(XXX): keep the viewport w/h ratio matchs the current window
  if(rk_state->window_res_changed)
  {
    F32 ratio = rk_state->last_window_dim.y / rk_state->last_window_dim.x;
    Vec2F32 viewport_dim = dim_2f32(camera->viewport_world);
    F32 new_height = viewport_dim.x * ratio;

    camera->viewport_world_target.y1 = camera->viewport_world.y0 + new_height;
  }

  // animating
  camera->viewport_world.x0 += rk_state->animation.fast_rate * (camera->viewport_world_target.x0 - camera->viewport_world.x0);
  camera->viewport_world.x1 += rk_state->animation.fast_rate * (camera->viewport_world_target.x1 - camera->viewport_world.x1);
  camera->viewport_world.y0 += rk_state->animation.fast_rate * (camera->viewport_world_target.y0 - camera->viewport_world.y0);
  camera->viewport_world.y1 += rk_state->animation.fast_rate * (camera->viewport_world_target.y1 - camera->viewport_world.y1);

  // update
  node->camera3d->orthographic.top    = camera->viewport_world.y0;
  node->camera3d->orthographic.bottom = camera->viewport_world.y1;
  node->camera3d->orthographic.left   = camera->viewport_world.x0;
  node->camera3d->orthographic.right  = camera->viewport_world.x1;
}

internal void s5_system_submarine(RK_Node *node, RK_Scene *scene, RK_FrameContext *ctx)
{
  Temp scratch = scratch_begin(0,0);
  S5_Scene *s = scene->custom_data;
  S5_Submarine *submarine = s5_submarine_from_node(node);
  RK_Transform2D *transform = &node->node2d->transform;
  RK_Node *sea_node = rk_node_from_handle(&s->sea);
  RK_Sprite2D *sprite2d = node->sprite2d;
  Vec2F32 *position = &node->node2d->transform.position;
  Vec2F32 center = rk_center_from_sprite2d(sprite2d, *position);

  // F32 y_in = transform->position.y+sprite2d->size.rect.y-sea_node->node2d->transform.position.y;
  // y_in = Clamp(0, y_in, sprite2d->size.rect.y);
  // F32 volume = (sprite2d->size.rect.x*S5_SCALE_WORLD_TO_METER) * (sprite2d->size.rect.y*S5_SCALE_WORLD_TO_METER);
  // F32 volume_displaced = sprite2d->size.rect.x*y_in*S5_SCALE_WORLD_TO_METER*S5_SCALE_WORLD_TO_METER;

  // F32 g = 9.8*1000.0; /* gravitational acceleration */
  // F32 mass = submarine->density * volume;
  // F32 mass_displaced = S5_SEA_FLUID_DENSITY * volume_displaced;

  ///////////////////////////////////////////////////////////////////////////////////////
  // Control

  ////////////////////////////////
  // unpack accmulator

  F32 oxygen = submarine->oxygen;
  F32 battery = submarine->battery;
  U64 part_count = submarine->part_count;
  F32 hp = submarine->hp;
  F32 watts = 0;
  F32 warning_t = 0;
  F32 critical_t = 0;

  if(oxygen == 0)  exit(0);
  if(battery == 0) exit(0);
  if(hp == 0)      exit(0);

  ////////////////////////////////
  // thrust

  Vec2F32 thrust = {0};
  thrust = submarine->thrust;

  if(rk_key_press(0, OS_Key_Up))
  {
    sy_instrument_play(s->instruments[S5_InstrumentKind_Gear], 0, 1.5, 0, 1.0);
    thrust.y -= SUBMARINE_THRUST_FORCE_INC_STEP;
  }
  if(rk_key_press(0, OS_Key_Down))
  {
    sy_instrument_play(s->instruments[S5_InstrumentKind_Gear], 0, 1.5, 0, 1.0);
    thrust.y += SUBMARINE_THRUST_FORCE_INC_STEP;
  }
  if(rk_key_press(0, OS_Key_Left))
  {
    sy_instrument_play(s->instruments[S5_InstrumentKind_Gear], 0, 1.5, 0, 1.0);
    thrust.x -= SUBMARINE_THRUST_FORCE_INC_STEP;
  }
  if(rk_key_press(0, OS_Key_Right))
  {
    sy_instrument_play(s->instruments[S5_InstrumentKind_Gear], 0, 1.5, 0, 1.0);
    thrust.x += SUBMARINE_THRUST_FORCE_INC_STEP;
  }

  for(U64 i = 0; i < 2; i++)
  {
    F32 force = thrust.v[i];
    force = Clamp(-SUBMARINE_THRUST_FORCE_CAP, force, SUBMARINE_THRUST_FORCE_CAP);
    thrust.v[i] = force;
    watts += SUBMARINE_FORCE_TO_POWER_COEFF * force * force;
  }
  MemoryCopy(&submarine->thrust, &thrust, sizeof(thrust));

  ////////////////////////////////
  // pulse (for some percesion control)

  if(submarine->pulse_cd_t > 0)
  {
    submarine->pulse_cd_t = ClampBot(0.0, submarine->pulse_cd_t-rk_state->frame_dt);
  }

  Vec2F32 pulse = {0};
  if(submarine->pulse_cd_t == 0.0)
  {
    B32 pulse_applied = 0;
    if(rk_key_press(0, OS_Key_A))
    {
      pulse.x -= SUBMARINE_PULSE_FORCE;
      watts += SUBMARINE_FORCE_TO_POWER_COEFF * SUBMARINE_PULSE_FORCE * SUBMARINE_PULSE_FORCE;
      pulse_applied = 1;
    }
    if(rk_key_press(0, OS_Key_D))
    {
      pulse.x += SUBMARINE_PULSE_FORCE;
      watts += SUBMARINE_FORCE_TO_POWER_COEFF * SUBMARINE_PULSE_FORCE * SUBMARINE_PULSE_FORCE;
      pulse_applied = 1;
    }
    if(rk_key_press(0, OS_Key_W))
    {
      pulse.y -= SUBMARINE_PULSE_FORCE;
      watts += SUBMARINE_FORCE_TO_POWER_COEFF * SUBMARINE_PULSE_FORCE * SUBMARINE_PULSE_FORCE;
      pulse_applied = 1;
    }
    if(rk_key_press(0, OS_Key_S))
    {
      pulse.y += SUBMARINE_PULSE_FORCE;
      watts += SUBMARINE_FORCE_TO_POWER_COEFF * SUBMARINE_PULSE_FORCE * SUBMARINE_PULSE_FORCE;
      pulse_applied = 1;
    }

    if(pulse_applied)
    {
      submarine->pulse_cd_t = SUBMARINE_PULSE_COOLDOWN;
      sy_instrument_play(s->instruments[S5_InstrumentKind_AirPressor], 0, 1.5, 0, 1.0);
    }
  }

  ////////////////////////////////
  // bouyancy force (based on Archimedes' principle)

  // F32 drag = 0.1;
  // Vec2F32 v = submarine->v;
  // // F.y += (mass-mass_displaced)*g;
  // // F.x += -drag*v.x;
  // // F.y += -drag*v.y;
  // Vec2F32 acc = {F.x/mass, F.y/mass};
  // v.x += acc.x * rk_state->frame_dt;
  // v.y += acc.y * rk_state->frame_dt;
  // Vec2F32 pos = transform->position;
  // pos.x += v.x * rk_state->frame_dt;
  // pos.y += v.y * rk_state->frame_dt;

  // submarine->v = v;
  // transform->position = pos;
  // submarine->density = density;

  ////////////////////////////////
  // compute velocity

  Vec2F32 net_force = {0};
  net_force.x += thrust.x;
  net_force.y += thrust.y;
  net_force.x += pulse.x;
  net_force.y += pulse.y;

  Vec2F32 v = submarine->v;
  Vec2F32 net_v = {0};
  net_v.x = net_force.x / SUBMARINE_MASS;
  net_v.y = net_force.y / SUBMARINE_MASS;
  v = add_2f32(v, net_v);

  // apply drag
  v.x *= (1.0f - DRAG_COEFF * rk_state->frame_dt);
  v.y *= (1.0f - DRAG_COEFF * rk_state->frame_dt);

  // update velocity
  submarine->v = v;

  ////////////////////////////////
  // update position

  *position = add_2f32(*position, scale_2f32(v, rk_state->frame_dt));

  ///////////////////////////////////////////////////////////////////////////////////////
  // visiual stuffs

  if(ui_key_press(0, OS_Key_Space))
  {
    sy_instrument_play(s->instruments[S5_InstrumentKind_Ping], 0, 3, 0, 1.0);
    submarine->is_scanning = 1;
  }

  if(submarine->is_scanning)
  {
    watts += 30;
    submarine->scan_t += 1 * rk_state->frame_dt;
    if(submarine->scan_t > 1.0)
    {
      submarine->scan_t = 0.0;
      submarine->is_scanning = 0;
    }

    submarine->scan_t = Clamp(0, submarine->scan_t, 1);
  }

  ////////////////////////////////
  // draw grid line distance viewer

  // RK_Parent_Scope(node)
  // {
  //   Vec2F32 grid_dim = v2f32(700,700);
  //   Vec2U32 grid_size = {30,30};
  //   F32 x = -grid_dim.x/2.0;
  //   F32 y = -grid_dim.y/2.0;
  //   F32 w = grid_dim.x/grid_size.x;
  //   F32 h = grid_dim.y/grid_size.y;
  //   for(U64 j = 0; j < grid_size.y; j++)
  //   {
  //     for(U64 i = 0; i < grid_size.x; i++)
  //     {
  //       B32 odd = (j%2==0 && i%2!=0) || (j%2!=0 && i%2==0);
  //       RK_Node *n = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,
  //                                               RK_NodeFlag_Transient, "cube_%I64u-I64u", i, j);
  //       F32 radius = submarine->viewport_radius;

  //       n->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
  //       n->sprite2d->shape = RK_Sprite2DShapeKind_Rect;
  //       n->sprite2d->size.rect.x = w;
  //       n->sprite2d->size.rect.y = h;
  //       n->sprite2d->color = v4f32(0.3*odd,0.3,0.1*odd,0.1);
  //       n->sprite2d->omit_texture = 1;
  //       n->sprite2d->draw_edge = 1;
  //       n->node2d->transform.position.x = x + i*w;
  //       n->node2d->transform.position.y = y + j*h;
  //       n->node2d->z_index = 1;
  //     }
  //   }
  // }

  ////////////////////////////////
  // draw viewport

  RK_Parent_Scope(node)
  {
    RK_Node *n = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,
                                            RK_NodeFlag_Transient, "viewport");
    F32 radius = submarine->viewport_radius;

    n->sprite2d->anchor = RK_Sprite2DAnchorKind_Center;
    n->sprite2d->shape = RK_Sprite2DShapeKind_Circle;
    n->sprite2d->size.circle.radius = radius;
    n->sprite2d->color = v4f32(0.01,0.01,0.1,0.5);
    n->sprite2d->omit_texture = 1;
    n->sprite2d->draw_edge = 1;
    n->node2d->z_index = 1;
  }

  ////////////////////////////////
  // draw a ball along the viewport circle to indicate submarine direction

  RK_Parent_Scope(node)
  {
    Vec2F32 submarine_direction = {0,-1};
    if(submarine->v.x != 0 || submarine->v.y != 0)
    {
      submarine_direction = normalize_2f32(submarine->v);
    }

    RK_Node *n = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,
                                            RK_NodeFlag_Transient, "direction_indicator");
    Vec2F32 pos = scale_2f32(submarine_direction, submarine->viewport_radius);
    n->sprite2d->anchor = RK_Sprite2DAnchorKind_Center;
    n->sprite2d->shape = RK_Sprite2DShapeKind_Circle;
    n->sprite2d->size.circle.radius = 20.0;
    n->sprite2d->color = v4f32(0.2,0.2,0.1,1.0);
    n->sprite2d->omit_texture = 1;
    n->sprite2d->draw_edge = 1;
    n->node2d->transform.position = pos;
    n->node2d->z_index = 1-0.1;
  }

  ////////////////////////////////
  // draw scan circle

  if(submarine->is_scanning) RK_Parent_Scope(node)
  {
    RK_Node *n = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,
                                            RK_NodeFlag_Transient, "scanner");
    Vec2F32 position = scale_2f32(node->sprite2d->size.rect, 0.5);
    F32 radius = 0;
    F32 target_radius = submarine->scan_radius;
    radius = mix_1f32(radius, target_radius, submarine->scan_t);

    n->sprite2d->anchor = RK_Sprite2DAnchorKind_Center;
    n->sprite2d->shape = RK_Sprite2DShapeKind_Circle;
    n->sprite2d->size.circle.radius = radius;
    n->sprite2d->color = v4f32(0.1,0.1,0,0.1);
    n->sprite2d->omit_texture = 1;
    n->node2d->transform.position = position;
    n->node2d->z_index = 1;
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // detect docking gate

  {
    Vec2F32 position = node->node2d->transform.position;
    Rng2F32 submarine_rect = rk_rect_from_sprite2d(node->sprite2d, position);
    Rng2F32 src_rect = pad_2f32(submarine_rect, submarine->magnetic_range);

    RK_Node **nodes_in_range = 0;
    quadtree_query(scratch.arena, s->root_quad, src_rect, (void***)&nodes_in_range);
    F32 strength = 0;
    for(U64 i = 0; i < darray_size(nodes_in_range); i++)
    {
      RK_Node *n = nodes_in_range[i];
      if(n->custom_flags & S5_EntityFlag_DockingGate)
      {
        Vec2F32 gate_position = n->node2d->transform.position;
        Rng2F32 gate_rect = rk_rect_from_sprite2d(n->sprite2d, gate_position);
        F32 dist = length_2f32(sub_2f32(position, gate_position));

        // TODO: game goal
        if(overlaps_2f32(gate_rect, submarine_rect))
        {
          exit(0);
        }

        if(dist <= submarine->magnetic_range)
        {
          F32 s = 1.0 - dist/submarine->magnetic_range;
          strength += s;
        }
      }
    }

    // F32 delay = 0;
    // for(U64 i = 0; i < 30; i++)
    // {
    //   F32 volumes[] = {3, 2, 2.5, 3.5};
    //   F32 volume = volumes[rand()%ArrayCount(volumes)];
    //   sy_instrument_play(s->instruments[S5_InstrumentKind_Radiation], delay, 0.1, 0, volume);
    //   F32 t[] = {0.05, 0.03, 0.02, 0.01, 0.04};
    //   F32 tt = t[rand() % ArrayCount(t)];
    //   printf("tt: %f\n", tt);
    //   delay += tt;
    // }
    // sy_sequencer_play(s->sequencers[S5_SequencerKind_SonarScan], 1);

    // NOTE: use raidation sound to navigate gate
    strength = Clamp(0.0, strength, 1.0);
    F32 volume = mix_1f32(-1.0, 3.0, strength);
    F32 dice = mix_1f32(-1.0, 1.0, strength);
    sy_sequencer_set_volume(s->sequencers[S5_SequencerKind_Radiation], volume);
    sy_sequencer_set_dice(s->sequencers[S5_SequencerKind_Radiation], dice);
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // collect resources

  {
    // TODO: add some helper function to make this easier
    Vec2F32 position = node->node2d->transform.position;
    Rng2F32 src_rect = rk_rect_from_sprite2d(node->sprite2d, position);

    RK_Node **nodes_in_range = 0;
    quadtree_query(scratch.arena, s->root_quad, src_rect, (void***)&nodes_in_range);
    RK_Node **resource_nodes_in_range = 0;
    for(U64 i = 0; i < darray_size(nodes_in_range); i++)
    {
      RK_Node *n = nodes_in_range[i];
      if(n->custom_flags & S5_EntityFlag_Resource)
      {
        darray_push(scratch.arena, resource_nodes_in_range, n);
      }
    }

    for(U64 i = 0; i < darray_size(resource_nodes_in_range); i++)
    {
      RK_Node *resource_node = resource_nodes_in_range[i];
      S5_Resource *resource = s5_resource_from_node(resource_node);
      // sy_sequencer_play(s->sequencers[S5_SequencerKind_Info], 1);
      switch(resource->kind)
      {
        case S5_ResourceKind_AirBag:
        {
          oxygen += resource->value.airbag.oxygen;
          os_sound_play(s->sounds[S5_SoundKind_GainResource_AirBag]);
        }break;
        case S5_ResourceKind_Battery:
        {
          battery += resource->value.battery.joules;
          os_sound_play(s->sounds[S5_SoundKind_GainResource_Battery]);
        }break;
        default:{InvalidPath;}break;
      }

      // NOTE: we can't just release node right now, free it at the beginning of next frame 
      RK_NodeBucket *node_bucket = resource_node->owner_bucket;
      SLLStackPush_N(node_bucket->first_to_free_node, resource_node, free_next);
    }
  }

  // clamp
  oxygen = ClampTop(oxygen, SUBMARINE_OXYGEN_CAP);
  battery = ClampTop(battery, SUBMARINE_BATTERY_CAP);

  // oxygen consuming & update
  oxygen = ClampBot(0.0, oxygen - 3.0*rk_state->frame_dt);
  battery = ClampBot(0.0, battery - watts*rk_state->frame_dt);
  submarine->oxygen = oxygen;
  submarine->battery = battery;
  submarine->part_count = part_count;

  ///////////////////////////////////////////////////////////////////////////////////////
  // take hit

  if(submarine->hp_mins != 0.0)
  {
    submarine->hp -= submarine->hp_mins;
    submarine->hp_mins = 0.0;
    submarine->last_take_hit_us = os_now_microseconds();
    submarine->hp = Clamp(0.0, submarine->hp, SUBMARINE_HP_CAP);
  }

  if((os_now_microseconds()-submarine->last_take_hit_us)/1000000.0 < 1.0)
  {
    critical_t += 1;
  }
  else if((os_now_microseconds()-submarine->last_take_hit_us)/1000000.0 < 3.0)
  {
    warning_t += 1;
  }

  // TODO: collision detect

  ///////////////////////////////////////////////////////////////////////////////////////
  // warning if no power or oxygen

  if(submarine->oxygen < SUBMARINE_OXYGEN_WARNING_LEVEL)
  {
    warning_t += 1;
  }
  if(submarine->battery < SUBMARINE_BATTERY_WARNING_LEVEL)
  {
    warning_t += 1;
  }

  if(submarine->oxygen < SUBMARINE_OXYGEN_CRITICAL_LEVEL)
  {
    critical_t += 1;
  }
  if(submarine->battery < SUBMARINE_BATTERY_CRITICAL_LEVEL)
  {
    critical_t += 1;
  }

  s->target_warning_t += warning_t;
  s->target_critical_t += critical_t;
  scratch_end(scratch);
}

internal void s5_system_flock(RK_Node *node, RK_Scene *scene, RK_FrameContext *ctx)
{
  Temp scratch = scratch_begin(0,0);
  S5_Scene *s = scene->custom_data;
  S5_Flock *flock = s5_flock_from_node(node);

  RK_Node **boids = 0;
  for(RK_Node *child = node->first; child != 0; child = child->next)
  {
    darray_push(scratch.arena, boids, child);
  }

  // TODO: we should use some cone viewport here, boid can't see behind
  RK_Node *submarine_node = rk_node_from_handle(&s->submarine);
  S5_Submarine *submarine = s5_submarine_from_node(submarine_node);
  Vec2F32 submarine_center = rk_center_from_sprite2d(submarine_node->sprite2d, submarine_node->node2d->transform.position);

  // compute boids average position
  Vec2F32 flock_position = {0};

  Vec2F32 flock_target_position = flock->target_pos;
  for(U64 j = 0; j < darray_size(boids); j++)
  {
    RK_Node *boid_node = boids[j];
    S5_Boid *boid = s5_boid_from_node(boid_node);
    RK_Transform2D *transform = &boid_node->node2d->transform;
    Vec2F32 size = boid_node->sprite2d->size.rect;
    Vec2F32 position = boid_node->node2d->transform.position;

    flock_position = add_2f32(flock_position, position);

    Vec2F32 acc = {0};

    /////////////////////////////////////////////////////////////////////////////////////
    // collect neighbors

    // TODO(XXX): it's still too expensive to run for every boid

    U64 max_neighbor_radius = size.x*2;
    RK_Node **neighbors_src = 0;
    Rng2F32 src_rect = rk_rect_from_sprite2d(boid_node->sprite2d, position);
    // TODO(XXX): consider the rect width and height seprately
    src_rect = pad_2f32(src_rect, max_neighbor_radius);
    quadtree_query(scratch.arena, s->root_quad, src_rect, (void***)&neighbors_src);
    RK_Node **neighbors = 0;
    for(U64 i = 0; i < darray_size(neighbors_src); i++)
    {
      if(neighbors_src[i]->custom_flags & S5_EntityFlag_Boid)
      {
        darray_push(scratch.arena, neighbors, neighbors_src[i]);
      }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // move to target

    {
      // TODO(XXX): we should make the direction turning slower

      F32 dist_self_to_target = length_2f32(sub_2f32(position, flock_target_position));
      Vec2F32 dir_self_to_target = normalize_2f32(sub_2f32(flock_target_position, position));
      // F32 dist_self_to_target = length_2f32(sub_2f32(position, submarine_center));
      // Vec2F32 dir_self_to_target = normalize_2f32(sub_2f32(submarine_center, position));
      // Vec2F32 dir_self_to_target = normalize_2f32(sub_2f32(s->world_mouse, position));
      // F32 dist_self_to_target = length_2f32(sub_2f32(s->world_mouse, position)); /* to mouse */

      F32 angle_rad = ((rand_f32()) * (1.0/2) - (1.0/4));
      F32 cos_a = cos_f32(angle_rad);
      F32 sin_a = cos_f32(angle_rad);

      Vec2F32 dir = {
        dir_self_to_target.x*cos_a - dir_self_to_target.y*sin_a,
        dir_self_to_target.x*sin_a + dir_self_to_target.y*cos_a,
      };

      // too close to the target
      if(dist_self_to_target < size.x*6.3)
      {
        F32 w = flock->w_target;
        Vec2F32 uf = scale_2f32(dir_self_to_target, -w);
        acc = add_2f32(acc, uf);
        AssertVec2F32(acc);
      }
      else
      {
        F32 w = flock->w_target;
        Vec2F32 uf = scale_2f32(dir, w);
        acc = add_2f32(acc, uf);
        AssertVec2F32(acc);
      }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // separation

    {
      for(U64 i = 0; i < darray_size(neighbors); i++)
      {
        RK_Node *rhs_node = neighbors[i];
        if(rhs_node != boid_node)
        {
          S5_Boid *rhs_boid = s5_boid_from_node(rhs_node);
          Vec2F32 rhs_position = rhs_node->node2d->transform.position;
          Vec2F32 rhs_to_self = sub_2f32(position, rhs_position);
          F32 dist = length_2f32(rhs_to_self);

          F32 min_dist = size.x*0.6;
          if(dist < min_dist && dist != 0)
          {
            F32 w = flock->w_separation * (dist/min_dist);
            // unit force (assuming mass is 1, so it's acceleration)
            Vec2F32 uf = scale_2f32(normalize_2f32(rhs_to_self), w);
            acc = add_2f32(acc, uf);
            AssertVec2F32(acc);
          }
        }
      }
    }

    // alignment
    {
      Vec2F32 alignment_direction = boid->vel;
      for(U64 i = 0; i < darray_size(neighbors); i++)
      {
        RK_Node *rhs_node = neighbors[i];

        if(rhs_node != boid_node)
        {
          S5_Boid *rhs_boid = s5_boid_from_node(rhs_node);
          Vec2F32 rhs_position = rhs_node->node2d->transform.position;

          Vec2F32 rhs_to_self = sub_2f32(position, rhs_position);
          F32 dist = length_2f32(rhs_to_self);

          alignment_direction.x += rhs_boid->vel.x;
          alignment_direction.y += rhs_boid->vel.y;
        }
      }
      U64 neighbor_count = darray_size(neighbors);
      if(neighbor_count)
      {
        alignment_direction.x /= (neighbor_count+1);
        alignment_direction.y /= (neighbor_count+1);

        if(alignment_direction.x != 0 || alignment_direction.y != 0)
        {
          Vec2F32 steer_alignment = normalize_2f32(sub_2f32(alignment_direction, boid->vel));
          F32 w = flock->w_alignment;
          Vec2F32 uf = scale_2f32(steer_alignment, w);
          acc = add_2f32(acc, uf);
          AssertVec2F32(acc);
        }
      }
    }

    // cohesion
    {
      Vec2F32 center_of_mass = position;
      U64 neighbor_count = darray_size(neighbors);
      for(U64 i = 0; i < neighbor_count; i++)
      {
        RK_Node *rhs_node = neighbors[i];

        if(rhs_node != boid_node)
        {
          S5_Boid *rhs_boid = s5_boid_from_node(rhs_node);
          Vec2F32 rhs_position = rhs_node->node2d->transform.position;

          Vec2F32 rhs_to_self = sub_2f32(position, rhs_position);
          F32 dist = length_2f32(rhs_to_self);

          center_of_mass.x += rhs_position.x;
          center_of_mass.y += rhs_position.y;
        }
      }
      if(neighbor_count > 0)
      {
        center_of_mass.x /= (neighbor_count+1);
        center_of_mass.y /= (neighbor_count+1);

        Vec2F32 steer_cohesion = normalize_2f32(sub_2f32(center_of_mass, position));
        F32 w = flock->w_cohesion;
        Vec2F32 uf = scale_2f32(steer_cohesion, w);
        acc = add_2f32(acc, uf);
        AssertVec2F32(acc);
      }
    }

    // F32 scale = 1.0 * (rand()/(F32)RAND_MAX) * (1);
    Vec2F32 vel = scale_2f32(acc, rk_state->frame_dt*boid->motivation);
    // clamp
    {
      F32 vel_len = length_2f32(vel);
      if(vel_len > BOID_MAX_VELOCITY)
      {
        vel = scale_2f32(vel, BOID_MAX_VELOCITY/vel_len);
      }
    }
    // limit vel
    boid->acc = acc;
    boid->target_vel = vel;
  }

  // pick new target position if we hit the path end
  flock_position = scale_2f32(flock_position, 1.0/darray_size(boids));
  F32 target_diff = length_2f32(sub_2f32(flock_position, flock->path_end));
  if(target_diff < 60.0)
  {
    Swap(Vec2F32, flock->path_end, flock->path_start);
  }
  // TODO: interpolation between current target_pos and next target pos
  flock->target_pos = flock->path_end;
  scratch_end(scratch);
}

internal void s5_system_boid(RK_Node *node, RK_Scene *scene, RK_FrameContext *ctx)
{
  S5_Scene *s = scene->custom_data;
  S5_Boid *boid = s5_boid_from_node(node);
  RK_Sprite2D *sprite2d = node->sprite2d;
  RK_Transform2D *transform = &node->node2d->transform;
  Vec2F32 size = node->sprite2d->size.rect;
  Vec2F32 position = node->node2d->transform.position;

  RK_Node *submarine_node = rk_node_from_handle(&s->submarine);
  S5_Submarine *submarine = s5_submarine_from_node(submarine_node);
  Vec2F32 submarine_center = rk_center_from_sprite2d(submarine_node->sprite2d, submarine_node->node2d->transform.position);
  F32 dist_to_submarine = length_2f32(sub_2f32(submarine_center, node->node2d->transform.position));
  B32 is_visiable = dist_to_submarine < submarine->viewport_radius;

  boid->vel = mix_2f32(boid->target_vel, boid->vel, 0.3);
  // fish->vel = fish->target_vel;

  // fish->vel = fish->target_vel;
  // Vec2F32 vel_normalized = normalize_2f32(fish->vel);
  // F32 vel_mag = length_2f32(fish->vel);
  // printf("vel_mag: %f\n", vel_mag);
  // F32 speed = Clamp(0, vel_mag, 900);
  // fish->vel = scale_2f32(vel_normalized, speed);

  Vec2F32 d_position = scale_2f32(boid->vel, rk_state->frame_dt);
  position = add_2f32(position, d_position);
  node->node2d->transform.position = position;

  F32 update_dt = os_now_microseconds()/1000000.0 - boid->last_motivation_update_time;
  if(update_dt > 6.0)
  {
    boid->motivation = 1 + (rand_f32()) * 1;
  }
}

// RK_NODE_CUSTOM_UPDATE(s5_fn_resource)
// {
//   S5_Scene *s = scene->custom_data;
//   S5_Resource *resource = node->custom_data;
//   RK_Sprite2D *sprite2d = node->sprite2d;
//   RK_Transform2D *transform = &node->node2d->transform;
//   Vec2F32 size = node->sprite2d->size.rect;
//   Vec2F32 position = node->node2d->transform.position;
//   Vec2F32 screen_pos = s5_screen_pos_from_world(position, ctx->proj_view_m);
// 
// }

internal void s5_system_predator(RK_Node *node, RK_Scene *scene, RK_FrameContext *ctx)
{
  S5_Scene *s = scene->custom_data;
  RK_Sprite2D *sprite2d = node->sprite2d;
  RK_Transform2D *transform = &node->node2d->transform;
  S5_Predator *predator = s5_predator_from_node(node);

  Vec2F32 *position = &node->node2d->transform.position;
  Vec2F32 screen_pos = s5_screen_pos_from_world(*position, ctx->proj_view_m);

  RK_Node *submarine_node = rk_node_from_handle(&s->submarine);
  S5_Submarine *submarine = s5_submarine_from_node(submarine_node);
  Vec2F32 submarine_center = rk_center_from_sprite2d(submarine_node->sprite2d, submarine_node->node2d->transform.position);
  F32 dist_to_submarine = length_2f32(sub_2f32(submarine_center, *position));
  Vec2F32 dir_to_submarine = normalize_2f32(sub_2f32(submarine_center, node->node2d->transform.position));

  B32 chasing = (predator->chasing && predator->current_energy >= predator->max_energy*0.2 && dist_to_submarine < 500.0) ||
                (!predator->chasing && dist_to_submarine < 300.0 && predator->current_energy >= predator->max_energy*0.9);
  F32 speed = 0;

  if(chasing)
  {
    speed = mix_1f32(0.0, predator->burst_speed, predator->current_energy/predator->max_energy);

    if(dist_to_submarine < 60.0)
    {
      submarine->hp_mins += 10.0 * rk_state->frame_dt;
    }
  }
  else
  {
    F32 lost_amount = predator->max_energy-predator->current_energy;
    predator->current_energy += predator->energy_recovery_rate * lost_amount * rk_state->frame_dt;
  }

  predator->current_energy -= predator->k_drag * speed * speed * rk_state->frame_dt;
  predator->current_energy = Clamp(0.0, predator->current_energy, predator->max_energy);

  Vec2F32 vel = scale_2f32(dir_to_submarine, speed);
  predator->chasing = chasing;
  *position = add_2f32(*position, scale_2f32(vel, rk_state->frame_dt));
}

internal void
s5_system_detectable(RK_Node *node, RK_Scene *scene, RK_FrameContext *ctx)
{
  S5_Scene *s = scene->custom_data;
  S5_Detectable *detectable = s5_detectable_from_node(node);
  Vec2F32 position = node->node2d->transform.position;
  Vec2F32 screen_pos = s5_screen_pos_from_world(position, ctx->proj_view_m);

  RK_Node *submarine_node = rk_node_from_handle(&s->submarine);
  S5_Submarine *submarine = s5_submarine_from_node(submarine_node);
  Vec2F32 submarine_center = rk_center_from_sprite2d(submarine_node->sprite2d, submarine_node->node2d->transform.position);
  F32 dist_to_submarine = length_2f32(sub_2f32(submarine_center, node->node2d->transform.position));
  F32 scan_range = mix_1f32(0.0, submarine->scan_radius, submarine->scan_t);

  B32 is_visiable = dist_to_submarine < submarine->viewport_radius;
  B32 is_in_scan_range = dist_to_submarine <= scan_range;

  if(!is_visiable)
  {
    detectable->analyze_t = 0.0;
  }

  if(detectable->analyze_t != 1.0 && fabs(1.0 - detectable->analyze_t) < 0.01)
  {
    detectable->analyze_t = 1.0;
    sy_sequencer_play(s->sequencers[S5_SequencerKind_Info], 1);
  }

  B32 is_analyzed = detectable->analyze_t == 1.0;
  B32 is_analyzing = detectable->analyze_t > 0.0 && (!is_analyzed);

  if(is_analyzed)
  {
    // draw text
    String8 string = str8_cstring((char*)detectable->name);
    rk_debug_gfx(30, add_2f32(screen_pos, v2f32(-10, 30)), v4f32(1,1,1,1),string);
  }
  else
  {
    // analyzing just started
    if(!is_analyzing && is_in_scan_range)
    {
      sy_sequencer_play(s->sequencers[S5_SequencerKind_Info], 1);
    }
    if(is_analyzing || is_in_scan_range)
    {
      detectable->analyze_t += SUBMARINE_ANALYZE_SPEED*rk_state->frame_dt;
      is_analyzing = 1;
    }
  }

  if(is_analyzing)
  {
    String8 string = push_str8f(rk_frame_arena(), "%.2f%%", detectable->analyze_t);
    rk_debug_gfx(30, add_2f32(screen_pos, v2f32(-10, 30)), v4f32(1,1,1,1),string);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Scene Setup & Entry

RK_SCENE_SETUP(s5_setup)
{
  S5_Scene *s = scene->custom_data;

  ///////////////////////////////////////////////////////////////////////////////////////
  // load sounds

  s->sounds[S5_SoundKind_GainResource_AirBag]  = os_sound_from_file("./src/rubik/scenes/duskers/gain_resource_airbag.wav");
  s->sounds[S5_SoundKind_GainResource_Battery] = os_sound_from_file("./src/rubik/scenes/duskers/gain_resource_part.wav");
  s->sounds[S5_SoundKind_Ambient]              = os_sound_from_file("./src/rubik/scenes/duskers/ambience.wav");

  os_sound_set_volume(s->sounds[S5_SoundKind_Ambient], 0.3);
  os_sound_set_looping(s->sounds[S5_SoundKind_Ambient], 1);
  os_sound_play(s->sounds[S5_SoundKind_Ambient]);

  ///////////////////////////////////////////////////////////////////////////////////////
  // load instruments

  for(U64 i = 0; i < S5_InstrumentKind_COUNT; i++)
  {
    SY_Instrument **dst = &s->instruments[i];
    SY_Instrument *src = 0;
    switch(i)
    {
      case S5_InstrumentKind_Beep:
      {

        src = sy_instrument_alloc(str8_lit("beep"));
        src->env.attack_time  = 0.005f;
        src->env.decay_time   = 0.05f;
        src->env.release_time = 0.0f;
        src->env.start_amp    = 1.0f;
        src->env.sustain_amp  = 0.8f;
        {

          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 880.0;
          osc->kind = SY_OSC_Kind_Square;
          osc->amp = 1.0;
        }
      }break;
      case S5_InstrumentKind_Boop:
      {
        src = sy_instrument_alloc(str8_lit("boop"));
        src->env.attack_time  = 0.005f;
        src->env.decay_time   = 0.05f;
        src->env.release_time = 0.0f;
        src->env.start_amp    = 1.0f;
        src->env.sustain_amp  = 0.8f;
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 440.0;
          osc->kind = SY_OSC_Kind_Square;
          osc->amp = 1.0;
        }
      }break;
      case S5_InstrumentKind_Gear:
      {
        src = sy_instrument_alloc(str8_lit("gear"));
        src->env.attack_time  = 0.001f;
        src->env.decay_time   = 0.10f;
        src->env.release_time = 0.0f;
        src->env.start_amp    = 1.0f;
        src->env.sustain_amp  = 0.0f;

        // Main gear clunk body (low-frequency thump)
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 80.0;
          osc->kind = SY_OSC_Kind_Square;
        }

        // Add high-frequency metallic overtone
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 1000.0;
          osc->kind = SY_OSC_Kind_Saw;
          osc->amp = 1.0f;
        }

        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 2000.0;
          osc->kind = SY_OSC_Kind_Sine;
          osc->amp = 1.0f;
        }

        // Add some white noise for texture (if supported)
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->kind = SY_OSC_Kind_NoiseWhite;
          osc->amp = 0.3f;
        }
      } break;
      case S5_InstrumentKind_Radiation:
      {
        src = sy_instrument_alloc(str8_lit("radiation"));
        src->env.attack_time  = 0.001f;
        src->env.decay_time   = 0.001f;
        src->env.release_time = 0.0f;
        src->env.start_amp    = 3.0f;
        src->env.sustain_amp  = 0.1f;

        {
          // Main burst - high-pitched noise
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 0.0; // Assume 0.0 or special flag means white noise
          osc->kind = SY_OSC_Kind_NoiseBrown;
          osc->amp = 1.0;
        }
      }break;
      case S5_InstrumentKind_AirPressor:
      {
#if 1
        src = sy_instrument_alloc(str8_lit("air_pressor"));
        src->env.attack_time  = 0.001f;
        src->env.decay_time   = 0.8f;
        src->env.release_time = 0.0f;
        src->env.start_amp    = 1.0f;
        src->env.sustain_amp  = 0.1f;
        {

          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 0.0;
          osc->kind = SY_OSC_Kind_NoiseWhite;
          osc->amp = 1.0;
        }
#else
        src  = sy_instrument_alloc(str8_lit("air_pressor"));
        src->env.attack_time  = 0.001f;
        src->env.decay_time   = 0.0f;
        src->env.release_time = 0.0f;
        src->env.start_amp    = 1.0f;
        src->env.sustain_amp  = 1.0f;
        {

          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 0.0;
          osc->kind = SY_OSC_Kind_NoiseBrown;
          osc->amp = 1.0;
        }
#endif
      }break;
      case S5_InstrumentKind_Ping:
      {
        // ping: sonar-like clean sine tone with subtle fade out
        src = sy_instrument_alloc(str8_lit("ping"));
        src->env.attack_time = 0.01f;
        src->env.decay_time = 0.3f;
        src->env.release_time = 0.2f;
        src->env.start_amp = 3.0;
        src->env.sustain_amp = 0.0;
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 1500;
          osc->kind = SY_OSC_Kind_Sine;
          osc->amp = 1.0;
        }
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 440.0;
          osc->kind = SY_OSC_Kind_Square;
          osc->amp = 0.2f;
        }
      }break;
      case S5_InstrumentKind_Echo:
      {
        // echo: softer, lower sine wave to simulate sonar reflection
        src = sy_instrument_alloc(str8_lit("echo"));
        src->env.attack_time = 0.005f;
        src->env.decay_time = 0.6f;
        src->env.release_time = 0.3f;
        src->env.start_amp = 1.6;
        src->env.sustain_amp = 0.0;
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 880.0f*0.75f;
          osc->kind = SY_OSC_Kind_Sine;
          osc->amp = 1.0;
        }
      }break;
      default:{InvalidPath;}break;
    }
    *dst = src;
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // load sequencers

  for(U64 kind = 0; kind < S5_SequencerKind_COUNT; kind++)
  {
    switch(kind)
    {
      case S5_SequencerKind_SonarScan:
      {
        F32 tempo = 60.0f; // slower tempo for sonar feel
        SY_Sequencer *seq = sy_sequencer_alloc();
        seq->tempo = tempo;
        seq->beat_count = 4;
        seq->subbeat_count = 4;
        seq->total_subbeat_count = 16;
        seq->loop = 0;
        seq->volume = 1.0f;
        seq->subbeat_time = (60.0f / tempo) / (F32)4; // seconds per subbeat
        seq->duration = (60.0f / tempo) * 4; // total duration of this sequence
        {
          SY_Channel *channel = sy_sequencer_push_channel(seq);
          channel->beats = str8_lit("X..............."); // single sonar ping at start
          channel->instrument = s->instruments[S5_InstrumentKind_Ping];
        }
        {
          SY_Channel *channel = sy_sequencer_push_channel(seq);
          channel->beats = str8_lit(".....X..........");
          channel->instrument = s->instruments[S5_InstrumentKind_Echo];
        }
        s->sequencers[kind] = seq;
      }break;
      case S5_SequencerKind_Radiation:
      {
        F32 tempo = 1500.0/2;
        SY_Sequencer *seq = sy_sequencer_alloc();
        seq->tempo = tempo;
        seq->beat_count = 4;
        seq->subbeat_count = 4;
        seq->total_subbeat_count = 16;
        seq->loop = 0;
        seq->volume = 1.0f;
        seq->subbeat_time = (60.0f / tempo) / (F32)4; // seconds per subbeat
        seq->duration = (60.0f / tempo) * 4; // total duration of this sequence
        {
          SY_Channel *channel = sy_sequencer_push_channel(seq);
          // channel->beats = str8_lit("X.X.X.X.X.X.X.X.");
          channel->beats = str8_lit("???.?.??.??.??.?");
          channel->instrument = s->instruments[S5_InstrumentKind_Radiation];
        }
        s->sequencers[kind] = seq;
      }break;
      case S5_SequencerKind_Info:
      {
        F32 tempo = 90.0f; // slower, friendly tempo
        SY_Sequencer *seq = sy_sequencer_alloc();
        seq->tempo = tempo;
        seq->beat_count = 4;
        seq->subbeat_count = 4;
        seq->total_subbeat_count = 16;
        seq->loop = 0;
        seq->volume = 1.0f;
        seq->subbeat_time = (60.0f / tempo) / (F32)4;
        seq->duration = (60.0f / tempo) * 4;
        {
          SY_Channel *channel = sy_sequencer_push_channel(seq);
          channel->beats = str8_lit("X...............");
          channel->instrument = s->instruments[S5_InstrumentKind_Boop];
        }
        s->sequencers[kind] = seq;
      }break;
      case S5_SequencerKind_Warning:
      {
        F32 tempo = 60.0;
        SY_Sequencer *seq = sy_sequencer_alloc();
        seq->tempo = tempo;
        seq->beat_count = 4;
        seq->subbeat_count = 4;
        seq->total_subbeat_count = 16;
        seq->loop = 1;
        seq->volume = 1.0;
        seq->subbeat_time = (60.0/tempo) / (F32)4; // seconds per subbeat
        seq->duration = (60.0/tempo) * 4; // total duration of this sequence
        {
          SY_Channel *channel = sy_sequencer_push_channel(seq);
          channel->beats = str8_lit("X...X...........");
          channel->instrument = s->instruments[S5_InstrumentKind_Beep];
        }
        s->sequencers[kind] = seq;
      }break;
      case S5_SequencerKind_Critical:
      {
        F32 tempo = 240.0;
        SY_Sequencer *seq = sy_sequencer_alloc();
        seq->tempo = tempo;
        seq->beat_count = 4;
        seq->subbeat_count = 4;
        seq->total_subbeat_count = 16;
        seq->loop = 1;
        seq->volume = 1.0;
        seq->subbeat_time = (60.0/tempo) / (F32)4;
        seq->duration = (60.0/tempo) * 4;
        {
          SY_Channel *channel = sy_sequencer_push_channel(seq);
          channel->beats = str8_lit("X.X.X.X.X.X.X.X.");
          channel->instrument = s->instruments[S5_InstrumentKind_Beep];
        }
        // {
        //   SY_Channel *channel = sy_sequencer_push_channel(seq);
        //   channel->beats = str8_lit("...X...X...X...X");
        //   channel->instrument = s->instruments[S5_InstrumentKind_Boop];
        // }
        s->sequencers[kind] = seq;
      }break;
      default:{InvalidPath;}break;
    }
  }

  sy_sequencer_play(s->sequencers[S5_SequencerKind_Radiation], 0);
  sy_sequencer_set_looping(s->sequencers[S5_SequencerKind_Radiation], 1);
  sy_sequencer_set_volume(s->sequencers[S5_SequencerKind_Radiation], 0.0);
}

RK_SCENE_DEFAULT(s5_default)
{
  RK_Scene *ret = rk_scene_alloc();
  ret->name = str8_lit("submarine");
  ret->save_path = str8_lit("./src/rubik/scenes/5/default.tscn");
  // TODO(k): create a macro to avoid using string literal here
  ret->setup_fn_name = str8_lit("s5_setup");
  ret->default_fn_name = str8_lit("s5_default");
  ret->update_fn_name = str8_lit("s5_update");

  rk_push_scene(ret);
  rk_push_node_bucket(ret->node_bucket);
  rk_push_res_bucket(ret->res_bucket);
  rk_push_handle_seed(ret->handle_seed);

  ///////////////////////////////////////////////////////////////////////////////////////
  // scene settings

  ret->omit_grid = 1;
  ret->omit_gizmo3d = 1;
  ret->omit_light = 1;

  // scene data
  S5_Scene *scene = rk_scene_push_custom_data(ret, S5_Scene);

  // 2d viewport
  // Rng2F32 viewport_screen = {0,0,600,600};
  // Vec2F32 viewport_screen_dim = dim_2f32(viewport_screen);
  Rng2F32 viewport_world = rk_state->window_rect;

  // root node
  RK_Node *root = rk_build_node3d_from_stringf(0,0, "root");

  ///////////////////////////////////////////////////////////////////////////////////////
  // load resources

  // RK_Handle tileset = rk_tileset_from_dir(str8_lit("./textures/isometric-tiles-2"), rk_key_zero());

  ///////////////////////////////////////////////////////////////////////////////////////
  // build node tree

  RK_Parent_Scope(root)
  {
    // create the orthographic camera 
    RK_Node *game_camera_node = rk_build_camera3d_from_stringf(0, 0, "camera2d");
    {
      // game_camera_node->camera3d->viewport = viewport_screen;
      game_camera_node->camera3d->projection = RK_ProjectionKind_Orthographic;
      game_camera_node->camera3d->viewport_shading = RK_ViewportShadingKind_Material;
      game_camera_node->camera3d->polygon_mode = R_GeoPolygonKind_Fill;
      game_camera_node->camera3d->hide_cursor = 0;
      game_camera_node->camera3d->lock_cursor = 0;
      game_camera_node->camera3d->is_active = 1;
      game_camera_node->camera3d->zn = -0.1;
      game_camera_node->camera3d->zf = 1000; // support 1000 layers
      game_camera_node->camera3d->orthographic.top    = viewport_world.y0;
      game_camera_node->camera3d->orthographic.bottom = viewport_world.y1;
      game_camera_node->camera3d->orthographic.left   = viewport_world.x0;
      game_camera_node->camera3d->orthographic.right  = viewport_world.x1;
      game_camera_node->node3d->transform.position = v3f32(0,0,0);
      game_camera_node->custom_flags = S5_EntityFlag_GameCamera;

      S5_Entity *entity = rk_node_push_custom_data(game_camera_node, S5_Entity);
      S5_GameCamera *camera = &entity->game_camera;
      camera->viewport_world = viewport_world;
      camera->viewport_world_target = viewport_world;
    }
    ret->active_camera = rk_handle_from_node(game_camera_node);

    // sea
    RK_Node *sea_node = 0;
    {
      RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D, 0, "sea");
      node->node2d->transform.position = v2f32(0., 0.);
      node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
      node->sprite2d->size.rect = v2f32(3000,3000);
      node->sprite2d->color = v4f32(0,0,0,1);
      node->sprite2d->omit_texture = 1;
      node->node2d->z_index = 2;
      sea_node = node;
    }
    scene->sea = rk_handle_from_node(sea_node);

    // submarine
    RK_Node *submarine_node = 0;
    {
      RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Collider2D|RK_NodeTypeFlag_Sprite2D, 0, "submarine");
      node->node2d->transform.position = v2f32(900.0, 150.0);
      node->sprite2d->anchor = RK_Sprite2DAnchorKind_Center;
      node->sprite2d->size.rect = v2f32(60,60);
      node->sprite2d->color = v4f32(0,0,1,1);
      node->sprite2d->omit_texture = 1;
      node->sprite2d->draw_edge = 1;
      submarine_node = node;
      node->custom_flags |= S5_EntityFlag_Submarine;

      S5_Entity *entity = rk_node_push_custom_data(node, S5_Entity);
      entity->submarine.viewport_radius = 500;
      entity->submarine.scan_radius = 500;
      entity->submarine.magnetic_range = 3500;
      entity->submarine.density = 1; // TODO: don't have any usage for now
      entity->submarine.oxygen = 600.0;
      entity->submarine.battery = 1000.0;
      entity->submarine.hp = 100.0;
    }
    scene->submarine = rk_handle_from_node(submarine_node);

    // flock and boids
    for(U64 j = 0; j < 1; j++)
    {
      RK_Node *flock_node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D, 0, "flock_%I64u", j);
      flock_node->custom_flags |= S5_EntityFlag_Flock;
      S5_Entity *entity = rk_node_push_custom_data(flock_node, S5_Entity);
      entity->flock.w_target = 50000;
      entity->flock.w_alignment = 2000;
      entity->flock.w_separation = 60000;
      entity->flock.w_cohesion = 15000;
      entity->flock.path_start = v2f32(30,1000);
      entity->flock.path_end = v2f32(9000,1000);
      entity->flock.target_pos = entity->flock.path_end;

      RK_Parent_Scope(flock_node)
      {
        Rng1U32 spwan_range_x = {0, 30};
        Rng1U32 spwan_range_y = {0, 30};
        U32 spwan_dim_x = dim_1u32(spwan_range_x);
        U32 spwan_dim_y = dim_1u32(spwan_range_y);

        // spawn boids
        for(U64 i = 0; i < 300; i++)
        {
          F32 x = (rand_u32()%spwan_dim_x) + spwan_range_x.min;
          F32 y = (rand_u32()%spwan_dim_y) + spwan_range_y.min;
          RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Collider2D|RK_NodeTypeFlag_Sprite2D, 0, "flock_%I64u", i);
          node->node2d->transform.position = v2f32(x, y);
          node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
          node->sprite2d->shape = RK_Sprite2DShapeKind_Rect;
          node->sprite2d->size.rect = v2f32(10,10);
          node->sprite2d->color = v4f32(0.9,0.0,0.1,1.);
          node->sprite2d->omit_texture = 1;
          node->sprite2d->draw_edge = 1;
          node->custom_flags |= S5_EntityFlag_Boid|S5_EntityFlag_ExternalObject;
          S5_Entity *entity = rk_node_push_custom_data(node, S5_Entity);
          S5_Boid *boid = &entity->boid;
          boid->min_depth = y - 30;
          boid->max_depth = y + 30;
          boid->motivation = 1.0;
        }
      }
    }

    // spawn some resources
    {
      Rng1U32 spwan_range_x = {0, 9000};
      Rng1U32 spwan_range_y = {0, 9000};
      U32 spwan_dim_x = dim_1u32(spwan_range_x);
      U32 spwan_dim_y = dim_1u32(spwan_range_y);
      for(U64 i = 0; i < 100; i++)
      {
        F32 x = (rand_u32()%spwan_dim_x) + spwan_range_x.min;
        F32 y = (rand_u32()%spwan_dim_y) + spwan_range_y.min;

        RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Collider2D|RK_NodeTypeFlag_Sprite2D, 0, "resource_%I64u", i);
        node->node2d->transform.position = v2f32(x, y);
        node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
        node->sprite2d->shape = RK_Sprite2DShapeKind_Rect;
        node->sprite2d->size.rect = v2f32(30,30);
        node->sprite2d->color = v4f32(0.3,0.1,0.1,1.);
        node->sprite2d->omit_texture = 1;
        node->sprite2d->draw_edge = 1;
        node->custom_flags = S5_EntityFlag_Resource|S5_EntityFlag_ExternalObject|S5_EntityFlag_Detectable;

        S5_Entity *entity = rk_node_push_custom_data(node, S5_Entity);
        S5_Resource *resource = &entity->resource;

        S5_ResourceKind resource_kind = (U64)(rand_u32()%S5_ResourceKind_COUNT);
        String8 resource_name = {0};
        switch(resource_kind)
        {
          case S5_ResourceKind_AirBag:
          {
            resource->value.airbag.oxygen = 200.0;
            resource_name = str8_lit("AirBag");
          }break;
          case S5_ResourceKind_Battery:
          {
            resource->value.battery.joules = 100.0;
            resource_name = str8_lit("Battery");
          }break;
          default:{InvalidPath;}break;
        }
        resource->kind = resource_kind;
        push_str8_copy_static(resource_name, entity->detectable.name);
      }
    }

    // spawn some predators
    {
      Rng1U32 spwan_range_x = {0, 9000};
      Rng1U32 spwan_range_y = {0, 9000};
      U32 spwan_dim_x = dim_1u32(spwan_range_x);
      U32 spwan_dim_y = dim_1u32(spwan_range_y);
      for(U64 i = 0; i < 80; i++)
      {
        F32 x = (rand_u32()%spwan_dim_x) + spwan_range_x.min;
        F32 y = (rand_u32()%spwan_dim_y) + spwan_range_y.min;

        RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Collider2D|RK_NodeTypeFlag_Sprite2D,
                                                   0, "predator_%I64u", i);
        node->node2d->transform.position = v2f32(x, y);
        node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
        node->sprite2d->shape = RK_Sprite2DShapeKind_Rect;
        node->sprite2d->size.rect = v2f32(30,30);
        node->sprite2d->color = v4f32(0.3,0.1,0.1,1.);
        node->sprite2d->omit_texture = 1;
        node->sprite2d->draw_edge = 1;
        node->custom_flags = S5_EntityFlag_Predator|S5_EntityFlag_ExternalObject|S5_EntityFlag_Detectable;

        S5_Entity *entity = rk_node_push_custom_data(node, S5_Entity);
        push_str8_copy_static(str8_lit("predator"), entity->detectable.name);
        entity->predator.current_energy = 600.0;
        entity->predator.max_energy = 600.0;
        entity->predator.energy_recovery_rate = 0.7;
        entity->predator.roaming_speed = 0.0;
        entity->predator.burst_speed = 200.0;
        entity->predator.k_drag = 0.002;
      }
    }

    // spawn docking gate
    {
      Rng1U32 spwan_range_x = {0, 9000};
      Rng1U32 spwan_range_y = {0, 9000};
      U32 spwan_dim_x = dim_1u32(spwan_range_x);
      U32 spwan_dim_y = dim_1u32(spwan_range_y);
      for(U64 i = 0; i < 3; i++)
      {
        F32 x = (rand_u32()%spwan_dim_x) + spwan_range_x.min;
        F32 y = (rand_u32()%spwan_dim_y) + spwan_range_y.min;

        RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Collider2D|RK_NodeTypeFlag_Sprite2D,
                                                   0, "docking_gate_%I64u", i);
        node->node2d->transform.position = v2f32(x, y);
        node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
        node->sprite2d->shape = RK_Sprite2DShapeKind_Rect;
        node->sprite2d->size.rect = v2f32(30,30);
        node->sprite2d->color = v4f32(0.3,0.1,0.1,1.);
        node->sprite2d->omit_texture = 1;
        node->sprite2d->draw_edge = 1;
        node->custom_flags = S5_EntityFlag_ExternalObject|S5_EntityFlag_Detectable|S5_EntityFlag_DockingGate;

        S5_Entity *entity = rk_node_push_custom_data(node, S5_Entity);
        push_str8_copy_static(str8_lit("Gate"), entity->detectable.name);
      }
    }
  }

  ret->root = rk_handle_from_node(root);
  rk_pop_scene();
  rk_pop_node_bucket();
  rk_pop_res_bucket();
  rk_pop_handle_seed();

  // TODO(k): maybe set it somewhere else
  ret->setup_fn = s5_setup;
  ret->update_fn = s5_update;
  ret->default_fn = s5_default;

  // TODO(k): call it somewhere else
  s5_setup(ret);
  return ret;
}
