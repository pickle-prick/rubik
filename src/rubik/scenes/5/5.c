/////////////////////////////////////////////////////////////////////////////////////////
// Basic Type/Enum

typedef U64 S5_EntityFlags;
#define S5_EntityFlag_Boid     (S5_EntityFlags)(1ull<<0)
#define S5_EntityFlag_Resource (S5_EntityFlags)(1ull<<1)

typedef enum S5_ResourceKind
{
  S5_ResourceKind_AirBag,
  S5_ResourceKind_Battery,
  S5_ResourceKind_COUNT,
} S5_ResourceKind;

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

  OS_Handle speaker_stream;
  OS_Handle sound;

  // per-frame build artifacts
  Vec2F32 world_mouse;
  QuadTree *root_quad;
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
      F32 kwh;
    } battery;

    F32 v[1];
  } value;
};

typedef struct S5_Submarine S5_Submarine;
struct S5_Submarine
{
  Vec2F32 v;
  F32 density;
  F32 min_density;
  F32 max_density;
  F32 drag;

  F32 viewport_radius;

  B32 is_scanning;
  F32 scan_radius;
  F32 scan_t;

  // health?

  // resource
  F32 oxygen;
  F32 power_kwh;
};

typedef struct S5_Flock S5_Flock;
struct S5_Flock
{
  F32 w_target;
  F32 w_separation;
  F32 w_alignment;
  F32 w_cohesion;
};

typedef struct S5_Boid S5_Boid;
struct S5_Boid
{
  F32 max_depth;
  F32 min_depth;

  Vec2F32 vel;
  Vec2F32 acc;
  Vec2F32 target_vel;

  F32 motivation;
  F32 last_motivation_update_time;
};

#define S5_SEA_FLUID_DENSITY    1025.0
#define S5_SCALE_WORLD_TO_METER 0.001

/////////////////////////////////////////////////////////////////////////////////////////
// Helper Functions

internal void
s5_audio_stream_output_callback(void *buffer, U64 frame_count)
{
  local_persist F32 phase = 0;
  // TODO: use device sample size here
  F32 phase_step = (440.0f * tau32) / 44100.0f;
  F32 *dst = buffer;

  for(U64 i = 0; i < frame_count; i++)
  {
    // TODO: use device channel count here
    for(U64 c = 0; c < 2; c++)
    {
      F32 value = sinf(phase);
      *dst++ = (value >= 0) ? 0.3f : -0.3f;
    }
    phase += phase_step;
    if(phase > tau32) phase -= tau32; // wrap phase
  }
}

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
// Update Functions

RK_NODE_CUSTOM_UPDATE(s5_fn_camera)
{
  S5_Camera *camera = node->custom_data;

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

RK_NODE_CUSTOM_UPDATE(s5_fn_scene_begin)
{
  S5_Scene *s = scene->custom_data;
  S5_Submarine *submarine = rk_node_from_handle(&s->submarine)->custom_data;
  RK_NodeBucket *node_bucket = scene->node_bucket;
  s->world_mouse = s5_world_position_from_mouse(rk_state->cursor, rk_state->window_dim, ctx->proj_view_inv_m);

  if(submarine->scan_t > 0.001)
  {
    os_audio_stream_set_volume(s->speaker_stream, 0.1);
  }
  else
  {
    os_audio_stream_set_volume(s->speaker_stream, 0);
  }

  // TODO(XXX): not ideal, how can find a resonal bounds for the world 
  // we may want to collect the 2d world bounds for sprite2d
  QuadTree *root_quad = quadtree_push(rk_frame_arena(), r2f32p(-400000,-400000,400000,400000));

  for(U64 slot_idx = 0; slot_idx < node_bucket->hash_table_size; slot_idx++)
  {
    RK_NodeBucketSlot *slot = &node_bucket->hash_table[slot_idx];
    for(RK_Node *n = slot->first; n != 0; n = n->hash_next)
    {
      // TODO(XXX): we should only add Collider2D instead of Sprite2D
      if(n->type_flags & RK_NodeTypeFlag_Collider2D)
      {
        Vec2F32 position = n->node2d->transform.position;
        Rng2F32 src_rect = rk_rect_from_sprite2d(n->sprite2d, position);
        quadtree_insert(rk_frame_arena(), root_quad, src_rect, n);
      }
    }
  }
  s->root_quad = root_quad;

  // draw a debug rect
  if(BUILD_DEBUG) RK_Parent_Scope(rk_node_from_handle(&scene->root))
  {
    Vec2F32 mouse_world = s5_world_position_from_mouse(rk_state->cursor, rk_state->window_dim, ctx->proj_view_inv_m);
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
}

RK_NODE_CUSTOM_UPDATE(s5_fn_game_ui)
{
  S5_Scene *s = scene->custom_data;
  RK_Node *sea_node = rk_node_from_handle(&s->sea);
  RK_Node *submarine_node = rk_node_from_handle(&s->submarine);
  S5_Submarine *submarine = submarine_node->custom_data;

  D_BucketScope(rk_state->bucket_rect)
  {
    UI_Box *container = 0;
    UI_Rect(rk_state->window_rect)
    {
      container = ui_build_box_from_stringf(0, "###game_overlay");
    }

    UI_Parent(container)
    {
      UI_Column
        UI_Flags(UI_BoxFlag_DrawBorder)
      {
        ui_labelf("density: %f", submarine->density);
        ui_labelf("scan_t: %f", submarine->scan_t);
        ui_labelf("oxygen: %f", submarine->oxygen);
        ui_labelf("power_kwh: %f", submarine->power_kwh);
      }
    }
  }
}

RK_NODE_CUSTOM_UPDATE(s5_fn_submarine)
{
  Temp scratch = scratch_begin(0,0);
  S5_Scene *s = scene->custom_data;
  S5_Submarine *submarine = node->custom_data;
  RK_Transform2D *transform = &node->node2d->transform;
  RK_Node *sea_node = rk_node_from_handle(&s->sea);
  RK_Sprite2D *sprite2d = node->sprite2d;

  F32 y_in = transform->position.y+sprite2d->size.rect.y-sea_node->node2d->transform.position.y;
  y_in = Clamp(0, y_in, sprite2d->size.rect.y);
  F32 volume = (sprite2d->size.rect.x*S5_SCALE_WORLD_TO_METER) * (sprite2d->size.rect.y*S5_SCALE_WORLD_TO_METER);
  F32 volume_displaced = sprite2d->size.rect.x*y_in*S5_SCALE_WORLD_TO_METER*S5_SCALE_WORLD_TO_METER;

  F32 g = 9.8*1000.0; /* gravitational acceleration */
  F32 mass = submarine->density * volume;
  F32 mass_displaced = S5_SEA_FLUID_DENSITY * volume_displaced;

  // force accmulator
  Vec2F32 F = {0};

  // thrust
  if(os_key_is_down(OS_Key_Up))
  {
    F32 remain_kwh = submarine->power_kwh;
    remain_kwh -= 10*rk_state->frame_dt;
    if(remain_kwh >= 0)
    {
      submarine->power_kwh = remain_kwh;
      F.y = -1000000;
    }
  }
  if(os_key_is_down(OS_Key_Left))
  {
    F32 remain_kwh = submarine->power_kwh;
    remain_kwh -= 10*rk_state->frame_dt;
    if(remain_kwh >= 0)
    {
      submarine->power_kwh = remain_kwh;
      F.x = -1000000;
    }
  }
  if(os_key_is_down(OS_Key_Right))
  {
    F32 remain_kwh = submarine->power_kwh;
    remain_kwh -= 10*rk_state->frame_dt;
    if(remain_kwh >= 0)
    {
      submarine->power_kwh = remain_kwh;
      F.x = 1000000;
    }
  }
  if(os_key_is_down(OS_Key_Down))
  {
    F32 remain_kwh = submarine->power_kwh;
    remain_kwh -= 10*rk_state->frame_dt;
    if(remain_kwh >= 0)
    {
      submarine->power_kwh = remain_kwh;
      F.y = 1000000;
    }
  }

  if(ui_key_press(0, OS_Key_W))
  {
    os_sound_play(s->sound);
  }

  // TODO(XXX): we should move these physics update into fixed_update
  F32 density = submarine->density;
  if(os_key_is_down(OS_Key_W))
  {
    density -= 300*rk_state->frame_dt;
  }
  if(os_key_is_down(OS_Key_S))
  {
    density += 300*rk_state->frame_dt;
  }
  density = Clamp(submarine->min_density, density, submarine->max_density);

  F32 drag = 6000;
  Vec2F32 v = submarine->v;
  F.y += (mass-mass_displaced)*g;
  F.x += -drag*v.x;
  F.y += -drag*v.y;
  Vec2F32 acc = {F.x/mass, F.y/mass};
  v.x += acc.x * rk_state->frame_dt;
  v.y += acc.y * rk_state->frame_dt;
  Vec2F32 pos = transform->position;
  pos.x += v.x * rk_state->frame_dt;
  pos.y += v.y * rk_state->frame_dt;

  submarine->v = v;
  transform->position = pos;
  submarine->density = density;

  if(ui_key_press(0, OS_Key_Space))
  {
    submarine->is_scanning = 1;
  }

  if(submarine->is_scanning)
  {
    submarine->scan_t += 1 * rk_state->frame_dt;
    if(submarine->scan_t > 1.0)
    {
      submarine->scan_t = 0.0;
      submarine->is_scanning = 0;
    }

    submarine->scan_t = Clamp(0, submarine->scan_t, 1);
  }

  // draw viewport
  RK_Parent_Scope(node)
  {
    RK_Node *n = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D,
                                            RK_NodeFlag_Transient, "viewport");
    Vec2F32 position = scale_2f32(node->sprite2d->size.rect, 0.5);
    F32 radius = submarine->viewport_radius;

    n->sprite2d->anchor = RK_Sprite2DAnchorKind_Center;
    n->sprite2d->shape = RK_Sprite2DShapeKind_Circle;
    n->sprite2d->size.circle.radius = radius;
    n->sprite2d->color = v4f32(0.1,0,0,0.5);
    n->sprite2d->omit_texture = 1;
    n->sprite2d->draw_edge = 1;
    n->node2d->transform.position = position;
    n->node2d->z_index = 1;
  }

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
    n->sprite2d->color = v4f32(1,0,0,0.1);
    n->sprite2d->omit_texture = 1;
    n->node2d->transform.position = position;
    n->node2d->z_index = 1;
  }

  // check collision with resource
  {
    // TODO: add some helper function to make this easier
    Vec2F32 position = node->node2d->transform.position;
    Rng2F32 src_rect = rk_rect_from_sprite2d(node->sprite2d, position);
    // src_rect = pad_2f32(src_rect, 0.1);

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
      S5_Resource *resource = resource_node->custom_data;

      switch(resource->kind)
      {
        case S5_ResourceKind_AirBag:
        {
          submarine->oxygen += resource->value.airbag.oxygen;
        }break;
        case S5_ResourceKind_Battery:
        {
          submarine->power_kwh += resource->value.battery.kwh;
        }break;
        default:{InvalidPath;}break;
      }

      rk_node_release(resource_node);
    }
  }
  scratch_end(scratch);
}

RK_NODE_CUSTOM_UPDATE(s5_fn_flock)
{
  Temp scratch = scratch_begin(0,0);
  S5_Scene *s = scene->custom_data;
  S5_Flock *flock = node->custom_data;

  RK_Node **boids = 0;
  for(RK_Node *child = node->first; child != 0; child = child->next)
  {
    darray_push(scratch.arena, boids, child);
  }

  RK_Node *submarine_node = rk_node_from_handle(&s->submarine);
  S5_Submarine *submarine = submarine_node->custom_data;
  Vec2F32 submarine_center = submarine_node->node2d->transform.position;
  submarine_center = add_2f32(submarine_center, scale_2f32(submarine_node->sprite2d->size.rect, 0.5));

  for(U64 j = 0; j < darray_size(boids); j++)
  {
    RK_Node *boid_node = boids[j];
    S5_Boid *boid = boid_node->custom_data;
    RK_Transform2D *transform = &boid_node->node2d->transform;
    Vec2F32 size = boid_node->sprite2d->size.rect;
    Vec2F32 position = boid_node->node2d->transform.position;

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
    // move to cursor

    {
      // TODO(XXX): we should make the direction turning slower

      F32 dist_self_to_target = length_2f32(sub_2f32(position, submarine_center));
      Vec2F32 dir_self_to_target = normalize_2f32(sub_2f32(submarine_center, position));
      // Vec2F32 dir_self_to_target = normalize_2f32(sub_2f32(s->world_mouse, position));
      // F32 dist_self_to_target = length_2f32(sub_2f32(s->world_mouse, position)); /* to mouse */

      F32 angle_rad = ((rand()/(F32)RAND_MAX) * (1.0/2) - (1.0/4));
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
      }
      else
      {
        F32 w = flock->w_target;
        Vec2F32 uf = scale_2f32(dir, w);
        acc = add_2f32(acc, uf);
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
          S5_Boid *rhs_boid = rhs_node->custom_data;
          Vec2F32 rhs_position = rhs_node->node2d->transform.position;
          Vec2F32 rhs_to_self = sub_2f32(position, rhs_position);
          F32 dist = length_2f32(rhs_to_self);

          F32 min_dist = size.x*0.6;
          if(dist < min_dist)
          {
            F32 w = flock->w_separation * (dist/min_dist);
            // unit force (assuming mass is 1, so it's acceleration)
            Vec2F32 uf = scale_2f32(normalize_2f32(rhs_to_self), w);
            acc = add_2f32(acc, uf);
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
          S5_Boid *rhs_boid = rhs_node->custom_data;
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
          S5_Boid *rhs_boid = rhs_node->custom_data;
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
      }
    }

    // F32 scale = 1.0 * (rand()/(F32)RAND_MAX) * (1);
    Vec2F32 vel = scale_2f32(acc, rk_state->frame_dt*boid->motivation);
    boid->acc = acc;
    boid->target_vel = vel;
  }
  scratch_end(scratch);
}

RK_NODE_CUSTOM_UPDATE(s5_fn_boid)
{
  S5_Scene *s = scene->custom_data;
  S5_Boid *fish = node->custom_data;
  RK_Sprite2D *sprite2d = node->sprite2d;
  RK_Transform2D *transform = &node->node2d->transform;
  Vec2F32 size = node->sprite2d->size.rect;
  Vec2F32 position = node->node2d->transform.position;

  RK_Node *submarine_node = rk_node_from_handle(&s->submarine);
  S5_Submarine *submarine = submarine_node->custom_data;
  Vec2F32 submarine_center = submarine_node->node2d->transform.position;
  submarine_center = add_2f32(submarine_center, scale_2f32(submarine_node->sprite2d->size.rect, 0.5));
  F32 dist_to_submarine = length_2f32(sub_2f32(submarine_center, node->node2d->transform.position));

  B32 is_visiable = dist_to_submarine < submarine->viewport_radius;

  fish->vel = mix_2f32(fish->target_vel, fish->vel, 0.7);
  // fish->vel = fish->target_vel;

  // fish->vel = fish->target_vel;
  // Vec2F32 vel_normalized = normalize_2f32(fish->vel);
  // F32 vel_mag = length_2f32(fish->vel);
  // printf("vel_mag: %f\n", vel_mag);
  // F32 speed = Clamp(0, vel_mag, 900);
  // fish->vel = scale_2f32(vel_normalized, speed);

  Vec2F32 d_position = scale_2f32(fish->vel, rk_state->frame_dt);
  position = add_2f32(position, d_position);
  node->node2d->transform.position = position;

  F32 update_dt = os_now_microseconds()/1000000.0 - fish->last_motivation_update_time;
  if(update_dt > 6.0)
  {
    fish->motivation = 1 + (rand()/(F32)RAND_MAX) * 1;
  }

  if(!is_visiable)
  {
    sprite2d->color.w = 0;
    sprite2d->draw_edge = 0;
  }
  else
  {
    sprite2d->color.w = 1;
    sprite2d->draw_edge = 1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Scene Entry

internal RK_Scene *
rk_scene_entry__5()
{
  RK_Scene *ret = rk_scene_alloc();
  ret->name = str8_lit("submarine");
  ret->save_path = str8_lit("./src/rubik/scenes/5/default.tscn");
  ret->reset_fn = str8_lit("rk_scene_entry__5");

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
  OS_Handle speaker_stream = os_audio_stream_alloc(48000, sizeof(F32), 1);
  os_audio_stream_set_output_callback(speaker_stream, s5_audio_stream_output_callback);
  os_audio_stream_play(speaker_stream);
  os_audio_stream_set_volume(speaker_stream, 0.1);
  // os_audio_stream_pause(speaker_stream);
  scene->speaker_stream = speaker_stream;

  // 2d viewport
  // Rng2F32 viewport_screen = {0,0,600,600};
  // Vec2F32 viewport_screen_dim = dim_2f32(viewport_screen);
  Rng2F32 viewport_world = rk_state->window_rect;

  // root node
  RK_Node *root = rk_build_node3d_from_stringf(0,0, "root");
  rk_node_push_fn(root, s5_fn_scene_begin);
  rk_node_push_fn(root, s5_fn_game_ui);

  ///////////////////////////////////////////////////////////////////////////////////////
  // load resources

  // RK_Handle tileset = rk_tileset_from_dir(str8_lit("./textures/isometric-tiles-2"), rk_key_zero());

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
      rk_node_push_fn(main_camera, s5_fn_camera);
    }
    ret->active_camera = rk_handle_from_node(main_camera);

    // sea
    RK_Node *sea_node = 0;
    {
      RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Sprite2D, 0, "sea");
      node->node2d->transform.position = v2f32(0., 0.);
      node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
      node->sprite2d->size.rect = v2f32(300000,300000);
      // node->sprite2d->color = v4f32(0.,0.1,0.5,1.);
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
      node->node2d->transform.position = v2f32(900., -150.);
      node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
      node->sprite2d->size.rect = v2f32(900,300);
      node->sprite2d->color = v4f32(0,0,1,1);
      node->sprite2d->omit_texture = 1;
      node->sprite2d->draw_edge = 1;
      rk_node_push_fn(node, s5_fn_submarine);
      submarine_node = node;

      S5_Submarine *submarine = rk_node_push_custom_data(node, S5_Submarine);
      submarine->viewport_radius = 1000;
      submarine->scan_radius = 1000;
      submarine->drag = 0.96;
      submarine->density = S5_SEA_FLUID_DENSITY/2.0;
      submarine->min_density = S5_SEA_FLUID_DENSITY/2.0;
      submarine->max_density = S5_SEA_FLUID_DENSITY;
      submarine->oxygen = 100.0;
      submarine->power_kwh = 100.0;
    }
    scene->submarine = rk_handle_from_node(submarine_node);

    // flock and boids
    for(U64 j = 0; j < 1; j++)
    {
      RK_Node *flock_node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D, 0, "flock_%I64u", j);
      rk_node_push_fn(flock_node, s5_fn_flock);
      S5_Flock *flock = rk_node_push_custom_data(flock_node, S5_Flock);
      flock->w_target = 50000;
      flock->w_alignment = 2000;
      flock->w_separation = 60000;
      flock->w_cohesion = 15000;

      RK_Parent_Scope(flock_node)
      {
        Rng1U32 spwan_range_x = {0, 3000};
        Rng1U32 spwan_range_y = {0, 3000};
        U32 spwan_dim_x = dim_1u32(spwan_range_x);
        U32 spwan_dim_y = dim_1u32(spwan_range_y);

        // spawn boids
        for(U64 i = 0; i < 300; i++)
        {
          F32 x = (rand()%spwan_dim_x) + spwan_range_x.min;
          F32 y = (rand()%spwan_dim_y) + spwan_range_y.min;
          RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Collider2D|RK_NodeTypeFlag_Sprite2D, 0, "flock_%I64u", i);
          node->node2d->transform.position = v2f32(x, y);
          node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
          node->sprite2d->shape = RK_Sprite2DShapeKind_Rect;
          node->sprite2d->size.rect = v2f32(10,10);
          node->sprite2d->color = v4f32(0.9,0.0,0.1,1.);
          node->sprite2d->omit_texture = 1;
          node->sprite2d->draw_edge = 1;
          node->custom_flags |= S5_EntityFlag_Boid;
          rk_node_push_fn(node, s5_fn_boid);
          S5_Boid *boid = rk_node_push_custom_data(node, S5_Boid);
          boid->min_depth = y - 30;
          boid->max_depth = y + 30;
          boid->motivation = 1.0;
        }
      }
    }

    // spawn some resources
    {
      Rng1U32 spwan_range_x = {0, 3000};
      Rng1U32 spwan_range_y = {0, 3000};
      U32 spwan_dim_x = dim_1u32(spwan_range_x);
      U32 spwan_dim_y = dim_1u32(spwan_range_y);
      for(U64 i = 0; i < 30; i++)
      {
        F32 x = (rand()%spwan_dim_x) + spwan_range_x.min;
        F32 y = (rand()%spwan_dim_y) + spwan_range_y.min;

        RK_Node *node = rk_build_node_from_stringf(RK_NodeTypeFlag_Node2D|RK_NodeTypeFlag_Collider2D|RK_NodeTypeFlag_Sprite2D, 0, "resource_%I64u", i);
        node->node2d->transform.position = v2f32(x, y);
        node->sprite2d->anchor = RK_Sprite2DAnchorKind_TopLeft;
        node->sprite2d->shape = RK_Sprite2DShapeKind_Rect;
        node->sprite2d->size.rect = v2f32(100,100);
        node->sprite2d->color = v4f32(0.3,0.1,0.1,1.);
        node->sprite2d->omit_texture = 1;
        node->sprite2d->draw_edge = 1;
        node->custom_flags = S5_EntityFlag_Resource;
        S5_Resource *resource = rk_node_push_custom_data(node, S5_Resource);

        S5_ResourceKind resource_kind = (U64)(rand()%S5_ResourceKind_COUNT);
        switch(resource_kind)
        {
          case S5_ResourceKind_AirBag:
          {
            resource->value.airbag.oxygen = 30.0;
            node->sprite2d->color = v4f32(0.1,0.3,0.1,1.);
          }break;
          case S5_ResourceKind_Battery:
          {
            resource->value.battery.kwh = 30.0;
          }break;
          default:{InvalidPath;}break;
        }
        resource->kind = resource_kind;
      }
    }
  }

  // TESTING
  OS_Handle sound = os_sound_from_file("./src/rubik/scenes/5/0a-0.wav");
  scene->sound = sound;

  ret->root = rk_handle_from_node(root);
  rk_pop_scene();
  rk_pop_node_bucket();
  rk_pop_res_bucket();
  rk_pop_handle_seed();
  return ret;
}
