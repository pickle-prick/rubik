//////////////////////////////
// Particle System State Functions

internal F32 *
ph_state_from_ps3d(Arena *arena, PH_ParticleSystem3D *ps)
{
    // Phase space (position + velocity)
    F32 *ret = push_array(arena, F32, sizeof(F32)*ps->n*6);

    F32 *dst = ret;
    for(PH_Particle3D *p = ps->first_p; p != 0; p = p->next)
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
ph_set_state_for_ps3d(PH_ParticleSystem3D *ps, F32 *src)
{
    for(PH_Particle3D *p = ps->first_p; p != 0; p = p->next)
    {
        p->x.v[0] = *(src++);
        p->x.v[1] = *(src++);
        p->x.v[2] = *(src++);
        p->v.v[0] = *(src++);
        p->v.v[1] = *(src++);
        p->v.v[2] = *(src++);
    }
}

internal F32 *
ph_derivatives_from_ps3d(Arena *arena, PH_ParticleSystem3D *ps)
{
    // xdot + vdot (vel + acc)
    F32 *ret = push_array(arena, F32, sizeof(F32)*ps->n*6);

    // zero the force accumulators
    for(PH_Particle3D *p = ps->first_p; p != 0; p = p->next)
    {
        MemoryZeroStruct(&p->f);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //~ compute forces (applied force + constrained force)

    // global forces
    for(PH_Particle3D *p = ps->first_p; p != 0; p = p->next)
    {
        // gravity
        Vec3F32 f1 = scale_3f32(ps->gravity.dir, ps->gravity.g*p->m);
        // drag
        Vec3F32 f2 = scale_3f32(p->v, ps->visous_drag.kd*-1);
        p->f = add_3f32(p->f, add_3f32(f1, f2));
    }

    for(PH_Force3D *force = ps->first_force; force != 0; force = force->next)
    {
        switch(force->kind)
        {
            case PH_Force3DKind_HookSpring:
            {
                F32 ks = force->v.hook_spring.ks;
                F32 kd = force->v.hook_spring.kd;
                F32 rest = force->v.hook_spring.rest;

                PH_Particle3D *a = force->v.hook_spring.a;
                PH_Particle3D *b = force->v.hook_spring.b;

                Vec3F32 l = sub_3f32(a->x, b->x);
                F32 l_length = length_3f32(l);
                Vec3F32 lnorm = normalize_3f32(l);
                // ldot = va-va
                Vec3F32 ldot = sub_3f32(a->v, b->v);
                F32 factor = -(ks * (l_length-rest) + kd * (dot_3f32(ldot,l) / l_length));
                Vec3F32 f = scale_3f32(lnorm, factor);
                a->f = add_3f32(a->f, f);
                b->f = add_3f32(b->f, negate_3f32(f));

                rk_debug_gfx(36, v2f32(300,300), v4f32(1,1,0,1), push_str8f(rk_frame_arena(), "%f %f %f", f.x, f.y, f.z));
                rk_debug_gfx(36, v2f32(300,100), v4f32(1,1,0,1), push_str8f(rk_frame_arena(), "distance: %f", l_length));
            }break;
            default:{InvalidPath;}break;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //~ copy result

    F32 *dst = ret;
    for(PH_Particle3D *p = ps->first_p; p != 0; p = p->next)
    {
        *(dst++) = p->v.v[0]; /* xdot = v */
        *(dst++) = p->v.v[1];
        *(dst++) = p->v.v[2];

        *(dst++) = p->f.v[0] / p->m; /* vdot = f/m */
        *(dst++) = p->f.v[1] / p->m;
        *(dst++) = p->f.v[2] / p->m;
    }
    return ret;
}

//////////////////////////////
// Diffeq Solver (Particle System)

internal void ph_euler_step_for_ps3d(PH_ParticleSystem3D *ps, F32 delta_t)
{
    Temp scratch = scratch_begin(0,0);

    U64 dim = sizeof(F32)*6*ps->n;

    // compute/eval derivatives (xdot + vdot)
    F32 *derivs = ph_derivatives_from_ps3d(scratch.arena, ps);

    // scale it based on delta_t
    for(U64 i = 0; i < dim; i++)
    {
        derivs[i] *= delta_t;
    }

    // get state (phase space) (x + xdot)
    F32 *phase_old = ph_state_from_ps3d(scratch.arena, ps);
    F32 *phase_new = push_array(scratch.arena, F32, dim);

    // add
    for(U64 i = 0; i < dim; i++)
    {
        phase_new[i] = phase_old[i] + derivs[i];
    }

    // update state
    ph_set_state_for_ps3d(ps, phase_new);

    // update clock
    ps->t += delta_t;
    scratch_end(scratch);
}
