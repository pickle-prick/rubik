//////////////////////////////
// Particle System State Functions

internal PH_Vector
ph_state_from_ps3d(Arena *arena, PH_ParticleSystem3D *ps)
{
    // Phase space (position + velocity)
    PH_Vector ret = ph_vec_from_dim(arena, ps->n*6);

    F32 *dst = ret.v;
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
ph_set_state_for_ps3d(PH_ParticleSystem3D *ps, PH_Vector state)
{
    F32 *src = state.v;
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

internal PH_Vector
ph_derivatives_from_ps3d(Arena *arena, PH_ParticleSystem3D *ps)
{
    Temp scratch = scratch_begin(&arena, 1);

    // xdot + vdot (vel + acc)
    PH_Vector ret = ph_vec_from_dim(arena, ps->n*6);

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
        Vec3F32 f1 = {0};
        if(!(p->flags & PH_Particle3DFlag_OmitGravity))
        {
            f1 = scale_3f32(ps->gravity.dir, ps->gravity.g*p->m);
        }
        p->f = add_3f32(p->f, f1);

        // drag
        Vec3F32 f2 = scale_3f32(p->v, ps->visous_drag.kd*-1);
        p->f = add_3f32(p->f, f2);

        // individual drag
        Vec3F32 f3 = scale_3f32(p->v, p->visous_drag.kd*-1);
        p->f = add_3f32(p->f, f3);
    }

    // unconstraint forces
    for(PH_Force3D *force = ps->first_force; force != 0; force = force->next)
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
    if(ps->constraint_count > 0)
    {
        /////////////////////////////////////////////////////////////////////////////////
        // Collect components

        U64 m = ps->constraint_count;
        U64 n = ps->n; /* particle count */
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
        for(PH_Particle3D *p = ps->first_p; p != 0; p = p->next)
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
        for(PH_Constraint3D *c = ps->first_constraint; c != 0; c = c->next)
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
        for(PH_Constraint3D *c = ps->first_constraint; c != 0; c = c->next)
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
        for(PH_Particle3D *p = ps->first_p; p != 0; p = p->next)
        {
            p->f.v[0] += *(src++);
            p->f.v[1] += *(src++);
            p->f.v[2] += *(src++);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //~ copy result

    F32 *dst = ret.v;
    for(PH_Particle3D *p = ps->first_p; p != 0; p = p->next)
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

// matrix
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

internal PH_Vector
ph_mul_sm_vec(Arena *arena, PH_SparseMatrix *m, PH_Vector v)
{
    // TODO(XXX): row col may be flipped
    U64 i_dim = m->i_dim;
    U64 j_dim = m->j_dim;
    Assert(j_dim == v.dim);
    PH_Vector ret = ph_vec_from_dim(arena, i_dim);
    
    for(U64 i = 0; i < i_dim; i++)
    {
        F32 *dst = &ret.v[i];
        for(PH_SparseBlock *b = m->col_heads[i]; b != 0; b = b->col_next)
        {
            (*dst) += b->v * v.v[b->j];
        }
    }
    return ret;
}

internal PH_Vector
ph_mul_smt_vec(Arena *arena, PH_SparseMatrix *m, PH_Vector v)
{
    // TODO(XXX): row col may be flipped
    U64 i_dim = m->i_dim;
    U64 j_dim = m->j_dim;
    Assert(i_dim == v.dim);
    PH_Vector ret = ph_vec_from_dim(arena, j_dim);
    
    for(U64 j = 0; j < j_dim; j++)
    {
        F32 *dst = &ret.v[j];
        for(PH_SparseBlock *b = m->row_heads[j]; b != 0; b = b->row_next)
        {
            (*dst) += b->v * v.v[b->i];
        }
    }
    return ret;
}


//////////////////////////////
// Linear System Solver

// conjugate gradient
internal PH_Vector
ph_ls_cg(Arena *arena, PH_SparseMatrix *J, PH_Vector W, PH_Vector b)
{
    PH_Vector ret = ph_vec_from_dim(arena, b.dim);
    Temp scratch = scratch_begin(&arena, 1);
    F32 norm_b = ph_length_vec(b);

    // first guess (starts with all 0)
    PH_Vector xk = ph_vec_from_dim(scratch.arena, b.dim);

    PH_Vector Axk = ph_mul_smt_vec(scratch.arena, J, xk);
    Axk = ph_eemul_vec(scratch.arena, W, Axk);
    Axk = ph_mul_sm_vec(scratch.arena, J, Axk);

    // initialize residual vector
    // residual = b - A * x
    PH_Vector rk = ph_sub_vec(scratch.arena, b, Axk);
    B32 reached = 0;
    if(ph_length_vec(rk) < 1e-6*norm_b)
    {
        reached = 1;
    }
    PH_Vector pk = rk;
    const U64 max_iter = 300;
    U64 i;
    for(i = 0; i < max_iter && !reached; i++)
    {
        PH_Vector Apk = ph_mul_smt_vec(scratch.arena, J, pk);
        Assert(!isnan(Apk.v[0]));
        Apk = ph_eemul_vec(scratch.arena, W, Apk);
        Assert(!isnan(Apk.v[0]));
        Apk = ph_mul_sm_vec(scratch.arena, J, Apk);
        Assert(!isnan(Apk.v[0]));
        // // add regularization: Apk += epsilon*pk
        // PH_Vector reg_term = ph_scale_vec(scratch.arena, pk, 1e-6);
        // Apk = ph_add_vec(scratch.arena, Apk, reg_term);

        F32 rk_dot = ph_dot_vec(rk, rk);
        Assert(isfinite(rk_dot));
        F32 pk_Apk = ph_dot_vec(pk, Apk);
        // TODO(XXX): don't know if we should do this
        if(abs_f32(pk_Apk) < 1e-12) 
        {
            reached = 1;
            break;
        }
        Assert(pk_Apk != 0);
        F32 alpha = rk_dot / pk_Apk;

        PH_Vector xk1 = ph_add_vec(scratch.arena, xk, ph_scale_vec(scratch.arena, pk, alpha));
        PH_Vector rk1 = ph_sub_vec(scratch.arena, rk, ph_scale_vec(scratch.arena, Apk, alpha));
        if(ph_length_vec(rk1) < 1e-6*norm_b)
        {
            reached = 1;
            xk = xk1;
            break;
        }
        Assert(rk_dot != 0);
        F32 beta = ph_dot_vec(rk1, rk1) / rk_dot;
        PH_Vector pk1 = ph_add_vec(scratch.arena, rk1, ph_scale_vec(scratch.arena, pk, beta));
        Assert(!isnan(pk1.v[0]));

        xk = xk1;
        pk = pk1;
        rk = rk1;
    }
    Assert(i<max_iter);
    if(reached)
    {
        MemoryCopy(ret.v, xk.v, sizeof(F32)*xk.dim);
        ret.dim = xk.dim;
    }

    scratch_end(scratch);
    return ret;
}

internal void gaussj2(F32 **a, U64 n, F32 *b)
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
    Trap();
}

//////////////////////////////
// Diffeq Solver (Particle System)

internal void ph_euler_step_for_ps3d(PH_ParticleSystem3D *ps, F32 delta_t)
{
    Temp scratch = scratch_begin(0,0);

    U64 dim = 6*ps->n;

    // compute/eval derivatives (xdot + vdot)
    PH_Vector derivs = ph_derivatives_from_ps3d(scratch.arena, ps);

    // scale it based on delta_t
    for(U64 i = 0; i < dim; i++)
    {
        derivs.v[i] *= delta_t;
    }

    // get state (phase space) (x + xdot)
    PH_Vector phase_old = ph_state_from_ps3d(scratch.arena, ps);
    PH_Vector phase_new = ph_vec_from_dim(scratch.arena, dim);

    // add
    for(U64 i = 0; i < dim; i++)
    {
        phase_new.v[i] = phase_old.v[i] + derivs.v[i];
    }

    // update state
    ph_set_state_for_ps3d(ps, phase_new);

    // update clock
    ps->t += delta_t;
    scratch_end(scratch);
}
