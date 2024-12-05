#ifndef RUBIK_CORE_H
#define RUBIK_CORE_H

/////////////////////////////////
// Enum

typedef enum RK_PhysicsKind
{
    RK_PhysicsKind_Static,
    RK_PhysicsKind_Kinematic,
    RK_PhysicsKind_Dynamic,
    RK_PhysicsKind_COUNT,
} RK_PhysicsKind;

typedef enum RK_NodeKind
{
    RK_NodeKind_Null,
    RK_NodeKind_MeshPrimitive,
    RK_NodeKind_MeshJoint,
    RK_NodeKind_MeshGroup,
    RK_NodeKind_MeshRoot,
    RK_NodeKind_CollisionShape3D,
    RK_NodeKind_CharacterBody3D,
    RK_NodeKind_RigidBody3D,
    RK_NodeKind_Camera3D,
    RK_NodeKind_COUNT,
} RK_NodeKind;

typedef enum RK_TransformKind
{
    RK_TransformKind_Invalid,
    RK_TransformKind_Scale,
    RK_TransformKind_Rotation,
    RK_TransformKind_Translation,
    RK_TransformKind_COUNT
} RK_TransformKind;

typedef enum RK_InterpolationMethod
{
    RK_InterpolationMethod_Step,
    RK_InterpolationMethod_Linear,
    RK_InterpolationMethod_Cubicspline,
    RK_InterpolationMethod_COUNT,
} RK_InterpolationMethod;

typedef enum RK_ViewportShadingKind
{
    RK_ViewportShadingKind_Wireframe,
    RK_ViewportShadingKind_Solid,
    RK_ViewportShadingKind_MaterialPreview,
    RK_ViewportShadingKind_COUNT,
} RK_ViewportShadingKind;

typedef enum RK_MeshKind
{
    RK_MeshKind_Box,
    RK_MeshKind_Plane,
    RK_MeshKind_Sphere,
    RK_MeshKind_Cylinder,
    RK_MeshKind_Capsule,
    RK_MeshKind_Model,
} RK_MeshKind;

typedef enum RK_ViewKind
{
    RK_ViewKind_Stats,
    RK_ViewKind_SceneInspector,
    RK_ViewKind_Profiler,
    RK_ViewKind_COUNT,
} RK_ViewKind;

typedef U64 RK_NodeFlags;
# define RK_NodeFlags_Animated         (RK_NodeFlags)(1ull<<0)
# define RK_NodeFlags_AnimatedSkeleton (RK_NodeFlags)(1ull<<1)
# define RK_NodeFlags_Float            (RK_NodeFlags)(1ull<<2)
# define RK_NodeFlags_NavigationRoot   (RK_NodeFlags)(1ull<<3)
# define RK_NodeFlags_Detachable       (RK_NodeFlags)(1ull<<4)

typedef U64 RK_SpecialKeyKind;
#define RK_SpecialKeyKind_GizmosIhat (U64)(0xffffffffffffffffull-0)
#define RK_SpecialKeyKind_GizmosJhat (U64)(0xffffffffffffffffull-1)
#define RK_SpecialKeyKind_GizmosKhat (U64)(0xffffffffffffffffull-2)
#define RK_SpecialKeyKind_Grid       (U64)(0xffffffffffffffffull-3)

/////////////////////////////////
// For magic (EPA)

// typedef struct RK_Vertex2DNode RK_Vertex2DNode;
// struct RK_Vertex2DNode {
//     Vec2F32      v;
//     RK_Vertex2DNode *next;
//     RK_Vertex2DNode *prev;
// };
// 
// typedef struct RK_Contact2D RK_Contact2D;
// struct RK_Contact2D {
//     Vec2F32 normal;
//     F32     length;
// };

/////////////////////////////////
//~ Key

typedef struct RK_Key RK_Key;
struct RK_Key
{
    U64 u64[1];
};

/////////////////////////////////
//~ Resource (alloc/load once)

//- 2D Sprite

typedef struct RK_Tex2D RK_Tex2D;
struct RK_Tex2D 
{
    R_Handle texture;
    U64      w;
    U64      h;
    U64      rc;
};

typedef struct RK_Sprite2D_Frame RK_Sprite2D_Frame;
struct RK_Sprite2D_Frame 
{
    U64       x;
    U64       y;
    U64       w;
    U64       h;
    R_Handle  texture;
    F32       duration;
};

typedef struct RK_Sprite2D_FrameTag RK_Sprite2D_FrameTag;
struct RK_Sprite2D_FrameTag 
{
    String8 name;
    U64     from;
    U64     to;
};

typedef struct RK_SpriteSheet2D RK_SpriteSheet2D;
struct RK_SpriteSheet2D
{
    R_Handle            texture;
    U64                 w;
    U64                 h;

    RK_Sprite2D_FrameTag *tags;
    U64                 tag_count;
    RK_Sprite2D_Frame    *frames;
    U64                 frame_count;
};

//- Mesh

typedef struct RK_MeshJoint RK_MeshJoint;
struct RK_MeshJoint
{
    Mat4x4F32   inverse_bind_matrix;
};

typedef struct RK_MeshSkeletonAnimSpline RK_MeshSkeletonAnimSpline;
struct RK_MeshSkeletonAnimSpline
{
    RK_TransformKind       transform_kind;
    F32                   *timestamps;
    union
    {
    void              *v;
    F32               *floats;
    Vec3F32           *v3s;
    Vec4F32           *v4s;
    } values;
    RK_Key                 target_key;
    U64                   frame_count;
    RK_InterpolationMethod interpolation_method;
};

typedef struct RK_MeshSkeletonAnimation RK_MeshSkeletonAnimation;
struct RK_MeshSkeletonAnimation
{
    String8                  name;
    F32                      duration;
    RK_MeshSkeletonAnimSpline *splines;
    U64                      spline_count;
};

struct RK_ModelNode;
typedef struct RK_ModelNode_HashSlot RK_ModelNode_HashSlot;
struct RK_ModelNode_HashSlot
{
    struct RK_ModelNode *first; 
    struct RK_ModelNode *last; 
};

typedef struct RK_ModelNode RK_ModelNode;
struct RK_ModelNode
{
    RK_ModelNode             *parent;
    RK_ModelNode             *first;
    RK_ModelNode             *last;
    RK_ModelNode             *next;
    RK_ModelNode             *prev;
    RK_ModelNode             *hash_next;
    RK_ModelNode             *hash_prev;
    U64                     children_count;

    String8                 name;
    String8                 path;
    RK_Key                   key;

    // Xform
    Mat4x4F32               xform;

    // Joint
    B32                     is_joint;
    B32                     is_skin; // root joint
    Mat4x4F32               inverse_bind_matrix;

    // Mesh
    B32                     is_mesh_group;
    B32                     is_mesh_primitive;
    R_Handle                vertices;
    R_Handle                indices;
    R_Handle                albedo_tex;

    // Skeleton
    B32                     is_skinned;
    U64                     joint_count;
    RK_ModelNode             **joints;
    RK_ModelNode             *root_joint;

    // Animation
    U64                     anim_count;
    RK_MeshSkeletonAnimation **anims;

    // Hash table
    RK_ModelNode_HashSlot    *hash_table;
    U64                     hash_table_size;

    U64                     rc; // TODO: make use of this
};

/////////////////////////////////
//~ Node Equipment

#define RK_Model RK_ModelNode

typedef struct RK_MeshRoot RK_MeshRoot;
struct RK_MeshRoot
{
    RK_MeshKind kind;
    String8    path;
};

typedef struct RK_MeshGroup RK_MeshGroup;
struct RK_MeshGroup
{
    // Skeleton
    B32       is_skinned;
    struct    RK_Node **joints;
    U32       joint_count;
    struct    RK_Node *root_joint;

    // Artifacts per frame
    Mat4x4F32 *joint_xforms;
};

typedef struct RK_MeshPrimitive RK_MeshPrimitive;
struct RK_MeshPrimitive
{
    R_Handle vertices;
    R_Handle indices;
    R_Handle albedo_tex;
    U64      vertice_count;
    U64      indice_count;
};

typedef struct RK_MeshCacheNode RK_MeshCacheNode;
struct RK_MeshCacheNode
{
    RK_MeshCacheNode *next;
    RK_MeshCacheNode *prev;
    RK_Key           key;
    U64              rc;
    RK_MeshKind      kind;
    void             *v;
};

typedef struct RK_MeshCacheSlot RK_MeshCacheSlot;
struct RK_MeshCacheSlot
{
    RK_MeshCacheNode *first;
    RK_MeshCacheNode *last;
};

typedef struct RK_MeshCacheTable RK_MeshCacheTable;
struct RK_MeshCacheTable
{
    Arena           *arena;
    U64             slot_count;
    RK_MeshCacheSlot *slots;
};

// typedef struct RK_Sprite2D RK_Sprite2D;
// struct RK_Sprite2D {
//     R_Handle texture;
//     U64      x;
//     U64      y;
//     U64      w;
//     U64      h;
//     B32      flip_h;
//     B32      flip_v;
// };
// 
// typedef struct RK_Shape2D RK_Shape2D;
// struct RK_Shape2D {
//     Vec4F32       color;
//     F32           border_thickness;
//     RK_Shape2DKind kind;
// 
//     union {
//         RectShape2D   rect;
//         CircleShape2D circle;
//     };
// };
// 
// typedef struct RK_Animation2D RK_Animation2D;
// struct RK_Animation2D {
//     U64              key;
//     RK_Animation2D    *hash_next;
//     RK_Animation2D    *hash_prev;
// 
//     RK_Sprite2D_Frame *frames;
//     U64              frame_count;
//     U64              duration;
// };
// 
// typedef struct RK_Animation2DHashSlot RK_Animation2DHashSlot;
// struct RK_Animation2DHashSlot {
//     RK_Animation2D *first;
//     RK_Animation2D *last;
// };
// 
// typedef struct RK_AnimatedSprite2D RK_AnimatedSprite2D;
// struct RK_AnimatedSprite2D {
//     B32                   flip_h;
//     B32                   flip_v;
//     F32                   speed_scale;
// 
//     String8               auto_play;
//     U64                   frame_idx;
//     U64                   frame_progress; // ms
// 
//     RK_Animation2DHashSlot *anim2d_hash_table;
//     U64                   anim2d_hash_table_size;
// };
// 
// typedef struct RK_RigidBody2D RK_RigidBody2D;
// struct RK_RigidBody2D {
//     Vec2F32       velocity;
//     Vec2F32       acc;
//     Vec2F32       force;
//     F32           mass;
// 
//     // Body
//     RK_PhysicsKind physics_kind;
//     RK_Shape2DKind body_kind;
//     union {
//         RectShape2D rect;
//         CircleShape2D circle;
//     } shape;
// };

typedef struct RK_Camera3D RK_Camera3D;
struct RK_Camera3D 
{
    F32     fov;
    F32     zn;
    F32     zf;
    B32     hide_cursor;
    B32     lock_cursor;
};

/////////////////////////////////
//~ Node Type

struct RK_Scene;
#define RK_NODE_CUSTOM_UPDATE(name) void name(struct RK_Node *node, struct RK_Scene *scene, OS_EventList os_events, F32 dt_sec)
typedef RK_NODE_CUSTOM_UPDATE(RK_NodeCustomUpdateFunctionType);

#define RK_NODE_CUSTOM_DRAW(name) void name(struct RK_Node *node, void *node_data)
typedef RK_NODE_CUSTOM_DRAW(RK_NodeCustomDrawFunctionType);

typedef struct RK_UpdateFnNode RK_UpdateFnNode;
struct RK_UpdateFnNode
{
    String8                    name; 
    RK_UpdateFnNode             *next;
    RK_UpdateFnNode             *prev;
    RK_NodeCustomUpdateFunctionType *f;
};

typedef struct RK_Node RK_Node;
struct RK_Node
{
    RK_Node                       *parent;
    RK_Node                       *first;
    RK_Node                       *last;
    RK_Node                       *next;
    RK_Node                       *prev;
    RK_Node                       *hash_next;
    RK_Node                       *hash_prev;

    RK_Key                        key;
    String8                       name;
    RK_NodeFlags                  flags;
    RK_NodeKind                   kind;

    U64                           children_count;

    // Transform (TRS)
    Vec3F32                       pos;
    QuatF32                       rot;
    Vec3F32                       scale;
    // Delta
    Vec3F32                       pre_pos_delta;
    QuatF32                       pre_rot_delta;
    Vec3F32                       pre_scale_delta;
    Vec3F32                       pst_pos_delta;
    QuatF32                       pst_rot_delta;
    Vec3F32                       pst_scale_delta;

    // Frame artifacts
    Mat4x4F32                     fixed_xform; // Calculated every frame in DFS order

    // TODO: rotation edit mode: Euler, Quaternion, Basis

    void                          *custom_data;
    RK_UpdateFnNode               *first_update_fn;
    RK_UpdateFnNode               *last_update_fn;
    RK_NodeCustomDrawFunctionType *custom_draw;

    // Animation
    F32                           anim_dt;
    RK_MeshSkeletonAnimation      **skeleton_anims;

    union
    {
        RK_MeshPrimitive mesh_primitive;
        RK_MeshJoint     mesh_joint;
        RK_MeshGroup     mesh_grp;
        RK_MeshRoot      mesh_root; // Serializable
        RK_Camera3D      camera; // Serializable
    } v;
};

typedef struct RK_NodeRec RK_NodeRec;
struct RK_NodeRec
{
    RK_Node *next;
    S32    push_count;
    S32    pop_count;
};

typedef struct RK_BucketSlot RK_BucketSlot;
struct RK_BucketSlot 
{
    RK_Node *first; 
    RK_Node *last; 
};

typedef struct RK_Bucket RK_Bucket;
struct RK_Bucket 
{
    Arena        *arena;
    RK_BucketSlot *node_hash_table;
    U64          node_hash_table_size;
    U64          node_count;
};

typedef struct RK_CameraNode RK_CameraNode;
struct RK_CameraNode
{
    RK_CameraNode *next;
    RK_CameraNode *prev;
    RK_Node       *v;
};

typedef struct RK_Scene RK_Scene;
struct RK_Scene 
{
    RK_Scene               *next;
    Arena                 *arena;
    RK_Bucket              *bucket;
    RK_Node                *root;
    RK_CameraNode          *active_camera;
    RK_CameraNode          *first_camera;
    RK_CameraNode          *last_camera;

    String8               name;
    String8               path;
    RK_MeshCacheTable      mesh_cache_table;

    RK_ViewportShadingKind viewport_shading;
    Vec3F32               global_light;
    R_GeoPolygonKind      polygon_mode;
};

////////////////////////////////
//~ k: Setting Types

typedef struct RK_SettingVal RK_SettingVal;
struct RK_SettingVal
{
    B32 set;
    // TODO(k): we may need to support different number type here later
    S32 s32;
};

/////////////////////////////////
// Global nils

read_only global RK_Key rk_nil_seed = {0};

/////////////////////////////////
// Generated code

#include "generated/rubik.meta.h"

/////////////////////////////////
// Views Types

typedef struct RK_View RK_View;
struct RK_View
{
    Arena *arena;
    void  *custom_data;
};

/////////////////////////////////
// Theme Types 

typedef struct RK_Theme RK_Theme;
struct RK_Theme
{
    Vec4F32 colors[RK_ThemeColor_COUNT];
};

typedef enum RK_FontSlot
{
    RK_FontSlot_Main,
    RK_FontSlot_Code,
    RK_FontSlot_Icons,
    RK_FontSlot_COUNT
} RK_FontSlot;

typedef enum RK_PaletteCode
{
    RK_PaletteCode_Base,
    RK_PaletteCode_MenuBar,
    RK_PaletteCode_Floating,
    RK_PaletteCode_ImplicitButton,
    RK_PaletteCode_PlainButton,
    RK_PaletteCode_PositivePopButton,
    RK_PaletteCode_NegativePopButton,
    RK_PaletteCode_NeutralPopButton,
    RK_PaletteCode_ScrollBarButton,
    RK_PaletteCode_Tab,
    RK_PaletteCode_TabInactive,
    RK_PaletteCode_DropSiteOverlay,
    RK_PaletteCode_COUNT
} RK_PaletteCode;

/////////////////////////////////
// Function Types 

typedef struct RK_FunctionNode RK_FunctionNode;
struct RK_FunctionNode
{
    RK_FunctionNode *next;
    RK_FunctionNode *prev;
    RK_Key          key;
    String8        alias;
    void           *ptr;
};

typedef struct RK_FunctionSlot RK_FunctionSlot;
struct RK_FunctionSlot
{
    RK_FunctionNode *first;
    RK_FunctionNode *last;
};

typedef struct RK_State RK_State;
struct RK_State
{
    Arena             *arena;
    Arena             *frame_arena;
    RK_Scene          *active_scene;

    B32               is_dragging;
    Vec3F32           drag_start_direction;
    OS_Handle         os_wnd;
    RK_MeshCacheTable mesh_cache_table;

    //- UI overlay signal (used for handle user input)
    UI_Signal         sig;

    //- Delta
    U64               dt;
    F32               dt_sec;
    F32               dt_ms;

    //- Bucket
    RK_Bucket         *node_bucket;
    D_Bucket          *bucket_rect;
    D_Bucket          *bucket_geo3d;

    //- Window
    Rng2F32           window_rect;
    Vec2F32           window_dim;
    F32               dpi;
    F32               last_dpi;

    //- key
    RK_Key            hot_key;
    RK_Key            active_key;

    //- Cursor
    Vec2F32           cursor;
    Vec2F32           last_cursor;
    B32               cursor_hidden;

    //- Functions
    RK_FunctionSlot   *function_hash_table;
    U64               function_hash_table_size;

    //- Theme
    RK_Theme          cfg_theme_target;
    RK_Theme          cfg_theme;
    F_Tag             cfg_font_tags[RK_FontSlot_COUNT];

    //- Palette
    UI_Palette        cfg_ui_debug_palettes[RK_PaletteCode_COUNT]; // derivative from theme

    //- Global Settings
    RK_SettingVal     setting_vals[RK_SettingCode_COUNT];

    // Views (UI)
    RK_View           views[RK_ViewKind_COUNT];

    struct {
                      U64 frame_dt_us;
                      U64 cpu_dt_us;
                      U64 gpu_dt_us;
    } debug;

    RK_Scene          *first_to_free_scene;
    RK_Scene          *first_free_scene;

    RK_DeclStackNils;
    RK_DeclStacks;
};

/////////////////////////////////
// Globals

global RK_State *rk_state;

internal void rk_init(OS_Handle os_wnd);

/////////////////////////////////
// Key

internal RK_Key rk_key_from_string(RK_Key seed, String8 string);
internal RK_Key rk_key_merge(RK_Key a, RK_Key b);
internal U64    rk_hash_from_string(U64 seed, String8 string);
internal B32    rk_key_match(RK_Key a, RK_Key b);
internal RK_Key rk_key_zero();

/////////////////////////////////
// Bucket

internal RK_Bucket *rk_bucket_make(Arena *arena, U64 hash_table_size);

/////////////////////////////////
//~ State accessor/mutator

internal void             rk_set_active_key(RK_Key key);
internal RK_Node*         rk_node_from_key(RK_Key key);
internal RK_FunctionNode* rk_function_from_string(String8 string);
internal RK_View*         rk_view_from_kind(RK_ViewKind kind);

/////////////////////////////////
//~ Node build api

internal RK_Node* rk_build_node_from_string(RK_NodeFlags flags, String8 name);
internal RK_Node* rk_build_node_from_stringf(RK_NodeFlags flags, char *fmt, ...);
internal RK_Node* rk_build_node_from_key(RK_NodeFlags flags, RK_Key key);
internal void     rk_node_release();

/////////////////////////////////
//~ Node Type Functions

//- DFS (pre/pos order)
internal RK_NodeRec rk_node_df(RK_Node *n, RK_Node *root, U64 sib_member_off, U64 child_member_off);
#define rk_node_df_pre(node, root) rk_node_df(node, root, OffsetOf(RK_Node, next), OffsetOf(RK_Node, first))
#define rk_node_df_post(node, root) rk_node_df(node, root, OffsetOf(RK_Node, prev), OffsetOf(RK_Node, last))

internal void      rk_local_coord_from_node(RK_Node *node, Vec3F32 *f, Vec3F32 *s, Vec3F32 *u);
internal Mat4x4F32 rk_view_from_node(RK_Node *node);
internal Vec3F32   rk_pos_from_node(RK_Node *node);
internal void      rk_node_delta_commit(RK_Node *node);
internal void      rk_node_push_fn(Arena *arena, RK_Node *n, RK_NodeCustomUpdateFunctionType *fn, String8 name);

/////////////////////////////////
// Node base scripting

RK_NODE_CUSTOM_UPDATE(base_fn);
RK_NODE_CUSTOM_UPDATE(editor_camera_fn);

/////////////////////////////////
//~ Magic

//- Support functions
#define RK_SHAPE_SUPPORT_FN(name) Vec2F32 name(void *shape_data, Vec2F32 direction)
typedef RK_SHAPE_SUPPORT_FN(RK_SHAPE_CUSTOM_SUPPORT_FN);
RK_SHAPE_SUPPORT_FN(RK_SHAPE_RECT_SUPPORT_FN);

//- GJK
// internal B32         gjk(Vec3F32 s1_center, Vec3F32 s2_center, void *s1_data, void *s2_data, RK_SHAPE_CUSTOM_SUPPORT_FN s1_support_fn, RK_SHAPE_CUSTOM_SUPPORT_FN s2_support_fn, Vec3F32 simplex[4]);
internal Vec2F32 triple_product_2f32(Vec2F32 A, Vec2F32 B, Vec2F32 C);

/////////////////////////////////
//~ Colors, Fonts, Config

//- colors
internal Vec4F32    rk_rgba_from_theme_color(RK_ThemeColor color);

//- code -> palette
internal UI_Palette *rk_palette_from_code(RK_PaletteCode code);

//- fonts/sizes
internal F_Tag rk_font_from_slot(RK_FontSlot slot);
internal F32   rk_font_size_from_slot(RK_FontSlot slot);

/////////////////////////////////
//~ UI widget functions

internal void rk_ui_stats(void);
internal void rk_ui_inspector(void);
internal void rk_ui_profiler(void);

/////////////////////////////////
// Frame

internal void rk_frame(RK_Scene *scene, OS_EventList os_events, U64 dt, U64 hot_key);

/////////////////////////////////
//~ Helpers

#define FileReadAll(arena, fp, return_data, return_size)                                 \
    do                                                                                   \
    {                                                                                    \
        OS_Handle f = os_file_open(OS_AccessFlag_Read, (fp));                            \
        FileProperties f_props = os_properties_from_file(f);                             \
        *return_size = f_props.size;                                                     \
        *return_data = push_array(arena, U8, f_props.size);                              \
        os_file_read(f, rng_1u64(0,f_props.size), *return_data);                         \
    } while (0);
internal void rk_trs_from_matrix(Mat4x4F32 *m, Vec3F32 *trans, QuatF32 *rot, Vec3F32 *scale);

/////////////////////////////////
// Scene creation and destruction

internal RK_Scene* rk_scene_alloc();
internal void      rk_scene_release(RK_Scene *s);

#endif
