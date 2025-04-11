#ifndef PHYSICS_CORE_H
#define PHYSICS_CORE_H

//////////////////////////////
// Enums

// TODO(XXX)
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
    PH_Force3DKind_COUNT,
} PH_Force3DKind;

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

    void *a;
    void *b;
};

typedef struct PH_Force3D PH_Force3D;
struct PH_Force3D
{
    PH_Force3D *next;
    PH_Force3DKind kind;
    // move targets here?
    union
    {
        PH_Force3D_Gravity gravity;
        PH_Force3D_VisousDrag visous_drag;
        PH_Force3D_HookSpring hook_spring;
    } v;
};

typedef struct PH_Particle3D PH_Particle3D;
struct PH_Particle3D
{
    PH_Particle3D *next;
    F32 m; /* mass */
    Vec3F32 x; /* position */
    Vec3F32 v; /* velocity */ 
    Vec3F32 f; /* force accumulator */
};

typedef struct PH_ParticleSystem3D PH_ParticleSystem3D;
struct PH_ParticleSystem3D
{
    ////////////////////////////////
    //~ per frame equipments

    PH_Particle3D *first_p;
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
    U64 force_count;
};

// TODO(XXX)
typedef struct PH_RigidBody3DSystem PH_RigidBody3DSystem;
struct PH_RigidBody3DSystem
{
    F32 mass;
};

//////////////////////////////
// Particle System State Functions

internal F32* ph_state_from_ps3d(Arena *arena, PH_ParticleSystem3D *ps);
internal void ph_set_state_for_ps3d(PH_ParticleSystem3D *ps, F32 *src);
internal F32* ph_derivatives_from_ps3d(Arena *arena, PH_ParticleSystem3D *ps);

//////////////////////////////
// Diffeq Solver (Particle System)

internal void ph_euler_step_for_ps(PH_ParticleSystem3D *ps, F32 delta_t);

#endif // PHYSICS_CORE_H
