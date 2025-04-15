#ifndef PHYSICS_CORE_H
#define PHYSICS_CORE_H

//////////////////////////////
// Enums

typedef enum PH_Force3DKind
{
    // Unary force: gravity, drag
    PH_Force3DKind_Gravity,
    PH_Force3DKind_VisousDrag,
    // N-ary force: spring
    PH_Force3DKind_HookSpring,
    // Spatial interaction force: attraction and replusion
    PH_Force3DKind_Attraction,
    PH_Force3DKind_Replusion,
    PH_Force3DKind_Const,
    PH_Force3DKind_COUNT,
} PH_Force3DKind;

typedef enum PH_Constraint3DKind
{
    PH_Constraint3DKind_Distance,
    PH_Constraint3DKind_PointOnSphere,
    PH_Constraint3DKind_PointOnCurve,
    PH_Constraint3DKind_COUNT,
} PH_Constraint3DKind;

typedef U64 PH_Particle3DFlags;
#define PH_Particle3DFlag_OmitGravity       (PH_Particle3DFlags)(1ull<<0)

//////////////////////////////
// Force Types

typedef struct PH_Force3D_Gravity PH_Force3D_Gravity;
struct PH_Force3D_Gravity
{
    F32 g;
    Vec3F32 dir;
};

typedef struct PH_Force3D_VisousDrag PH_Force3D_VisousDrag;
struct PH_Force3D_VisousDrag
{
    F32 kd; /* f = -kd*v */
};

// Hook's law spring (A basic mass-and-spring simulation)
typedef struct PH_Force3D_HookSpring PH_Force3D_HookSpring;
struct PH_Force3D_HookSpring
{
    F32 ks; /* spring constant */
    F32 kd; /* damping constant */
    F32 rest; /* rest length */
};

typedef struct PH_Force3D_Const PH_Force3D_Const;
struct PH_Force3D_Const
{
    PH_Force3D_Const *next;
    Vec3F32 direction;
    F32 strength;
};

typedef struct PH_Force3D PH_Force3D;
struct PH_Force3D
{
    U64 idx;
    PH_Force3D *next;
    PH_Force3DKind kind;
    union
    {
        // TODO(XXX): support unary&binary for now
        struct {void* a; void *b;};
        void *v;
    } targets;
    U64 target_count;

    union
    {
        PH_Force3D_Gravity gravity;
        PH_Force3D_VisousDrag visous_drag;
        PH_Force3D_HookSpring hook_spring;
        PH_Force3D_Const constf;
    } v;
};

//////////////////////////////
// Constraint Types

typedef struct PH_Constraint3D_Distance PH_Constraint3D_Distance;
struct PH_Constraint3D_Distance
{
    F32 d; /* distance */
};

typedef struct PH_Constraint3D PH_Constraint3D;
struct PH_Constraint3D
{
    PH_Constraint3D *next;
    U64 idx;
    // we need C(q) and Cdot(q) eval for different kind of constraints
    PH_Constraint3DKind kind;

    union
    {
        // TODO(XXX): support unary&binary for now
        struct {void* a; void *b;};
        void *v[2];
    } targets;
    U64 target_count;

    union
    {
        PH_Constraint3D_Distance distance;
    } v;
};

typedef struct PH_SparseBlock PH_SparseBlock;
struct PH_SparseBlock
{
    PH_SparseBlock *row_next;
    PH_SparseBlock *col_next;
    U64 i;
    U64 j;
    F32 v;
};

typedef struct PH_SparseMatrix PH_SparseMatrix;
struct PH_SparseMatrix
{
    U64 i_dim;
    U64 j_dim;

    PH_SparseBlock **row_heads;
    PH_SparseBlock **row_tails;
    PH_SparseBlock **col_heads;
    PH_SparseBlock **col_tails;
    U64 count;
};

//////////////////////////////
// Matrix Vector Types

typedef struct PH_Vector PH_Vector;
struct PH_Vector
{
    U64 dim;
    F32 *v;
};

typedef struct PH_Matrix PH_Matrix;
struct PH_Matrix
{
    U64 i_dim;
    U64 j_dim;
    F32 **v;
};

//////////////////////////////
// Particle3D & System

typedef struct PH_Particle3D PH_Particle3D;
struct PH_Particle3D
{
    PH_Particle3D *next;
    U64 idx;
    PH_Particle3DFlags flags;
    F32 m; /* mass */
    Vec3F32 x; /* position */
    Vec3F32 v; /* velocity */ 
    Vec3F32 f; /* force accumulator */

    PH_Force3D_VisousDrag visous_drag;
};

typedef struct PH_ParticleSystem3D PH_ParticleSystem3D;
struct PH_ParticleSystem3D
{
    ////////////////////////////////
    //~ per frame equipments

    PH_Particle3D *first_p;
    PH_Particle3D *last_p;
    U64 n; /* particle count */

    ////////////////////////////////
    //~ persistent state

    F32 t; /* simulation clock */

    ////////////////////////////////
    // global forces

    PH_Force3D_Gravity gravity;
    PH_Force3D_VisousDrag visous_drag;

    ////////////////////////////////
    // forces

    PH_Force3D *first_force;
    PH_Force3D *last_force;
    U64 force_count;

    ////////////////////////////////
    // constraints

    PH_Constraint3D *first_constraint;
    PH_Constraint3D *last_constraint;
    U64 constraint_count;
};

// TODO(XXX)
typedef struct PH_RigidBody3DSystem PH_RigidBody3DSystem;
struct PH_RigidBody3DSystem
{
    F32 mass;
};

//////////////////////////////
// Particle System State Functions

internal PH_Vector ph_state_from_ps3d(Arena *arena, PH_ParticleSystem3D *ps);
internal void      ph_set_state_for_ps3d(PH_ParticleSystem3D *ps, PH_Vector state);
internal PH_Vector ph_derivatives_from_ps3d(Arena *arena, PH_ParticleSystem3D *ps);

//////////////////////////////
// Constraint Eval Functions

internal PH_Vector ph_C_distance3D(Arena *arena, Vec3F32 x1, Vec3F32 x2);

//////////////////////////////
// Aribitrary-length Matrix/Vector Building

internal PH_Matrix ph_mat_from_dim(Arena *arena, U64 i_dim, U64 j_dim);
internal PH_Vector ph_vec_from_dim(Arena *arena, U64 dim);
internal PH_Vector ph_vec_copy(Arena *arena, PH_Vector src);

//////////////////////////////
// Aribitrary-length Matrix/Vector Math Operations

// vector
internal PH_Vector ph_add_vec(Arena *arena, PH_Vector a, PH_Vector b);
internal PH_Vector ph_sub_vec(Arena *arena, PH_Vector a, PH_Vector b);
internal F32       ph_dot_vec(PH_Vector a, PH_Vector b);
internal PH_Vector ph_scale_vec(Arena *arena, PH_Vector v, F32 s);
internal PH_Vector ph_negate_vec(Arena *arena, PH_Vector v);
internal PH_Vector ph_eemul_vec(Arena *arena, PH_Vector a, PH_Vector b); /* element-wise mul */
internal F32       ph_length_vec(PH_Vector v);

// matrix
internal PH_Matrix ph_mul_mm(Arena *arena, PH_Matrix A, PH_Matrix B);
internal PH_Vector ph_mul_mv(Arena *arena, PH_Matrix A, PH_Vector v);
internal PH_Matrix ph_trp_mat(Arena *arena, PH_Matrix A); /* transpose */

// sparse Matrix Computation
internal PH_Vector       ph_mul_sm_vec(Arena *arena, PH_SparseMatrix *m, PH_Vector v);
internal PH_Vector       ph_mul_smt_vec(Arena *arena, PH_SparseMatrix *m, PH_Vector v);
internal PH_SparseMatrix ph_sparsed_m_from_blocks(Arena *arena, PH_SparseBlock *blocks, U64 i_dim, U64 j_dim);

//////////////////////////////
// Linear System Solver

// conjugate gradient
internal PH_Vector ph_ls_cg(Arena *arena, PH_SparseMatrix *J, PH_Vector W, PH_Vector b);
internal void gaussj(F32 **a, U64 n, F32 **b, U64 m);
internal void gaussj2(F32 **a, U64 n, F32 *b);
internal void gaussj_test(void);

//////////////////////////////
// Diffeq Solver (Particle System)

internal void ph_euler_step_for_ps(PH_ParticleSystem3D *ps, F32 delta_t);

#endif // PHYSICS_CORE_H
