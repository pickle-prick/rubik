//////////////////////////////
// Particle/System Functions

internal PH_Vector
ph_state_from_ps3d(Arena *arena, PH_Particle3DSystem *system)
{
  // Phase space (position + velocity)
  PH_Vector ret = ph_vec_from_dim(arena, system->particle_count*6);

  F32 *dst = ret.v;
  for(PH_Particle3D *p = system->first_particle; p != 0; p = p->next)
  {
    // x
    *(dst++) = p->x.v[0];
    *(dst++) = p->x.v[1];
    *(dst++) = p->x.v[2];

    // xdot
    *(dst++) = p->v.v[0];
    *(dst++) = p->v.v[1];
    *(dst++) = p->v.v[2];
  }
  return ret;
}

internal void
ph_ps3d_state_set(PH_Particle3DSystem *ps, PH_Vector state)
{
  F32 *src = state.v;
  for(PH_Particle3D *p = ps->first_particle; p != 0; p = p->next)
  {
    p->x.v[0] = *(src++);
    p->x.v[1] = *(src++);
    p->x.v[2] = *(src++);
    p->v.v[0] = *(src++);
    p->v.v[1] = *(src++);
    p->v.v[2] = *(src++);
  }
}

internal PH_Vector
ph_dxdt_ps3d(Arena *arena, PH_Vector X, F32 dt, void *_system)
{
  Temp scratch = scratch_begin(&arena, 1);
  PH_Particle3DSystem *system = _system;

  // TODO(XXX): some ode will generate X multiple times, some won't, maybe we should pass that information
  ph_ps3d_state_set(system, X);

  // xdot + vdot (vel + acc)
  Assert(X.dim == system->particle_count*6);
  PH_Vector ret = ph_vec_from_dim(arena, X.dim);

  // zero the force accumulators
  for(PH_Particle3D *p = system->first_particle; p != 0; p = p->next)
  {
    MemoryZeroStruct(&p->f);
  }

  /////////////////////////////////////////////////////////////////////////////////////
  //~ compute forces (applied force + constrained force)

  // global forces
  for(PH_Particle3D *p = system->first_particle; p != 0; p = p->next)
  {
    // gravity
    Vec3F32 f1 = {0};
    if(!(p->flags & PH_Particle3DFlag_OmitGravity))
    {
      f1 = scale_3f32(system->gravity.dir, system->gravity.g*p->m);
    }
    p->f = add_3f32(p->f, f1);

    // drag
    Vec3F32 f2 = scale_3f32(p->v, system->visous_drag.kd*-1);
    p->f = add_3f32(p->f, f2);

    // individual drag
    Vec3F32 f3 = scale_3f32(p->v, p->visous_drag.kd*-1);
    p->f = add_3f32(p->f, f3);
  }

  // unconstraint forces
  for(PH_Force3D *force = system->first_force; force != 0; force = force->next)
  {
    switch(force->kind)
    {
      case PH_Force3DKind_HookSpring:
        {
          F32 ks = force->v.hook_spring.ks;
          F32 kd = force->v.hook_spring.kd;
          F32 rest = force->v.hook_spring.rest;

          Assert(force->target_count == 2);
          PH_Particle3D *a = force->targets.a;
          PH_Particle3D *b = force->targets.b;

          Vec3F32 l = sub_3f32(a->x, b->x);
          F32 l_length = length_3f32(l);
          Vec3F32 lnorm = normalize_3f32(l);
          // ldot = va-va
          Vec3F32 ldot = sub_3f32(a->v, b->v);
          F32 factor = -(ks * (l_length-rest) + kd * (dot_3f32(ldot,l) / l_length));
          Vec3F32 f = scale_3f32(lnorm, factor);
          a->f = add_3f32(a->f, f);
          b->f = add_3f32(b->f, negate_3f32(f));
        }break;
        // case PH_Force3DKind_Const:
        // {
        //     PH_Particle3D *t = force->targets.v;
        //     Vec3F32 F = scale_3f32(force->v.constf.direction, force->v.constf.strength);
        //     t->f = add_3f32(t->f, F);
        // }break;
      default:{InvalidPath;}break;
    }
  }

  // constraint forces
  if(system->constraint_count > 0)
  {
    /////////////////////////////////////////////////////////////////////////////////
    // Collect components

    U64 m = system->constraint_count;
    U64 n = system->particle_count;
    U64 N = n*3;

    // force vector [3n, 1]
    PH_Vector Q = ph_vec_from_dim(scratch.arena, N);
    F32 *Q_dst = Q.v;

    // velocity vector [3n, 1]
    // Unless phase space, q only contains positions, qdot only contains velocities
    PH_Vector qdot = ph_vec_from_dim(scratch.arena, N);
    F32 *qdot_dst = qdot.v;
    // TODO(XXX): we don't need q, just for testing, delete it later
    // PH_Vector q = ph_vec_from_dim(scratch.arena, N);
    // F32 *q_dst = q.v;

    // mass matrix [3n, 3n]
    // TODO(XXX): if we were going to use cg, mass matrix could be stored as vector for easier computation
    // W is just the reciprocal of M
    PH_Matrix W = ph_mat_from_dim(scratch.arena, N,N);

    // loop through particles to collect above values
    for(PH_Particle3D *p = system->first_particle; p != 0; p = p->next)
    {
      // Q
      *(Q_dst++) = p->f.v[0];
      *(Q_dst++) = p->f.v[1];
      *(Q_dst++) = p->f.v[2];

      // qdot
      *(qdot_dst++) = p->v.v[0];
      *(qdot_dst++) = p->v.v[1];
      *(qdot_dst++) = p->v.v[2];

      // q
      // *(q_dst++) = p->x.v[0];
      // *(q_dst++) = p->x.v[1];
      // *(q_dst++) = p->x.v[2];

      // W
      U64 i = p->idx*3;
      W.v[i+0][i+0] = 1.0/p->m;
      W.v[i+1][i+1] = 1.0/p->m;
      W.v[i+2][i+2] = 1.0/p->m;
    }

    // jacobian of C(q) [m, 3n]
    // collect J & Jt
    PH_Matrix J = ph_mat_from_dim(scratch.arena, m, N);
    PH_Matrix Jdot = ph_mat_from_dim(scratch.arena, m, N);
    for(PH_Constraint3D *c = system->first_constraint; c != 0; c = c->next)
    {
      switch(c->kind)
      {
        case PH_Constraint3DKind_Distance:
          {
            PH_Particle3D *a = c->targets.a;
            PH_Particle3D *b = c->targets.b;

            U64 i,j;

            // J & Jdot
            i = c->idx;
            j = a->idx*3;
            for(U64 k = 0; k < 3; k++)
            {
              U64 jj = j+k;
              J.v[i][jj] = a->x.v[k] - b->x.v[k];
              Jdot.v[i][jj] = a->v.v[k] - b->v.v[k];
            }

            j = b->idx*3;
            for(U64 k = 0; k < 3; k++)
            {
              U64 jj = j+k;
              J.v[i][jj] = -(a->x.v[k] - b->x.v[k]);
              Jdot.v[i][jj] = -(a->v.v[k] - b->v.v[k]);
            }
          }break;
        default: {InvalidPath;}break;
      }
    }
    // transpose of J
    PH_Matrix Jt = ph_trp_mat(scratch.arena, J);

    // C(q) Cdot(q)
    PH_Vector C_q = ph_vec_from_dim(scratch.arena, m);
    Assert(C_q.dim == m);
    // TODO(XXX): implement this
    PH_Vector Cdot_q = ph_mul_mv(scratch.arena, J, qdot);
    Assert(Cdot_q.dim == C_q.dim);
    for(PH_Constraint3D *c = system->first_constraint; c != 0; c = c->next)
    {
      switch(c->kind)
      {
        case PH_Constraint3DKind_Distance:
          {
            PH_Particle3D *a = c->targets.a;
            PH_Particle3D *b = c->targets.b;
            C_q.v[c->idx] = 0.5 * (pow_f32(a->x.x-b->x.x,2) + pow_f32(a->x.y-b->x.y,2) + pow_f32(a->x.z-b->x.z,2) - pow_f32(c->v.distance.d,2));
          }break;
        default: {InvalidPath;}break;
      }
    }

    /////////////////////////////////////////////////////////////////////////////////
    //~ Compute

    //- solve Lagrange multipliers
    // [m, 3n] [3n, 1] => [m, 1]
    PH_Vector Jdot_qdot = ph_mul_mv(scratch.arena, Jdot, qdot); /* Jdot * qdot */
    // [3n, 1]
    PH_Vector WQ = ph_mul_mv(scratch.arena, W, Q);
    // [m, 3n] [3n, 1] => [m, 1]
    PH_Vector JWQ = ph_mul_mv(scratch.arena, J, WQ);
    PH_Vector b = ph_add_vec(scratch.arena, Jdot_qdot, JWQ);
    b = ph_add_vec(scratch.arena, b, ph_scale_vec(scratch.arena,C_q, 1000)); // ks * C
    b = ph_add_vec(scratch.arena, b, ph_scale_vec(scratch.arena,Cdot_q, 64)); // kd * Cdot
    b = ph_negate_vec(scratch.arena, b);

    //- solve the linear system
    // [m, 1]
    PH_Matrix A = ph_mul_mm(scratch.arena, J, W);
    A = ph_mul_mm(scratch.arena, A, Jt);
    PH_Vector lambda = ph_vec_copy(scratch.arena, b);
    Assert(lambda.dim == m);
    Assert(A.i_dim == A.j_dim);
    gaussj2(A.v, A.i_dim, lambda.v);

    rk_debug_gfx(19, v2f32(300, 200), v4f32(1,1,0,1), push_str8f(scratch.arena, "lambda: %f", lambda.v[0]));
    // constraint forces
    // [3n, m] [m, 1] => [3n, 1]
    PH_Vector Q_c = ph_mul_mv(scratch.arena, Jt, lambda);
    Assert(Q_c.dim == N);
    rk_debug_gfx(19, v2f32(300, 600), v4f32(1,1,0,1), push_str8f(scratch.arena, "%f %f %f %f %f %f", Q_c.v[0], Q_c.v[1], Q_c.v[2], Q_c.v[3], Q_c.v[4], Q_c.v[5]));
    //- add constraint force to particles
    F32 *src = Q_c.v;
    for(PH_Particle3D *p = system->first_particle; p != 0; p = p->next)
    {
      p->f.v[0] += *(src++);
      p->f.v[1] += *(src++);
      p->f.v[2] += *(src++);
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////
  //~ copy result

  F32 *dst = ret.v;
  for(PH_Particle3D *p = system->first_particle; p != 0; p = p->next)
  {
    *(dst++) = p->v.v[0]; /* xdot = v */
    *(dst++) = p->v.v[1];
    *(dst++) = p->v.v[2];

    *(dst++) = p->f.v[0] / p->m; /* vdot = f/m */
    *(dst++) = p->f.v[1] / p->m;
    *(dst++) = p->f.v[2] / p->m;
  }
  scratch_end(scratch);
  return ret;
}

//////////////////////////////
// Rigidbody3D Functions

internal void
ph_state_from_rb3d(PH_Rigidbody3D *rb, F32 *dst)
{
  U64 i,j;
  // x (position)
  *(dst++) = rb->x.v[0];
  *(dst++) = rb->x.v[1];
  *(dst++) = rb->x.v[2];

  // q (rotation)
  *(dst++) = rb->q.x;
  *(dst++) = rb->q.y;
  *(dst++) = rb->q.z;
  *(dst++) = rb->q.w;

  // P
  *(dst++) = rb->P.v[0];
  *(dst++) = rb->P.v[1];
  *(dst++) = rb->P.v[2];

  // L
  *(dst++) = rb->L.v[0];
  *(dst++) = rb->L.v[1];
  *(dst++) = rb->L.v[2];
}

internal void
ph_rb3d_state_set(PH_Rigidbody3D *b, F32 *src)
{
  // x (position)
  b->x.v[0] = *(src++);
  b->x.v[1] = *(src++);
  b->x.v[2] = *(src++);

  // q (rotation)
  // Assert(length_4f32(b->q) != 0);
  b->q.v[0] = *(src++);
  b->q.v[1] = *(src++);
  b->q.v[2] = *(src++);
  b->q.v[3] = *(src++);
  b->q = normalize_4f32(b->q);
  // Assert(length_4f32(b->q) != 0);

  // P
  b->P.v[0] = *(src++);
  b->P.v[1] = *(src++);
  b->P.v[2] = *(src++);

  // L
  b->L.v[0] = *(src++);
  b->L.v[1] = *(src++);
  b->L.v[2] = *(src++);

  // compute auxiliary variables ...

  // R (rotation matrix)
  b->R = mat3x3f32_from_quat_rmajor(b->q);

  // Iinv
  b->Iinv = mul_3x3f32_rmajor(b->R, mul_3x3f32_rmajor(b->Ibodyinv, transpose_3x3f32(b->R)));

  // v(t)
  b->v = scale_3f32(b->P, 1.0/b->mass);

  // ω(t) = I−1(t)L(t)
  b->omega = transform_3x3f32_rmajor(b->Iinv, b->L);
}

// system
internal PH_Vector
ph_state_from_rs3d(Arena *arena, PH_Rigidbody3DSystem *system)
{
  PH_Vector ret = ph_vec_from_dim(arena, PH_RB3D_STATE_DIM*system->body_count);
  U64 stride = PH_RB3D_STATE_DIM;
  F32 *dst = ret.v;
  for(PH_Rigidbody3D *b = system->first_body; b != 0; b = b->next)
  {
    ph_state_from_rb3d(b, dst);
    dst += stride;
  }
  return ret;
}

internal void
ph_rs3d_state_set(PH_Rigidbody3DSystem *system, PH_Vector state)
{
  Assert(state.dim == PH_RB3D_STATE_DIM*system->body_count);
  U64 stride = PH_RB3D_STATE_DIM;
  F32 *src = state.v;
  for(PH_Rigidbody3D *b = system->first_body; b != 0; b = b->next)
  {
    ph_rb3d_state_set(b, src);
    src += stride;
  }
}

internal PH_Vector
ph_dxdt_rs3d(Arena *arena, PH_Vector X, F32 t, void *_system)
{
  Temp scratch = scratch_begin(&arena, 1);
  PH_Rigidbody3DSystem *system = _system;
  PH_Vector ret = ph_vec_from_dim(arena, X.dim);

  // TODO(XXX): some ode will generate X multiple times, some won't, we should pass that information
  ph_rs3d_state_set(system, X);

  // zero out the force accumulators
  for(PH_Rigidbody3D *rb = system->first_body; rb != 0; rb = rb->next)
  {
    rb->force = v3f32(0,0,0);
    rb->torque = v3f32(0,0,0);
  }

  /////////////////////////////////////////////////////////////////////////////////////
  //~ compute forces (applied force/torque + constrained force)

  /////////////////////////////////////////////////////////////////////////////////////
  //- global forces (uniform force field)
  for(PH_Rigidbody3D* rb = system->first_body; rb != 0; rb = rb->next)
  {
    // don't apply force if body is static 
    if(rb->flags & PH_Rigidbody3DFlag_Static) continue;

    // gravity
    Vec3F32 f1 = {0};
    if(!(rb->flags & PH_Rigidbody3DFlag_OmitGravity))
    {
      f1 = scale_3f32(system->gravity.dir, system->gravity.g*rb->mass);
    }
    rb->force = add_3f32(rb->force, f1);

    // global drag (add little bit to reduce numerical drift)
    Vec3F32 f2 = scale_3f32(rb->v, system->visous_drag.kd*-1);
    rb->force = add_3f32(rb->force, f2);
  }

  /////////////////////////////////////////////////////////////////////////////////////
  //- unconstraint force/torque (contact position dependent)

  for(PH_Force3D *f = system->first_force; f != 0; f = f->next)
  {
    switch(f->kind)
    {
      case PH_Force3DKind_Constant:
        {
          Assert(f->target_count == 1);
          PH_Rigidbody3D *rb = f->targets.v;

          Vec3F32 F = scale_3f32(f->v.constant.direction, f->v.constant.strength);

          // linear
          rb->force = add_3f32(rb->force, F);

          // torque
          Vec3F32 torque = cross_3f32(f->contact, F);
          rb->torque = add_3f32(rb->torque, torque);
        }break;
      case PH_Force3DKind_VisousDrag:
        {
          // drag (air, water, fog, e.g.)
          Assert(f->target_count == 1);
          PH_Rigidbody3D *rb = f->targets.v;
          Vec3F32 F = scale_3f32(rb->v, f->v.visous_drag.kd*-1);
          rb->force = add_3f32(rb->force, F);

          // TODO(XXX): apply torque
        }break;
      default:{InvalidPath;}break;
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////
  //- constrant force/torque (contact position dependent)
  if(system->constraint_count > 0)
  {
    /////////////////////////////////////////////////////////////////////////////////
    // Collect components

    U64 m = system->constraint_count;
    U64 n = system->body_count;
    U64 N = n*3; // only position

    // force vector [3n, 1]
    PH_Vector Q = ph_vec_from_dim(scratch.arena, N);
    F32 *Q_dst = Q.v;

    // velocity vector [3n, 1]
    // Unless phase space, q only contains positions, qdot only contains velocities
    PH_Vector qdot = ph_vec_from_dim(scratch.arena, N);
    F32 *qdot_dst = qdot.v;
    // TODO(XXX): we don't need q, just for testing, delete it later
    // PH_Vector q = ph_vec_from_dim(scratch.arena, N);
    // F32 *q_dst = q.v;

    // mass matrix [3n, 3n]
    // TODO(XXX): if we were going to use cg, mass matrix could be stored as vector for easier computation
    // W is just the reciprocal of M
    PH_Matrix W = ph_mat_from_dim(scratch.arena, N,N);

    // loop through all bodies to collect above values
    for(PH_Rigidbody3D *rb = system->first_body; rb != 0; rb = rb->next)
    {
      // Q
      *(Q_dst++) = rb->force.v[0];
      *(Q_dst++) = rb->force.v[1];
      *(Q_dst++) = rb->force.v[2];

      // qdot
      *(qdot_dst++) = rb->v.v[0];
      *(qdot_dst++) = rb->v.v[1];
      *(qdot_dst++) = rb->v.v[2];

      // q
      // *(q_dst++) = rb->x.v[0];
      // *(q_dst++) = rb->x.v[1];
      // *(q_dst++) = rb->x.v[2];

      // W
      U64 i = rb->idx*3;
      W.v[i+0][i+0] = 1.0/rb->mass;
      W.v[i+1][i+1] = 1.0/rb->mass;
      W.v[i+2][i+2] = 1.0/rb->mass;
    }

    // jacobian of C(q) [m, 3n]
    // collect J & Jt
    PH_Matrix J = ph_mat_from_dim(scratch.arena, m, N);
    PH_Matrix Jdot = ph_mat_from_dim(scratch.arena, m, N);
    for(PH_Constraint3D *c = system->first_constraint; c != 0; c = c->next)
    {
      switch(c->kind)
      {
        case PH_Constraint3DKind_Distance:
          {
            PH_Rigidbody3D *a = c->targets.a;
            PH_Rigidbody3D *b = c->targets.b;
            Assert(a&&b);

            U64 i,j;

            // J & Jdot
            i = c->idx;
            j = a->idx*3;
            for(U64 k = 0; k < 3; k++)
            {
              U64 jj = j+k;
              J.v[i][jj] = a->x.v[k] - b->x.v[k];
              Jdot.v[i][jj] = a->v.v[k] - b->v.v[k];
            }

            j = b->idx*3;
            for(U64 k = 0; k < 3; k++)
            {
              U64 jj = j+k;
              J.v[i][jj] = -(a->x.v[k] - b->x.v[k]);
              Jdot.v[i][jj] = -(a->v.v[k] - b->v.v[k]);
            }
          }break;
        default: {InvalidPath;}break;
      }
    }
    // transpose of J
    PH_Matrix Jt = ph_trp_mat(scratch.arena, J);

    // C(q) Cdot(q)
    PH_Vector C_q = ph_vec_from_dim(scratch.arena, m);
    PH_Vector Cdot_q = ph_mul_mv(scratch.arena, J, qdot);
    Assert(Cdot_q.dim == C_q.dim);
    for(PH_Constraint3D *c = system->first_constraint; c != 0; c = c->next)
    {
      switch(c->kind)
      {
        case PH_Constraint3DKind_Distance:
          {
            PH_Rigidbody3D *a = c->targets.a;
            PH_Rigidbody3D *b = c->targets.b;
            C_q.v[c->idx] = 0.5 * (pow_f32(a->x.x-b->x.x,2) + pow_f32(a->x.y-b->x.y,2) + pow_f32(a->x.z-b->x.z,2) - pow_f32(c->v.distance.d,2));
          }break;
        default: {InvalidPath;}break;
      }
    }
    rk_debug_gfx(19, v2f32(300, 100), v4f32(1,1,0,1), push_str8f(scratch.arena, "C_q: %f", C_q.v[0]));

    /////////////////////////////////////////////////////////////////////////////////
    //~ Compute

    //- solve Lagrange multipliers
    // [m, 3n] [3n, 1] => [m, 1]
    PH_Vector Jdot_qdot = ph_mul_mv(scratch.arena, Jdot, qdot); /* Jdot * qdot */
    // [3n, 1]
    PH_Vector WQ = ph_mul_mv(scratch.arena, W, Q);
    // [m, 3n] [3n, 1] => [m, 1]
    PH_Vector JWQ = ph_mul_mv(scratch.arena, J, WQ);
    PH_Vector b = ph_add_vec(scratch.arena, Jdot_qdot, JWQ);
    b = ph_add_vec(scratch.arena, b, ph_scale_vec(scratch.arena,C_q, 1000)); // ks * C
    b = ph_add_vec(scratch.arena, b, ph_scale_vec(scratch.arena,Cdot_q, 64)); // kd * Cdot
    b = ph_negate_vec(scratch.arena, b);

    //- solve the linear system
    // [m, 1]
    PH_Matrix A = ph_mul_mm(scratch.arena, J, W);
    A = ph_mul_mm(scratch.arena, A, Jt);
    PH_Vector lambda = ph_vec_copy(scratch.arena, b);
    Assert(lambda.dim == m);
    Assert(A.i_dim == A.j_dim);
    gaussj2(A.v, A.i_dim, lambda.v);

    rk_debug_gfx(19, v2f32(300, 200), v4f32(1,1,0,1), push_str8f(scratch.arena, "lambda: %f", lambda.v[0]));
    // constraint forces
    // [3n, m] [m, 1] => [3n, 1]
    PH_Vector Q_c = ph_mul_mv(scratch.arena, Jt, lambda);
    Assert(Q_c.dim == N);
    rk_debug_gfx(19, v2f32(300, 600), v4f32(1,1,0,1), push_str8f(scratch.arena, "%f %f %f %f %f %f", Q_c.v[0], Q_c.v[1], Q_c.v[2], Q_c.v[3], Q_c.v[4], Q_c.v[5]));
    //- add constraint force to rb
    F32 *src = Q_c.v;
    for(PH_Rigidbody3D *rb = system->first_body; rb != 0; rb = rb->next)
    {
      if(!(rb->flags&PH_Rigidbody3DFlag_Static))
      {
        // TODO(XXX): apply force/torque
        rb->force.v[0] += *(src++);
        rb->force.v[1] += *(src++);
        rb->force.v[2] += *(src++);
      }
      else
      {
        src+=3;
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////
  //~ gather result

  F32 *dst = ret.v;
  for(PH_Rigidbody3D *rb = system->first_body; rb != 0; rb = rb->next)
  {
    // v(t) (linear velocity)
    *(dst++) = rb->v.v[0];
    *(dst++) = rb->v.v[1];
    *(dst++) = rb->v.v[2];

    // qdot
    QuatF32 qdot = mul_quat_f32((QuatF32){rb->omega.x, rb->omega.y, rb->omega.z, 0}, rb->q);
    qdot = scale_4f32(qdot, .5);
    *(dst++) = qdot.x;
    *(dst++) = qdot.y;
    *(dst++) = qdot.z;
    *(dst++) = qdot.w;

    // Pdot(t) = F(t)
    *(dst++) = rb->force.v[0];
    *(dst++) = rb->force.v[1];
    *(dst++) = rb->force.v[2];

    // Ldot(t) = τ(t)
    *(dst++) = rb->torque.v[0];
    *(dst++) = rb->torque.v[1];
    *(dst++) = rb->torque.v[2];
  }

  scratch_end(scratch);
  return ret;
}

internal Mat3x3F32
ph_inertia_from_cuboid(F32 M, Vec3F32 dim)
{
  Mat3x3F32 ret = {0};

  F32 x0 = dim.x;
  F32 y0 = dim.y;
  F32 z0 = dim.z;
  ret.v[0][0] = (y0*y0+z0*z0) * (M/12.0);
  ret.v[1][1] = (x0*x0+z0*z0) * (M/12.0);
  ret.v[2][2] = (x0*x0+y0*y0) * (M/12.0);
  return ret;
}

internal Mat3x3F32
ph_inertiainv_from_cuboid(F32 M, Vec3F32 dim)
{
  Mat3x3F32 ret = {0};

  F32 x0 = dim.x;
  F32 y0 = dim.y;
  F32 z0 = dim.z;
  ret.v[0][0] = 12.0 / ((y0*y0+z0*z0)*M);
  ret.v[1][1] = 12.0 / ((x0*x0+z0*z0)*M);
  ret.v[2][2] = 12.0 / ((x0*x0+y0*y0)*M);
  return ret;
}

internal PH_SparseMatrix
ph_sparsed_m_from_blocks(Arena *arena, PH_SparseBlock *blocks, U64 i_dim, U64 j_dim)
{
  PH_SparseMatrix ret = {0};
  ret.row_heads = push_array(arena, PH_SparseBlock*, j_dim);
  ret.row_tails = push_array(arena, PH_SparseBlock*, j_dim);
  ret.col_heads = push_array(arena, PH_SparseBlock*, i_dim);
  ret.col_tails = push_array(arena, PH_SparseBlock*, i_dim);

  U64 count = 0;
  // loop rows
  for(U64 j = 0; j < j_dim; j++)
  {
    PH_SparseBlock **row_head = &ret.row_heads[j];
    PH_SparseBlock **row_tail = &ret.row_tails[j];

    // loop cols
    for(U64 i = 0; i < i_dim; i++)
    {
      PH_SparseBlock **col_head = &ret.col_heads[i];
      PH_SparseBlock **col_tail = &ret.col_tails[i];

      PH_SparseBlock *b = &blocks[j*i_dim + i];
      if(b->v != 0)
      {
        count++;
        SLLQueuePush_N(*row_head, *row_tail, b, row_next);
        SLLQueuePush_N(*col_head, *col_tail, b, col_next);
      }
    }
  }

  ret.i_dim = i_dim;
  ret.j_dim = j_dim;
  ret.count = count;
  return ret;
}

//////////////////////////////
// Aribitrary-length Vector Building

internal PH_Matrix
ph_mat_from_dim(Arena *arena, U64 i_dim, U64 j_dim)
{
  PH_Matrix ret = {0};
  ret.v = push_array(arena, F32*, i_dim);
  for(U64 i = 0; i < i_dim; i++)
  {
    ret.v[i] = push_array(arena, F32, j_dim);
  }
  ret.i_dim = i_dim;
  ret.j_dim = j_dim;
  return ret;
}

internal PH_Vector
ph_vec_from_dim(Arena *arena, U64 dim)
{
  PH_Vector ret = {.dim = dim, .v = push_array(arena, F32, dim)};
  return ret;
}

internal PH_Vector
ph_vec_copy(Arena *arena, PH_Vector src)
{
  PH_Vector ret = ph_vec_from_dim(arena, src.dim);
  MemoryCopy(ret.v, src.v, sizeof(F32)*src.dim);
  return ret;
}

//////////////////////////////
// Aribitrary-length Vector Math Operations

internal PH_Vector ph_add_vec(Arena *arena, PH_Vector a, PH_Vector b)
{
  // TODO(XXX): good place to use SIMD
  Assert(a.dim==b.dim);
  PH_Vector ret = ph_vec_from_dim(arena, a.dim);
  for(U64 i = 0; i < a.dim; i++)
  {
    ret.v[i] = a.v[i]+b.v[i];
  }
  return ret;
}

internal PH_Vector
ph_sub_vec(Arena *arena, PH_Vector a, PH_Vector b)
{
  // TODO(XXX): good place to use SIMD
  Assert(a.dim==b.dim);
  PH_Vector ret = ph_vec_from_dim(arena, a.dim);
  for(U64 i = 0; i < a.dim; i++)
  {
    ret.v[i] = a.v[i]-b.v[i];
  }
  return ret;
}

internal F32
ph_dot_vec(PH_Vector a, PH_Vector b)
{
  F32 ret = 0;

  // TODO(XXX): good place to use SIMD
  Assert(a.dim==b.dim);
  for(U64 i = 0; i < a.dim; i++)
  {
    ret += a.v[i]*b.v[i];
  }
  return ret;
}

internal PH_Vector
ph_scale_vec(Arena *arena, PH_Vector v, F32 s)
{
  // TODO(XXX): good place to use SIMD
  PH_Vector ret = ph_vec_from_dim(arena, v.dim);
  for(U64 i = 0; i < v.dim; i++)
  {
    ret.v[i] = v.v[i]*s;
  }
  return ret;
}

internal PH_Vector
ph_negate_vec(Arena *arena, PH_Vector v)
{
  // TODO(XXX): good place to use SIMD
  PH_Vector ret = ph_vec_from_dim(arena, v.dim);
  for(U64 i = 0; i < v.dim; i++)
  {
    ret.v[i] = -v.v[i];
  }
  return ret;
}

internal PH_Vector
ph_eemul_vec(Arena *arena, PH_Vector a, PH_Vector b)
{
  // TODO(XXX): good place to use SIMD
  Assert(a.dim == b.dim);
  PH_Vector ret = ph_vec_from_dim(arena, a.dim);
  for(U64 i = 0; i < a.dim; i++)
  {
    ret.v[i] = a.v[i]*b.v[i];
  }
  return ret;
}

internal F32
ph_length_vec(PH_Vector v)
{
  // TODO(XXX): good place to use SIMD
  F32 ret = 0;
  for(U64 i = 0; i < v.dim; i++)
  {
    ret += pow_f32(v.v[i], 2);
  }
  ret = sqrt_f32(ret);
  return ret;
}

//////////////////////////////
// matrix (row major)

internal PH_Matrix
ph_mul_mm(Arena *arena, PH_Matrix A, PH_Matrix B)
{
  Assert(A.j_dim == B.i_dim);
  PH_Matrix ret = ph_mat_from_dim(arena, A.i_dim, B.j_dim);

  U64 i,j,k;
  for(i = 0; i < A.i_dim; i++)
  {
    for(j = 0; j < B.j_dim; j++)
    {
      U64 n = A.j_dim;
      // dot product
      F32 acc = 0;
      for(k = 0; k < n; k++)
      {
        acc += A.v[i][k] * B.v[k][j];
      }
      ret.v[i][j] = acc;
    }
  }
  return ret;
}

internal PH_Vector
ph_mul_mv(Arena *arena, PH_Matrix A, PH_Vector v)
{
  Assert(A.j_dim == v.dim);
  PH_Vector ret = ph_vec_from_dim(arena, A.i_dim);

  U64 i,j;
  for(i = 0; i < A.i_dim; i++)
  {
    F32 acc = 0;
    for(j = 0; j < v.dim; j++)
    {
      acc += A.v[i][j] * v.v[j];
    }
    ret.v[i] = acc;
  }
  return ret;
}

internal PH_Matrix
ph_trp_mat(Arena *arena, PH_Matrix A)
{
  PH_Matrix ret = ph_mat_from_dim(arena, A.j_dim, A.i_dim);

  U64 i,j;
  for(i = 0; i < A.j_dim; i++)
  {
    for(j = 0; j < A.i_dim; j++)
    {
      ret.v[i][j] = A.v[j][i];
    }
  }
  return ret;
}

//////////////////////////////
// Sparse Matrix Computation

// internal PH_Vector
// ph_mul_sm_vec(Arena *arena, PH_SparseMatrix *m, PH_Vector v)
// {
//     // TODO(XXX): row col may be flipped
//     U64 i_dim = m->i_dim;
//     U64 j_dim = m->j_dim;
//     Assert(j_dim == v.dim);
//     PH_Vector ret = ph_vec_from_dim(arena, i_dim);

//     for(U64 i = 0; i < i_dim; i++)
//     {
//         F32 *dst = &ret.v[i];
//         for(PH_SparseBlock *b = m->col_heads[i]; b != 0; b = b->col_next)
//         {
//             (*dst) += b->v * v.v[b->j];
//         }
//     }
//     return ret;
// }

// internal PH_Vector
// ph_mul_smt_vec(Arena *arena, PH_SparseMatrix *m, PH_Vector v)
// {
//     // TODO(XXX): row col may be flipped
//     U64 i_dim = m->i_dim;
//     U64 j_dim = m->j_dim;
//     Assert(i_dim == v.dim);
//     PH_Vector ret = ph_vec_from_dim(arena, j_dim);

//     for(U64 j = 0; j < j_dim; j++)
//     {
//         F32 *dst = &ret.v[j];
//         for(PH_SparseBlock *b = m->row_heads[j]; b != 0; b = b->row_next)
//         {
//             (*dst) += b->v * v.v[b->i];
//         }
//     }
//     return ret;
// }

//////////////////////////////
// Linear System Solver

// conjugate gradient
// internal PH_Vector
// ph_ls_cg(Arena *arena, PH_SparseMatrix *J, PH_Vector W, PH_Vector b)
// {
//     PH_Vector ret = ph_vec_from_dim(arena, b.dim);
//     Temp scratch = scratch_begin(&arena, 1);
//     F32 norm_b = ph_length_vec(b);

//     // first guess (starts with all 0)
//     PH_Vector xk = ph_vec_from_dim(scratch.arena, b.dim);

//     PH_Vector Axk = ph_mul_smt_vec(scratch.arena, J, xk);
//     Axk = ph_eemul_vec(scratch.arena, W, Axk);
//     Axk = ph_mul_sm_vec(scratch.arena, J, Axk);

//     // initialize residual vector
//     // residual = b - A * x
//     PH_Vector rk = ph_sub_vec(scratch.arena, b, Axk);
//     B32 reached = 0;
//     if(ph_length_vec(rk) < 1e-6*norm_b)
//     {
//         reached = 1;
//     }
//     PH_Vector pk = rk;
//     const U64 max_iter = 300;
//     U64 i;
//     for(i = 0; i < max_iter && !reached; i++)
//     {
//         PH_Vector Apk = ph_mul_smt_vec(scratch.arena, J, pk);
//         Assert(!isnan(Apk.v[0]));
//         Apk = ph_eemul_vec(scratch.arena, W, Apk);
//         Assert(!isnan(Apk.v[0]));
//         Apk = ph_mul_sm_vec(scratch.arena, J, Apk);
//         Assert(!isnan(Apk.v[0]));
//         // // add regularization: Apk += epsilon*pk
//         // PH_Vector reg_term = ph_scale_vec(scratch.arena, pk, 1e-6);
//         // Apk = ph_add_vec(scratch.arena, Apk, reg_term);

//         F32 rk_dot = ph_dot_vec(rk, rk);
//         Assert(isfinite(rk_dot));
//         F32 pk_Apk = ph_dot_vec(pk, Apk);
//         // TODO(XXX): don't know if we should do this
//         if(abs_f32(pk_Apk) < 1e-12) 
//         {
//             reached = 1;
//             break;
//         }
//         Assert(pk_Apk != 0);
//         F32 alpha = rk_dot / pk_Apk;

//         PH_Vector xk1 = ph_add_vec(scratch.arena, xk, ph_scale_vec(scratch.arena, pk, alpha));
//         PH_Vector rk1 = ph_sub_vec(scratch.arena, rk, ph_scale_vec(scratch.arena, Apk, alpha));
//         if(ph_length_vec(rk1) < 1e-6*norm_b)
//         {
//             reached = 1;
//             xk = xk1;
//             break;
//         }
//         Assert(rk_dot != 0);
//         F32 beta = ph_dot_vec(rk1, rk1) / rk_dot;
//         PH_Vector pk1 = ph_add_vec(scratch.arena, rk1, ph_scale_vec(scratch.arena, pk, beta));
//         Assert(!isnan(pk1.v[0]));

//         xk = xk1;
//         pk = pk1;
//         rk = rk1;
//     }
//     Assert(i<max_iter);
//     if(reached)
//     {
//         MemoryCopy(ret.v, xk.v, sizeof(F32)*xk.dim);
//         ret.dim = xk.dim;
//     }

//     scratch_end(scratch);
//     return ret;
// }

internal void gaussj2(F32 **a, U64 n, F32 *b)
{
  Temp scratch = scratch_begin(0,0);
  int *indxc,*indxr,*ipiv;
  int i,icol,irow,j,k,l,ll;
  F32 big,dum,pivinv,temp;

  indxc = push_array(scratch.arena, int, n);
  indxr = push_array(scratch.arena, int, n);
  ipiv = push_array(scratch.arena, int, n);
  // for(j = 0; j < n; j++) ipiv[j] = 0;
  for(i = 0; i < n; i++)
  {
    big = 0.0f;
    for(j = 0; j < n; j++)
    {
      if(ipiv[j] != 1)
      {
        for(k = 0; k < n; k++)
        {
          if(ipiv[k] == 0)
          {
            if(abs_f32(a[j][k]) >= big)
            {
              big = abs_f32(a[j][k]);
              irow = j;
              icol = k;
            }
          }
        }
      }
    }
    ++(ipiv[icol]);

    if(irow != icol)
    {
      for(l = 0; l < n; l++) Swap(F32, a[irow][l], a[icol][l]);
      Swap(F32, b[irow], b[icol]);
    }
    indxr[i] = irow;
    indxc[i] = icol;
    if(a[icol][icol] == 0.0)
    {
      // singular matrix ?
      Assert(false);
    }
    pivinv = 1.0/a[icol][icol];
    a[icol][icol] = 1.0;
    for(l = 0; l < n; l++) a[icol][l] *= pivinv;
    b[icol] *= pivinv;
    for(ll = 0; ll < n; ll++)
    {
      if(ll != icol)
      {
        dum = a[ll][icol];
        a[ll][icol] = 0.0;
        for(l = 0; l < n; l++) a[ll][l] -= a[icol][l]*dum;
        b[ll] -= b[icol]*dum;
      }
    }
  }

  for(l = n-1; l >= 0; l--)
  {
    if(indxr[l] != indxc[l])
    {
      for(k = 0; k < n; k++)
      {
        Swap(F32, a[k][indxr[l]], a[k][indxc[l]]);
      }
    }
  }
  scratch_end(scratch);
}

internal void gaussj(F32 **a, U64 n, F32 **b, U64 m)
{
  Temp scratch = scratch_begin(0,0);
  int *indxc,*indxr,*ipiv;
  int i,icol,irow,j,k,l,ll;
  F32 big,dum,pivinv,temp;

  indxc = push_array(scratch.arena, int, n);
  indxr = push_array(scratch.arena, int, n);
  ipiv = push_array(scratch.arena, int, n);
  for(j = 0; j < n; j++) ipiv[j] = 0;
  for(i = 0; i < n; i++)
  {
    big = 0.0f;
    for(j = 0; j < n; j++)
    {
      if(ipiv[j] != 1)
      {
        for(k = 0; k < n; k++)
        {
          if(ipiv[k] == 0)
          {
            if(abs_f32(a[j][k]) >= big)
            {
              big = abs_f32(a[j][k]);
              irow = j;
              icol = k;
            }
          }
        }
      }
    }
    ++(ipiv[icol]);

    if(irow != icol)
    {
      for(l = 0; l < n; l++) Swap(F32, a[irow][l], a[icol][l]);
      for(l = 0; l < m; l++) Swap(F32, b[irow][l], b[icol][l]);
    }
    indxr[i] = irow;
    indxc[i] = icol;
    if(a[icol][icol] == 0.0)
    {
      // singular matrix ?
      Assert(false);
    }
    pivinv = 1.0/a[icol][icol];
    a[icol][icol] = 1.0;
    for(l = 0; l < n; l++) a[icol][l] *= pivinv;
    for(l = 0; l < m; l++) b[icol][l] *= pivinv;
    for(ll = 0; ll < n; ll++)
    {
      if(ll != icol)
      {
        dum = a[ll][icol];
        a[ll][icol] = 0.0;
        for(l = 0; l < n; l++) a[ll][l] -= a[icol][l]*dum;
        for(l = 0; l < m; l++) b[ll][l] -= b[icol][l]*dum;
      }
    }
  }

  for(l = n-1; l >= 0; l--)
  {
    if(indxr[l] != indxc[l])
    {
      for(k = 0; k < n; k++)
      {
        Swap(F32, a[k][indxr[l]], a[k][indxc[l]]);
      }
    }
  }
  scratch_end(scratch);
}

internal void gaussj_test(void)
{
  Temp scratch = scratch_begin(0,0);
  U64 m = 1;
  U64 n = 3;
  F32 **a = push_array(scratch.arena, F32*, n);
  for(U64 i = 0; i < n; i++)
  {
    a[i] = push_array(scratch.arena, F32, n);
  }
  a[0][0] = 1;
  a[0][1] = 3;
  a[0][2] = 2;

  a[1][0] = 0;
  a[1][1] = 4;
  a[1][2] = 0;

  a[2][0] = 5;
  a[2][1] = 0;
  a[2][2] = 6;

  F32 **b = push_array(scratch.arena, F32*, n);
  for(U64 i = 0; i < n; i++)
  {
    b[i] = push_array(scratch.arena, F32, m);
    b[i][0] = 1;
  }
  F32 *bb = push_array(scratch.arena, F32, n);
  bb[0] = 11;
  bb[1] = 4;
  bb[2] = 28;
  gaussj2(a, n, bb);
}

//////////////////////////////
// Diffeq Solver

internal PH_Vector
ph_ode_euler(Arena *arena, PH_Vector X, DxDt dxdt, F32 t, void *args)
{
  Temp scratch = scratch_begin(&arena,1);
  PH_Vector ret = ph_vec_from_dim(arena, X.dim);
  U64 i;

  // dxdt
  // {xdot, vdot} (for particle3d system)
  // {v(t), ω(t)∗q(t), F(t), τ(t)} (for rigidbody3d system)

  PH_Vector Xdot = dxdt(scratch.arena, X, t, args);

  // scale
  for(i = 0; i < X.dim; i++)
  {
    Xdot.v[i] *= t;
  }

  // add
  for(i = 0; i < X.dim; i++)
  {
    ret.v[i] = X.v[i] + Xdot.v[i];
  }

  scratch_end(scratch);
  return ret;
}

//////////////////////////////
// Step Functions

internal void
ph_step_ps(PH_Particle3DSystem *system, F32 dt)
{
  Temp scratch = scratch_begin(0,0);

  // TODO(XXX): use fixed time step to update the system

  // get state
  PH_Vector X = ph_state_from_ps3d(scratch.arena, system);
  PH_Vector X_next = ph_ode_euler(scratch.arena, X, ph_dxdt_ps3d, dt, system);
  ph_ps3d_state_set(system, X_next);

  system->t += dt;
  scratch_end(scratch);
}

internal void
ph_step_rs(PH_Rigidbody3DSystem *system, F32 dt)
{
  Temp scratch = scratch_begin(0,0);

  // TODO(XXX): use fixed time step to update the system

  // {x(t), q(t), P(t), L(t)}
  PH_Vector X = ph_state_from_rs3d(scratch.arena, system);
  PH_Vector X_next = ph_ode_euler(scratch.arena, X, ph_dxdt_rs3d, dt, system);
  ph_rs3d_state_set(system, X_next);
  system->t += dt;
  scratch_end(scratch);
}
