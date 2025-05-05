#ifndef PHYSICS_CORE_H
#define PHYSICS_CORE_H

//////////////////////////////
// Some Constants

// 3(pos) + 4(rotation q) + 3(P) + 3(L) = 13
#define  PH_RB3D_STATE_DIM 13 /* rigidbody3d state dimension */

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
    PH_Force3DKind_Constant, /* push or pull */
    PH_Force3DKind_COUNT,
} PH_Force3DKind;

typedef enum PH_Constraint3DKind
{
    PH_Constraint3DKind_Distance,
    PH_Constraint3DKind_PointOnSphere,
    PH_Constraint3DKind_PointOnCurve,
    PH_Constraint3DKind_COUNT,
} PH_Constraint3DKind;

typedef enum PH_Rigidbody3DShapeKind
{
    PH_Rigidbody3DShapeKind_Point, /* TODO(XXX): do we need this*/
    PH_Rigidbody3DShapeKind_Sphere,
    PH_Rigidbody3DShapeKind_Cuboid,
    PH_Rigidbody3DShapeKind_COUNT,
} PH_Rigidbody3DShapeKind;

//////////////////////////////
// Flags

typedef U64 PH_Particle3DFlags;
#define PH_Particle3DFlag_OmitGravity  (PH_Particle3DFlags)(1ull<<0)

typedef U64                            PH_Rigidbody3DFlags;
#define PH_Rigidbody3DFlag_OmitGravity (PH_Rigidbody3DFlags)(1ull<<0)
#define PH_Rigidbody3DFlag_Static      (PH_Rigidbody3DFlags)(1ull<<1)

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

typedef struct PH_Force3D_Constant PH_Force3D_Constant;
struct PH_Force3D_Constant
{
    PH_Force3D_Constant *next;
    Vec3F32 direction;
    F32 strength;
};

typedef struct PH_Force3D PH_Force3D;
struct PH_Force3D
{
    U64 idx;
    PH_Force3D *next;
    PH_Force3DKind kind;

    // NOTE(k): relative to the center of mass
    Vec3F32 contact; /* for rigidbody */ 
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
        PH_Force3D_Constant constant;
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

typedef struct PH_Particle3DSystem PH_Particle3DSystem;
struct PH_Particle3DSystem
{
    /////////////////////////////////////////////////////////////////////////////////////
    //~ per frame equipments

    PH_Particle3D *first_particle;
    PH_Particle3D *last_particle;
    U64 particle_count;

    /////////////////////////////////////////////////////////////////////////////////////
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

//////////////////////////////
// Rigidbody3D & System

typedef struct PH_Rigidbody3D PH_Rigidbody3D;
struct PH_Rigidbody3D
{
    PH_Rigidbody3D *next;

    U64 idx;
    PH_Rigidbody3DShapeKind shape;
    union
    {
        Vec3F32 v3f32;
    } dim;
    // TODO(XXX): do we need to recalculate Ibody/Ibodyinv if dim is changed?
    union
    {
        Vec3F32 v3f32;
    } last_dim;
    PH_Rigidbody3DFlags flags;

    // constant quantities
    F32 mass; /* M */
    Mat3x3F32 Ibody;
    Mat3x3F32 Ibodyinv;

    // state variables
    Vec3F32 x; /* position */
    QuatF32 q; /* rotation quternion */ 
    Vec3F32 P; /* linear momentum */
    Vec3F32 L; /* angular momentum */

    // derived quantities (auxiliary variables)
    Mat3x3F32 R; /* rotation matrix computed from q */
    Mat3x3F32 Iinv; /* inverse of inertia(world) */
    Vec3F32 v; /* linear velocity */
    Vec3F32 omega; /* angular velocity ω(t) */

    // computed quantities(artifacts)
    Vec3F32 force;
    Vec3F32 torque;
};

// TODO(XXX)
typedef struct PH_Rigidbody3DSystem PH_Rigidbody3DSystem;
struct PH_Rigidbody3DSystem
{
    /////////////////////////////////////////////////////////////////////////////////////
    //~ per frame equipments

    PH_Rigidbody3D *first_body;
    PH_Rigidbody3D *last_body;
    U64 body_count;

    /////////////////////////////////////////////////////////////////////////////////////
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

//////////////////////////////
// Particle/System Functions

internal PH_Vector ph_state_from_ps3d(Arena *arena, PH_Particle3DSystem *ps);
internal void      ph_ps3d_state_set(PH_Particle3DSystem *ps, PH_Vector state);
internal PH_Vector ph_dxdt_ps3d(Arena *arena, PH_Vector X, F32 dt, void *system);

//////////////////////////////
// Rigidbody3D Functions

// rigidbody
internal void ph_state_from_rb3d(PH_Rigidbody3D *rb, F32 *dst);
internal void ph_rb3d_state_set(PH_Rigidbody3D *rb, F32 *src);
// system
internal PH_Vector ph_state_from_rs3d(Arena *arena, PH_Rigidbody3DSystem *system);
internal void      ph_rs3d_state_set(PH_Rigidbody3DSystem *system, PH_Vector state);
// dxdt
internal PH_Vector ph_dxdt_rs3d(Arena *arena, PH_Vector X, F32 t, void *system);

// helpers
internal Mat3x3F32 ph_inertia_from_cuboid(F32 M, Vec3F32 dim);
internal Mat3x3F32 ph_inertiainv_from_cuboid(F32 M, Vec3F32 dim);

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

// matrix (row major)
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
// Diffeq/ODE Solver

typedef PH_Vector (*DxDt)(Arena *arena, PH_Vector X, F32 t, void *args);
internal PH_Vector ph_ode_euler(Arena *arena, PH_Vector X, DxDt dxdt, F32 t, void *args);

//////////////////////////////
// Step Functions

internal void ph_step_ps(PH_Particle3DSystem *system, F32 dt);
internal void ph_step_rs(PH_Rigidbody3DSystem *system, F32 dt);

#endif // PHYSICS_CORE_H
