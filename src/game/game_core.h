#ifndef GAME_CORE_H
#define GAME_CORE_H

/////////////////////////////////
// Enum

typedef enum G_PhysicsKind {
    G_PhysicsKind_Static,
    G_PhysicsKind_Kinematic,
    G_PhysicsKind_Dynamic,
    G_PhysicsKind_COUNT,
} G_PhysicsKind;

typedef enum G_NodeKind {
    G_NodeKind_Null,
    G_NodeKind_MeshInstance3D,
    G_NodeKind_CollisionShape3D,
    G_NodeKind_CharacterBody3D,
    G_NodeKind_RigidBody3D,
    G_NodeKind_Camera3D,
    G_NodeKind_COUNT,
} G_NodeKind;

typedef U64 G_NodeFlags;
# define G_NodeFlags_Visiable            (G_NodeFlags)(1ull<<0)

typedef enum G_MeshKind {
    G_MeshKind_Box,
    G_MeshKind_Plane,
    G_MeshKind_Capsule,
    G_MeshKind_Cylinder,
    G_MeshKind_Sphere,
    G_MeshKind_Custom,
    G_MeshKind_COUNT,
} G_MeshKind;

typedef U64 G_SpecialKeyKind;
#define G_SpecialKeyKind_GizmosIhat (U64)(0xffffffffffffffffull-0)
#define G_SpecialKeyKind_GizmosJhat (U64)(0xffffffffffffffffull-1)
#define G_SpecialKeyKind_GizmosKhat (U64)(0xffffffffffffffffull-2)
#define G_SpecialKeyKind_Grid (U64)(0xffffffffffffffffull-3)

/////////////////////////////////
// For magic (EPA)

// typedef struct G_Vertex2DNode G_Vertex2DNode;
// struct G_Vertex2DNode {
//     Vec2F32      v;
//     G_Vertex2DNode *next;
//     G_Vertex2DNode *prev;
// };
// 
// typedef struct G_Contact2D G_Contact2D;
// struct G_Contact2D {
//     Vec2F32 normal;
//     F32     length;
// };

/////////////////////////////////
// Key

typedef struct G_Key G_Key;
struct G_Key {
    U64 u64[1];
};

/////////////////////////////////
// Material

// TODO
typedef struct G_Material G_Material;

/////////////////////////////////
//~ Resource

//- 2D Sprite

typedef struct G_Tex2D G_Tex2D;
struct G_Tex2D {
    R_Handle texture;
    U64      w;
    U64      h;
    U64      rc;
};

typedef struct G_Sprite2D_Frame G_Sprite2D_Frame;
struct G_Sprite2D_Frame {
    U64       x;
    U64       y;
    U64       w;
    U64       h;
    R_Handle  texture;
    F32       duration;
};

typedef struct G_Sprite2D_FrameTag G_Sprite2D_FrameTag;
struct G_Sprite2D_FrameTag {
    String8 name;
    U64     from;
    U64     to;
};

typedef struct G_SpriteSheet2D G_SpriteSheet2D;
struct G_SpriteSheet2D {
    R_Handle            texture;
    U64                 w;
    U64                 h;

    G_Sprite2D_FrameTag *tags;
    U64                 tag_count;
    G_Sprite2D_Frame    *frames;
    U64                 frame_count;
};

//- Mesh

typedef struct G_Mesh G_Mesh;
struct G_Mesh {
    G_Mesh     *next;
    R_Handle   vertices;
    R_Handle   indices;
    G_MeshKind kind;
    B32        flip_face;
};

/////////////////////////////////
//~ Node Equipment

// typedef struct G_Sprite2D G_Sprite2D;
// struct G_Sprite2D {
//     R_Handle texture;
//     U64      x;
//     U64      y;
//     U64      w;
//     U64      h;
//     B32      flip_h;
//     B32      flip_v;
// };
// 
// typedef struct G_Shape2D G_Shape2D;
// struct G_Shape2D {
//     Vec4F32       color;
//     F32           border_thickness;
//     G_Shape2DKind kind;
// 
//     union {
//         RectShape2D   rect;
//         CircleShape2D circle;
//     };
// };
// 
// typedef struct G_Animation2D G_Animation2D;
// struct G_Animation2D {
//     U64              key;
//     G_Animation2D    *hash_next;
//     G_Animation2D    *hash_prev;
// 
//     G_Sprite2D_Frame *frames;
//     U64              frame_count;
//     U64              duration;
// };
// 
// typedef struct G_Animation2DHashSlot G_Animation2DHashSlot;
// struct G_Animation2DHashSlot {
//     G_Animation2D *first;
//     G_Animation2D *last;
// };
// 
// typedef struct G_AnimatedSprite2D G_AnimatedSprite2D;
// struct G_AnimatedSprite2D {
//     B32                   flip_h;
//     B32                   flip_v;
//     F32                   speed_scale;
// 
//     String8               auto_play;
//     U64                   frame_idx;
//     U64                   frame_progress; // ms
// 
//     G_Animation2DHashSlot *anim2d_hash_table;
//     U64                   anim2d_hash_table_size;
// };
// 
// typedef struct G_RigidBody2D G_RigidBody2D;
// struct G_RigidBody2D {
//     Vec2F32       velocity;
//     Vec2F32       acc;
//     Vec2F32       force;
//     F32           mass;
// 
//     // Body
//     G_PhysicsKind physics_kind;
//     G_Shape2DKind body_kind;
//     union {
//         RectShape2D rect;
//         CircleShape2D circle;
//     } shape;
// };

//- Mesh
typedef struct G_MeshInstance3D G_MeshInstance3D;
struct G_MeshInstance3D {
    G_Mesh    *mesh;
    // TODO: surface
    // G_Texture *surface;
    // TODO: custom AABB
    // TODO: skeleton
};

typedef struct G_Camera3D G_Camera3D;
struct G_Camera3D {
    F32     fov;
    F32     zn;
    F32     zf;
};

/////////////////////////////////
//~ Node Type

struct G_Node;
struct G_Scene;
#define G_NODE_CUSTOM_UPDATE(name) void name(struct G_Node *node, struct G_Scene *scene, OS_EventList os_events, F32 dt_sec)
typedef G_NODE_CUSTOM_UPDATE(G_NodeCustomUpdateFunctionType);

#define G_NODE_CUSTOM_DRAW(name) void name(struct G_Node *node, void *node_data)
typedef G_NODE_CUSTOM_DRAW(G_NodeCustomDrawFunctionType);

typedef struct G_Node G_Node;
struct G_Node {
    G_Node                         *parent;
    G_Node                         *first;
    G_Node                         *last;
    G_Node                         *next;
    G_Node                         *prev;
    G_Node                         *hash_next;
    G_Node                         *hash_prev;

    G_Key                          key;
    String8                        name;
    G_NodeFlags                    flags;
    G_NodeKind                     kind;

    // Transform
    Vec3F32                        pos;
    QuatF32                        rot;
    Vec3F32                        scale;
    // Delta
    Vec3F32                        pos_delta;
    QuatF32                        rot_delta;
    Vec3F32                        scale_delta;

    // Xform
    // TODO: remove base_xform
    Mat4x4F32                      base_xform;
    Mat4x4F32                      fixed_xform; // Calculated every iteration

    // TODO: rotation edit mode: Euler, Quaternion, Basis

    void                           *custom_data;
    G_NodeCustomUpdateFunctionType *update_fn;
    G_NodeCustomDrawFunctionType   *custom_draw;

    union
    {
        G_MeshInstance3D mesh_inst3d;
        G_Camera3D       camera;
    };
    
};

typedef struct G_BucketSlot G_BucketSlot;
struct G_BucketSlot {
    G_Node *first; 
    G_Node *last; 
};

typedef struct G_Bucket G_Bucket;
struct G_Bucket {
    Arena        *arena;
    G_BucketSlot *node_hash_table;
    U64          node_hash_table_size;
    U64          node_count;

    G_Node       *first_free_node;
};

typedef struct G_Scene G_Scene;
struct G_Scene {
    Arena    *arena;
    G_Bucket *bucket;
    G_Node   *root;
    G_Node   *camera;

    G_Mesh   *first_free_mesh;
};

/////////////////////////////////
// Generated code

#include "generated/game.meta.h"

typedef struct G_State G_State;
struct G_State
{
    Arena     *arena;
    G_Bucket  *node_bucket;
    D_Bucket  *bucket_rect;
    D_Bucket  *bucket_geo3d;
    UI_Signal sig;

    G_Key     hot_key;
    G_Key     active_key;
    B32       is_dragging;
    Vec3F32   drag_start_direction;

    OS_Handle os_wnd;

    G_DeclStackNils;
    G_DeclStacks;
};

/////////////////////////////////
// Globals

global G_State *g_state;
// read_only global G_Node g_g_nil_node = {
//     &g_g_nil_node,
//     &g_g_nil_node,
//     &g_g_nil_node,
//     &g_g_nil_node,
//     &g_g_nil_node,
//     &g_g_nil_node,
//     &g_g_nil_node,
// };

internal void g_init(OS_Handle os_wnd);
// internal B32  g_node_is_nil(G_Node *n);

/////////////////////////////////
// Key

internal G_Key g_key_from_string(String8 string);
internal B32   g_key_match(G_Key a, G_Key b);
internal G_Key g_key_zero();

/////////////////////////////////
// Bucket

internal G_Node   *g_node_from_string(G_Bucket *bucket, String8 string);
internal G_Node   *g_node_from_key(G_Bucket *bucket, G_Key key);
internal G_Bucket *g_bucket_make(Arena *arena, U64 hash_table_size);

/////////////////////////////////
// Node build api

internal G_Node *g_build_node_from_string(String8 name);
internal G_Node *g_build_node_from_key(G_Key key);
internal G_Node *g_node_camera3d_alloc(String8 string);
internal G_Node *g_node_camera_mesh_inst3d_alloc(String8 string);
internal G_Mesh *g_mesh_alloc();

/////////////////////////////////
// Node Type Functions

internal G_Node *g_node_df(G_Node *n, G_Node *root, U64 sib_member_off, U64 child_member_off); 
#define g_node_df_pre(node, root) g_node_df(node, root, OffsetOf(G_Node, next), OffsetOf(G_Node, first))
#define g_node_df_post(node, root) g_node_df(node, root, OffsetOf(G_Node, prev), OffsetOf(G_Node, last))

internal Mat4x4F32 g_xform_from_node(G_Node *node);
internal void      g_local_coord_from_node(G_Node *node, Vec3F32 *f, Vec3F32 *s, Vec3F32 *u);
internal Mat4x4F32 g_view_from_node(G_Node *node);

/////////////////////////////////
// Node modification

/////////////////////////////////
// Physics

internal void g_physics_dynamic_root(G_Node *node, F32 dt);

/////////////////////////////////
// Magic

#define G_SHAPE_SUPPORT_FN(name) Vec2F32 name(void *shape_data, Vec2F32 direction)
typedef G_SHAPE_SUPPORT_FN(G_SHAPE_CUSTOM_SUPPORT_FN);
G_SHAPE_SUPPORT_FN(G_SHAPE_RECT_SUPPORT_FN);

// internal B32         gjk(Vec3F32 s1_center, Vec3F32 s2_center, void *s1_data, void *s2_data, G_SHAPE_CUSTOM_SUPPORT_FN s1_support_fn, G_SHAPE_CUSTOM_SUPPORT_FN s2_support_fn, Vec3F32 simplex[4]);
internal Vec2F32     triple_product_2f32(Vec2F32 A, Vec2F32 B, Vec2F32 C);

/////////////////////////////////
// Camera


/////////////////////////////////
// Helpers

internal U64 g_hash_from_string(U64 seed, String8 string);
#define FileReadAll(arena, fp, return_data, return_size)                                 \
    do                                                                                   \
    {                                                                                    \
        OS_Handle f = os_file_open(OS_AccessFlag_Read, (fp));                            \
        FileProperties f_props = os_properties_from_file(f);                             \
        *return_size = f_props.size;                                                     \
        *return_data = push_array(arena, U8, f_props.size);                              \
        os_file_read(f, rng_1u64(0,f_props.size), *return_data);                         \
    } while (0);

#endif
