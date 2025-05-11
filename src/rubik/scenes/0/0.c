// Camera (editor camera)
RK_NODE_CUSTOM_UPDATE(editor_camera_fn)
{
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
    F32 h_speed_per_screen_px = 4./rk_state->window_dim.x;
    F32 h_pct = -delta.x * h_speed_per_screen_px;
    Vec3F32 h_dist = scale_3f32(s, h_pct);

    F32 v_speed_per_screen_px = 4./rk_state->window_dim.y;
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
  F32 slow_rate = 1 - pow_f32(2, (-90.f * ctx->dt_sec));
  F32 fast_rate = 1 - pow_f32(2, (-40.f * ctx->dt_sec));

  // animating intensity
  // light->intensity += fast_rate * (light->intensity > 0.999f ? 0 : 1 - light->intensity);
  // light->range += slow_rate * (data->target_range - light->range);

  QuatF32 r = make_rotate_quat_f32(data->rotation_axis, 0.3*ctx->dt_sec);
  node->node3d->transform.rotation = mul_quat_f32(r, node->node3d->transform.rotation);
}

RK_NODE_CUSTOM_UPDATE(spring_knot)
{
  RK_Particle3D *p = node->particle3d;
  B32 on_ground = 0;
  if(p->v.x.y > -0.2)
  {
    p->v.x.y = -0.2;
    on_ground = 1;
  }

  if(on_ground)
  {
    p->v.flags |= PH_Particle3DFlag_OmitGravity;
    p->v.v.y = 0;
    // TODO(XXX): we don't have rigidbody for now, use visous drag to simulate friction
    p->v.visous_drag.kd = 10;
  }
  else
  {
    p->v.flags &= ~PH_Particle3DFlag_OmitGravity;
    p->v.visous_drag.kd = 0;
  }

  // PH_Force3D *f = push_array(rk_frame_arena(), PH_Force3D, 1);
  // f->kind = PH_Force3DKind_Const;
  // f->v.constf.direction = v3f32(1,0,0);
  // f->v.constf.strength = 1.f;
  // f->targets.v = p;
  // f->target_count = 1;
  // // TODO(XXX): maybe we don't need idx for force
  // f->idx = scene->particle3d_system.force_count++;
  // SLLQueuePush(scene->particle3d_system.first_force,
  //              scene->particle3d_system.last_force,
  //              f);
}

RK_NODE_CUSTOM_UPDATE(body)
{
  RK_Rigidbody3D *rb = node->rigidbody3d;
  B32 on_ground = 0;
  if(rb->v.x.y > -0.2)
  {
    rb->v.x.y = -0.2;
    on_ground = 1;
  }

  if(on_ground)
  {
    rb->v.flags |= PH_Rigidbody3DFlag_OmitGravity;
    rb->v.v.y = 0;
  }
  else
  {
    rb->v.flags &= ~PH_Particle3DFlag_OmitGravity;
  }

  if(rk_key_match(scene->hot_key, node->key) && rk_state->sig.f & UI_SignalFlag_LeftPressed)
  {
    // TODO(XXX): use zfar here?
    Vec3F32 intersect = {0,0, 10000};
    // left,right, top,bottom, front,back in this order
    Vec4F32 normals[6] =
    {
      v4f32(-1,0,0,0),
      v4f32(1,0,0,0),
      v4f32(0,-1,0,0),
      v4f32(0,1,0,0),
      v4f32(0,0,-1,0),
      v4f32(0,0,1,0),
    };
    Vec4F32 points[6] =
    {
      v4f32(-0.5,0,0,1),
      v4f32(0,0.5,0,0),
      v4f32(0,-0.5,0,0),
      v4f32(0,0.5,0,0),
      v4f32(0,0,-0.5,0),
      v4f32(0,0,0.5,0),
    };

    Mat4x4F32 world_m = node->fixed_xform; /* local to world xform */
    for(U64 i = 0; i < 6; i++)
    {
      normals[i] = transform_4x4f32(world_m, normals[i]);
      points[i] = transform_4x4f32(world_m, points[i]);
    }

    // mouse ndc pos
    F32 mox_ndc = (rk_state->cursor.x / rk_state->window_dim.x) * 2.f - 1.f;
    F32 moy_ndc = (rk_state->cursor.y / rk_state->window_dim.y) * 2.f - 1.f;

    // in world space
    Vec4F32 _ray_end = transform_4x4f32(ctx->proj_view_inv_m, v4f32(mox_ndc, moy_ndc, 1.0, 1.0));
    Vec3F32 ray_end = xyz_from_v4f32(_ray_end);

    // TODO(XXX): this produre is not quite right, fix it later
    for(U64 i = 0; i < 6; i++) 
    {
      F32 t = rk_plane_intersect(ctx->eye, ray_end, xyz_from_v4f32(normals[i]), xyz_from_v4f32(points[i]));
      Vec3F32 i = mix_3f32(ctx->eye, ray_end, t);
      if(i.z < intersect.z)
      {
        intersect = i;
      }
    }
    // printf("intersect: %f %f %f\n", intersect.x, intersect.y, intersect.z);

    PH_Force3D *f = push_array(rk_frame_arena(), PH_Force3D, 1);
    f->kind = PH_Force3DKind_Constant;
    f->v.constant.direction = normalize_3f32(sub_3f32(node->fixed_position, intersect));
    f->v.constant.strength = 100;
    // f->contact = v3f32(0,0.3,0);
    f->contact = sub_3f32(intersect, node->fixed_position);
    f->targets.v = &rb->v;
    f->target_count = 1;
    rk_rs_push_force3d(f);
  }

  PH_Rigidbody3D *anchor = push_array(rk_frame_arena(), PH_Rigidbody3D, 1);
  anchor->x = v3f32(0,-3,0);
  anchor->q = make_indentity_quat_f32();
  anchor->mass = 1.0;
  anchor->flags |= PH_Rigidbody3DFlag_Static;
  rk_rs_push_rigidbody3d(anchor);

  PH_Constraint3D *c = push_array(rk_frame_arena(), PH_Constraint3D, 1);
  c->kind = PH_Constraint3DKind_Distance;
  c->v.distance.d = 2.f;
  c->target_count = 2;
  c->targets.a = &rb->v;
  c->targets.b = anchor;
  rk_rs_push_constraint3d(c);

  // visous drag
  PH_Force3D *drag = push_array(rk_frame_arena(), PH_Force3D, 1);
  drag->targets.v = &rb->v;
  drag->target_count = 1;
  drag->kind = PH_Force3DKind_VisousDrag;
  drag->v.visous_drag.kd = 0.5;
  rk_rs_push_force3d(drag);

  // D_BucketScope(rk_state->bucket_geo3d[RK_GeoBucketKind_Back])
  // {
  //     rk_drawlist_push_line(rk_frame_arena(),
  //                           rk_frame_drawlist(),
  //                           rk_key_zero(),
  //                           v3f32(0,-3,0),
  //                           rb->v.x,
  //                           v4f32(1,1,1,1), 30, 0);
  // }
}

// default editor scene
internal RK_Scene *
rk_scene_entry__0()
{
  RK_Scene *ret = rk_scene_alloc(str8_lit("default"), str8_lit("./src/rubik/scenes/0/default.rscn"));

  // some initilization for scene
  {
    /////////////////////////////////////////////////////////////////////////////////
    // physics

    // particle3d
    ret->particle3d_system.gravity = (PH_Force3D_Gravity){
      .g = 9.81f,
        .dir = v3f32(0,1,0),
    };
    ret->particle3d_system.visous_drag = (PH_Force3D_VisousDrag){
      .kd = 0.01f,
    };

    // rigidbody3d
    ret->rigidbody3d_system.gravity = (PH_Force3D_Gravity){
      .g = 9.81f,
        .dir = v3f32(0,1,0),
    };
    ret->rigidbody3d_system.visous_drag = (PH_Force3D_VisousDrag){
      .kd = 0.01f,
    };
  }

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
    // RK_PackedScene *droide = rk_packed_scene_from_gltf(str8_lit("./models/free_droide_de_seguridad_k-2so_by_oscar_creativo/scene.gltf"));
    rk_pop_parent();
    rk_pop_node_bucket();

    RK_Handle white_mat = rk_material_from_color(str8_lit("white"), v4f32(1,1,1,1));
    RK_Handle grid_prototype_material = rk_material_from_image(str8_lit("grid_prototype"), str8_lit("./textures/gridbox-prototype-materials/prototype_512x512_grey2.png"));

    /////////////////////////////////////////////////////////////////////////////////
    // spwan node

    // reference plane
    RK_Node *floor = rk_build_node3d_from_stringf(0,0,"floor");
    rk_node_equip_plane(floor, v2f32(9,9), 0,0);

    floor->node3d->transform.position.y -= 0.01;
    floor->mesh_inst3d->material_override = white_mat;
    // floor->mesh_inst3d->material_override = grid_prototype_material;
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
    for(S64 i = 0; i < 3; i++)
    {
      for(S64 j = 0; j < 3; j++)
      {
        RK_Node *l = rk_build_node3d_from_stringf(RK_NodeTypeFlag_PointLight, 0, "point_light_%I64d_%I64d", i, j);
        rk_node_equip_sphere(l, 0.1, 0.2, 9,9,0);
        l->flags |= RK_NodeFlag_NavigationRoot;
        l->node3d->transform.position = v3f32(-3+j*3, -3.5, -3+i*3);
        l->mesh_inst3d->omit_light = 1;
        l->mesh_inst3d->draw_edge = 1;
        l->point_light->color = v3f32(1,1,1);
        l->point_light->range = 3.9;
        l->point_light->intensity = 0.6;
        l->point_light->attenuation = v3f32(0,0,1);
      }
    }

    // spot lights
    for(U64 i = 0; i < 3; i++)
    {
      RK_Node *l = rk_build_node3d_from_stringf(RK_NodeTypeFlag_SpotLight, 0, "spot_light_%I64d", i);
      rk_node_equip_box(l, v3f32(0.15, 0.15, 0.15), 0,0,0);
      l->flags |= RK_NodeFlag_NavigationRoot;
      l->node3d->transform.position.y -= 3;
      l->node3d->transform.position.x = -3 + (S64)i*3;
      l->mesh_inst3d->omit_light = 1;
      l->mesh_inst3d->draw_edge = 1;
      l->spot_light->color.v[i] = 1;
      l->spot_light->intensity = 3;
      l->spot_light->attenuation = v3f32(0,0,1);
      l->spot_light->direction = normalize_3f32(v3f32(1,1,0));
      l->spot_light->range = 9.9;
      l->spot_light->angle = radians_from_turns_f32(0.09);
      rk_node_push_fn(l, rotating_spot_light);

      AnimatedSpotLight *data = rk_node_push_custom_data(l, AnimatedSpotLight);
      data->rotation_axis.v[i] = 1;
    }

    RK_Node *n2 = rk_node_from_packed_scene(str8_lit("2"), rk_packed_from_handle(blackguard));
    {
      // flip y
      n2->node3d->transform.rotation = mul_quat_f32(make_rotate_quat_f32(v3f32(1,0,0), 0.5f), n2->node3d->transform.rotation);
      n2->node3d->transform.position = v3f32(0,0,0);
      n2->flags |= RK_NodeFlag_NavigationRoot;
    }

    RK_Node *n3 = rk_node_from_packed_scene(str8_lit("3"), rk_packed_from_handle(dancing_stormtrooper));
    {
      // flip y
      n3->node3d->transform.rotation = mul_quat_f32(make_rotate_quat_f32(v3f32(1,0,0), 0.5f), n3->node3d->transform.rotation);
      n3->node3d->transform.position = v3f32(3,0,0);
      n3->flags |= RK_NodeFlag_NavigationRoot;
    }

    // TODO(XXX): testing particle physics
    {
      // RK_Node *a = rk_build_node_from_stringf(RK_NodeTypeFlag_Node3D|RK_NodeTypeFlag_Particle3D, RK_NodeFlag_NavigationRoot, "particle_1");
      // rk_node_equip_box(a, v3f32(0.3,0.3,0.3), 0,0,0);
      // a->particle3d->v.x = v3f32(-1,0, -1);
      // a->particle3d->v.m = 1;
      // rk_node_push_fn(a, spring_knot);

      // RK_Node *b = rk_build_node_from_stringf(RK_NodeTypeFlag_Node3D|RK_NodeTypeFlag_Particle3D, RK_NodeFlag_NavigationRoot, "particle_2");
      // rk_node_equip_box(b, v3f32(0.3,0.3,0.3), 0,0,0);
      // b->particle3d->v.m = 1;
      // b->particle3d->v.x = v3f32(1,0, -1);
      // rk_node_push_fn(b, spring_knot);

      // RK_Node *c = rk_build_node_from_stringf(RK_NodeTypeFlag_Node3D|RK_NodeTypeFlag_Particle3D, RK_NodeFlag_NavigationRoot, "particle_3");
      // rk_node_equip_box(c, v3f32(0.3,0.3,0.3), 0,0,0);
      // c->particle3d->v.m = 1;
      // c->particle3d->v.x = v3f32(-1,0, -3);
      // rk_node_push_fn(c, spring_knot);

      // // spring1
      // RK_Node *spring1 = rk_build_node_from_stringf(RK_NodeTypeFlag_Node3D|RK_NodeTypeFlag_HookSpring3D, RK_NodeFlag_NavigationRoot, "spring_1");
      // rk_node_equip_box(spring1, v3f32(0.1,2,0.1), 0,0,0);
      // spring1->hook_spring3d->a = rk_handle_from_node(a);
      // spring1->hook_spring3d->b = rk_handle_from_node(c);
      // spring1->hook_spring3d->ks = 39.f;
      // spring1->hook_spring3d->kd = 12.f;
      // spring1->hook_spring3d->rest = 2.f;

      // spring2
      // RK_Node *spring2 = rk_build_node_from_stringf(RK_NodeTypeFlag_Node3D|RK_NodeTypeFlag_HookSpring3D, RK_NodeFlag_NavigationRoot, "spring_2");
      // rk_node_equip_box(spring2, v3f32(0.1,2,0.1), 0,0,0);
      // spring2->hook_spring3d->a = rk_handle_from_node(b);
      // spring2->hook_spring3d->b = rk_handle_from_node(c);
      // spring2->hook_spring3d->ks = 39.f;
      // spring2->hook_spring3d->kd = 12.f;
      // spring2->hook_spring3d->rest = 2.f;

      // constraint
      // RK_Node *constraint = rk_build_node_from_stringf(RK_NodeTypeFlag_Constraint3D, 0, "constraint_1");
      // constraint->constraint3d->d = 2.f;
      // constraint->constraint3d->kind = PH_Constraint3DKind_Distance;
      // constraint->constraint3d->target_count = 2;
      // constraint->constraint3d->targets.a = rk_handle_from_node(a);
      // constraint->constraint3d->targets.b = rk_handle_from_node(b);

      RK_Node *a = rk_build_node_from_stringf(RK_NodeTypeFlag_Node3D|RK_NodeTypeFlag_Rigidbody3D, RK_NodeFlag_NavigationRoot, "b_1");
      rk_node_equip_box(a, v3f32(1,1,1), 0,0,0);
      a->rigidbody3d->v.x = v3f32(0,-1, 0);
      a->rigidbody3d->v.q = make_indentity_quat_f32();
      a->rigidbody3d->v.mass = 1;
      a->rigidbody3d->v.shape = PH_Rigidbody3DShapeKind_Cuboid;
      a->rigidbody3d->v.dim.v3f32 = v3f32(1,1,1);
      a->rigidbody3d->v.last_dim.v3f32 = v3f32(1,1,1);
      a->rigidbody3d->v.Ibody = ph_inertia_from_cuboid(1, v3f32(1,1,1));
      a->rigidbody3d->v.Ibodyinv = ph_inertiainv_from_cuboid(1, v3f32(1,1,1));
      // a->rigidbody3d->v.L = v3f32(0.1, 0,0);
      // a->rigidbody3d->v.P = v3f32(0.3, 0.1,0);
      rk_node_push_fn(a, body);
    }
  }
  ret->root = rk_handle_from_node(root);
  rk_pop_node_bucket();
  rk_pop_res_bucket();
  return ret;
}
