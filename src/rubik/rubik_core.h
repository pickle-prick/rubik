#ifndef RUBIK_CORE_H
#define RUBIK_CORE_H

/////////////////////////////////
//~ Enum

typedef enum RK_PhysicsKind
{
    RK_PhysicsKind_Static,
    RK_PhysicsKind_Kinematic,
    RK_PhysicsKind_Dynamic,
    RK_PhysicsKind_COUNT,
} RK_PhysicsKind;

typedef enum RK_ProjectionKind
{
    RK_ProjectionKind_Perspective,
    RK_ProjectionKind_Orthographic,
    RK_ProjectionKind_COUNT,
} RK_ProjectionKind;

typedef enum RK_TransformKind
{
    RK_TransformKind_Scale,
    RK_TransformKind_Rotation,
    RK_TransformKind_Translation,
    RK_TransformKind_COUNT
} RK_TransformKind;

/////////////////////////////////
//- Animation

typedef enum RK_InterpolationKind
{
    RK_InterpolationKind_Step,
    RK_InterpolationKind_Linear,
    RK_InterpolationKind_Cubic,
    RK_InterpolationKind_COUNT,
} RK_InterpolationKind;

typedef enum RK_TrackTargetKind 
{
    RK_TrackTargetKind_Position3D,
    RK_TrackTargetKind_Rotation3D,
    RK_TrackTargetKind_Scale3D,
    RK_TrackTargetKind_MorphWeight3D, // Blend shape
} RK_TrackTargetKind;

typedef enum RK_ViewportShadingKind
{
    RK_ViewportShadingKind_Wireframe,
    RK_ViewportShadingKind_Solid,
    RK_ViewportShadingKind_Material,
    RK_ViewportShadingKind_COUNT,
} RK_ViewportShadingKind;

// Mesh
typedef enum RK_MeshSourceKind
{
    RK_MeshSourceKind_Invalid,
    RK_MeshSourceKind_BoxPrimitive,
    RK_MeshSourceKind_PlanePrimitive,
    RK_MeshSourceKind_SpherePrimitive,
    RK_MeshSourceKind_CylinderPrimitive,
    RK_MeshSourceKind_CapsulePrimitive,
    RK_MeshSourceKind_Extern,
    RK_MeshSourceKind_COUNT,
} RK_MeshSourceKind;

// UI
typedef enum RK_ViewKind
{
    RK_ViewKind_Stats,
    RK_ViewKind_SceneInspector,
    RK_ViewKind_Profiler,
    RK_ViewKind_COUNT,
} RK_ViewKind;

// Anything can be loadedfromfile/computed/uploadedtogpu once and reuse
typedef enum RK_ResourceKind
{
    RK_ResourceKind_Skin,
    RK_ResourceKind_Mesh,
    RK_ResourceKind_PackedScene,
    RK_ResourceKind_Material,
    RK_ResourceKind_Animation,
    RK_ResourceKind_Texture2D,
    // RK_ResourceKind_AudioStream,
    RK_ResourceKind_COUNT,
} RK_ResourceKind;

typedef U64 RK_NodeFlags;
#define RK_NodeFlag_NavigationRoot       (RK_NodeFlags)(1ull<<0)
#define RK_NodeFlag_Float                (RK_NodeFlags)(1ull<<1)

typedef U64 RK_NodeTypeFlags;
#define RK_NodeTypeFlag_Node2D           (RK_NodeTypeFlags)(1ull<<0)
#define RK_NodeTypeFlag_Node3D           (RK_NodeTypeFlags)(1ull<<1)
#define RK_NodeTypeFlag_Camera3D         (RK_NodeTypeFlags)(1ull<<2)
// TODO: do we need this flag?
// #define RK_NodeTypeFlag_Joint3D          (RK_NodeTypeFlags)(1ull<<3)
#define RK_NodeTypeFlag_MeshInstance3D   (RK_NodeTypeFlags)(1ull<<4)
#define RK_NodeTypeFlag_SceneInstance    (RK_NodeTypeFlags)(1ull<<5) /* instance from another scene */
#define RK_NodeTypeFlag_AnimationPlayer  (RK_NodeTypeFlags)(1ull<<6) /* instance from another scene */

/////////////////////////////////
//~ Key

typedef struct RK_Key RK_Key;
struct RK_Key
{
    U64 u64[1];
};

/////////////////////////////////
//~ Handle Type

typedef union RK_Handle RK_Handle;
union RK_Handle
{
    U64 u64[2];
    U32 u32[4];
    U16 u16[8];
};


/////////////////////////////////
//~ Basic types

typedef struct RK_Transform2D RK_Transform2D;
struct RK_Transform2D
{
    Vec2F32 position;
    F32     rotation;
    Vec2F32 scale;
    F32     skew;
};

typedef struct RK_Transform3D RK_Transform3D;
struct RK_Transform3D
{
    Vec3F32 position;
    QuatF32 rotation;
    Vec3F32 scale;
};

/////////////////////////////////
//- Mesh basic types

typedef struct RK_Bind RK_Bind;
struct RK_Bind
{
    RK_Key         joint;
    Mat4x4F32      inverse_bind_matrix;
};

typedef struct RK_MorphTarget RK_MorphTarget;
struct RK_MorphTarget
{
    R_Handle       vertices;
};

// Animation Track
#define RK_MAX_MORPH_TARGET_COUNT 2
typedef struct RK_TrackFrame RK_TrackFrame;
struct RK_TrackFrame
{
    // Allocation link
    RK_TrackFrame *next;

    // B-tree link
    RK_TrackFrame *parent;
    RK_TrackFrame *left;
    RK_TrackFrame *right;
    U64 btree_height;
    U64 btree_size;

    F32 ts_sec; // frame time in seconds
    union
    {
        Vec3F32 position3d;
        Vec4F32 rotation3d;
        Vec3F32 scale3d;
        F32 morph_weights3d[RK_MAX_MORPH_TARGET_COUNT];
    } v;
};

typedef struct RK_Track RK_Track;
struct RK_Track
{
    RK_Track *next;
    RK_Track *prev;
    RK_Key target_key;
    RK_TrackTargetKind target_kind;
    RK_InterpolationKind interpolation;
    RK_TrackFrame *frame_btree_root;
    U64 frame_count;
    F32 duration_sec;
};

typedef struct RK_AnimationPlayback RK_AnimationPlayback;
struct RK_AnimationPlayback
{
    RK_Handle curr;
    RK_Handle next;
    B32 loop;
    F32 speed_scale;

    F32 pos;
    F32 start_time;
    F32 end_time;
};

/////////////////////////////////
//~ Resource

typedef struct RK_Resource RK_Resource;
struct RK_Resource;

//- Mesh Resources

// PBR material
typedef struct RK_Material RK_Material;
struct RK_Material
{
    String8 name;
    U8      name_buffer[300];

    // pbr
    RK_Handle base_clr_tex;
    Vec4F32 base_clr_factor;
    RK_Handle metallic_roughness_tex;
    F32 metallic_factor;
    F32 roughness_factor;

    // normal texture
    RK_Handle normal_tex;
    F32 normal_scale;

    // occlusion texture
    RK_Handle occlusion_tex;
    F32 occlusion_strength;

    // emissive texture
    RK_Handle emissive_tex;
    Vec3F32 emissive_factor;
};

typedef struct RK_Skin RK_Skin;
struct RK_Skin
{
    RK_Bind     binds[300];
    U64         bind_count;
};

typedef struct RK_Mesh RK_Mesh;
struct RK_Mesh
{
    R_Handle          vertices;
    R_Handle          indices;
    R_GeoTopologyKind topology; // rendering mode
    U64               indice_count;
    U64               vertex_count;

    // TODO
    RK_Handle         material;

    // TODO: make use of morph targets
    RK_MorphTarget    morph_targets[RK_MAX_MORPH_TARGET_COUNT];
    U64               morph_target_count;
    F32               morph_weights[RK_MAX_MORPH_TARGET_COUNT];

    RK_MeshSourceKind src_kind;

    union
    {
        struct
        {
            U64 subdivide_w;
            U64 subdivide_h;
            U64 subdivide_d;
            Vec3F32 size;
        }
        box_primitive;
        struct
        {
            U64 subdivide_w;
            U64 subdivide_d;
            Vec2F32 size;
        }
        plane_primitive;
        struct
        {
            F32 radius;
            F32 height;
            U64 radial_segments;
            U64 rings;
            B32 is_hemisphere;
        }
        sphere_primitive;
        struct
        {
            F32 radius;
            F32 height;
            U64 radial_segments;
            U64 rings;
            B32 cap_top;
            B32 cap_bottom;
        }
        cylinder_primitive;
        struct
        {
            F32 radius;
            F32 height;
            U64 radial_segments;
            U64 rings;
        }
        capsule_primitive;
        struct
        {

            String8 path;
            U8 path_buffer[300];
        }
        ext;
    } src;
};

typedef struct RK_Animation RK_Animation;
struct RK_Animation
{
    RK_Animation *next;
    String8 name;
    U8 name_buffer[300];
    F32 duration_sec;

    // TODO(k): not sure what data structure to use here (binary tree or array?)
    RK_Track *first_track;
    RK_Track *last_track;
    U64 track_count;
};

typedef struct RK_Texture2D RK_Texture2D;
struct RK_Texture2D
{
    R_Tex2DSampleKind sample_kind;
    R_Handle          tex;

    // for serialize/deserialize
    String8           path;
    U8                path_buffer[300];
};

struct RK_Node;
struct RK_ResourceBucket;
typedef struct RK_PackedScene RK_PackedScene;
struct RK_PackedScene
{
    struct RK_Node           *root;

    struct RK_ResourceBucket *res_bucket;

    // for serialize/deserialize
    String8                  path;
    U8                       path_buffer[300];
};

struct RK_ResourceBucket;
typedef struct RK_Resource RK_Resource;
struct RK_Resource
{
    // Allocation link
    RK_Resource              *next;
    U64                      generation;

    // Hash link
    RK_Resource              *hash_next;
    RK_Resource              *hash_prev;

    RK_ResourceKind          kind;
    RK_Key                   key;
    // TODO(k): maybe it's a bad idea to individually free resource
    U64                      rc;
    B32                      auto_free; /* Release after rc hit 0 */

    struct RK_ResourceBucket *owner_bucket;

    union
    {
        RK_Material    material;
        RK_Skin        skin;
        RK_Mesh        mesh;
        RK_Animation   animation;
        RK_Texture2D   tex2d;
        RK_PackedScene packed_scene;
    } v;
};

/////////////////////////////////
//~ Node Equipment

typedef struct RK_Node2D RK_Node2D;
struct RK_Node2D
{
    // allocation link
    RK_Node2D      *next;
    RK_Transform2D transform;
};

typedef struct RK_Node3D RK_Node3D;
struct RK_Node3D
{
    // allocation link
    RK_Node3D      *next;
    RK_Transform3D transform;
};

typedef struct RK_MeshInstance3D RK_MeshInstance3D;
struct RK_MeshInstance3D
{
    // allocation link
    RK_MeshInstance3D *next;

    RK_Handle         mesh;
    RK_Handle         skin;
    RK_Key            skin_seed;
    RK_Handle         material_override;
};

#define RK_MAX_ANIMATION_PER_INST_COUNT 10
typedef struct RK_AnimationPlayer RK_AnimationPlayer;
struct RK_AnimationPlayer
{
    RK_AnimationPlayer *next;
    RK_Handle animations[RK_MAX_ANIMATION_PER_INST_COUNT];
    U64 animation_count;

    RK_Key target_seed;
    RK_AnimationPlayback playback;

    // TODO(k): support animation blending
};

typedef struct RK_SceneInstance RK_SceneInstance;
struct RK_SceneInstance
{
    RK_SceneInstance *next;
    RK_Handle        *packed_scene;
};

typedef struct RK_Camera3D RK_Camera3D;
struct RK_Camera3D
{
    RK_Camera3D            *next;
    RK_ProjectionKind      projection;
    RK_ViewportShadingKind viewport_shading;
    R_GeoPolygonKind       polygon_mode;
    B32                    hide_cursor;
    B32                    lock_cursor;
    B32                    show_grid;
    B32                    show_gizmos;
    B32                    is_active;

    union
    {
        // Perspective
        struct
        {
            F32 zn;
            F32 zf;
            F32 fov;
        } perspective;

        // Orthographic
        struct
        {
            F32 zn;
            F32 zf;
            F32 left;
            F32 right;
            F32 bottom;
            F32 top;
        } orthographic;
    };
};

/////////////////////////////////
//~ Node Type

struct RK_Node;
struct RK_Scene;
#define RK_NODE_CUSTOM_UPDATE(name) void name(struct RK_Node *node, struct RK_Scene *scene, OS_EventList os_events, F32 dt_sec)
typedef RK_NODE_CUSTOM_UPDATE(RK_NodeCustomUpdateFunctionType);

#define RK_NODE_CUSTOM_DRAW(name) void name(struct RK_Node *node, void *node_data)
typedef RK_NODE_CUSTOM_DRAW(RK_NodeCustomDrawFunctionType);

typedef struct RK_UpdateFnNode RK_UpdateFnNode;
struct RK_UpdateFnNode
{
    RK_UpdateFnNode                 *next;
    RK_UpdateFnNode                 *prev;
    RK_NodeCustomUpdateFunctionType *f;
};

struct RK_NodeBucket;
typedef struct RK_Node RK_Node;
struct RK_Node
{
    // Instance
    RK_Handle                     instance; // if this node came from a PackedScene
    B32                           is_foreign; // if this is a subnode of a instance node

    //~ Node tree links
    RK_Node                       *parent;
    RK_Node                       *first;
    RK_Node                       *last;
    RK_Node                       *next;
    RK_Node                       *prev;

    //~ Bucket links
    RK_Node                       *hash_next;
    RK_Node                       *hash_prev;

    struct RK_NodeBucket          *owner_bucket;

    U64                           generation;
    RK_Key                        key;
    // NOTE(k): we need to reuse RK_Node, so the string has to be fixed sized
    String8                       name;
    U8                            name_buffer[300];
    RK_NodeFlags                  flags;
    RK_NodeTypeFlags              type_flags;

    U64                           children_count;

    //~ Transform (3d coord behind the scene regradless 2D or 3D)
    Vec3F32                       position;
    Vec3F32                       scale;
    QuatF32                       rotation;

    //~ Equipments
    RK_Node2D                     *node2d;
    RK_Node3D                     *node3d;
    RK_Camera3D                   *camera3d;
    RK_MeshInstance3D             *mesh_inst3d;
    RK_AnimationPlayer            *animation_player;

    //~ Custom update/draw functions
    void                          *custom_data;
    RK_UpdateFnNode               *first_update_fn;
    RK_UpdateFnNode               *last_update_fn;
    RK_NodeCustomDrawFunctionType *custom_draw;

    //~ Artifacts
    Mat4x4F32                     fixed_xform;
};

typedef struct RK_NodeRec RK_NodeRec;
struct RK_NodeRec
{
    RK_Node *next;
    S32     push_count;
    S32     pop_count;
};

/////////////////////////////////
// Bucket

typedef struct RK_NodeBucketSlot RK_NodeBucketSlot;
struct RK_NodeBucketSlot 
{
    RK_Node *first;
    RK_Node *last;
};

typedef struct RK_NodeBucket RK_NodeBucket;
struct RK_NodeBucket 
{
    Arena              *arena_ref;

    RK_NodeBucketSlot  *hash_table;
    U64                hash_table_size;
    U64                node_count;

    //~ Resource Management
    RK_Node            *first_free_node;
    RK_UpdateFnNode    *first_free_update_fn_node;

    //- Equipment
    RK_Node2D          *first_free_node2d;
    RK_Node3D          *first_free_node3d;
    RK_Camera3D        *first_free_camera3d;
    RK_MeshInstance3D  *first_free_mesh_inst3d;
    RK_AnimationPlayer *first_free_animation_player;
};

typedef struct RK_ResourceBucketSlot RK_ResourceBucketSlot;
struct RK_ResourceBucketSlot
{
    RK_Resource *first;
    RK_Resource *last;
};

typedef struct RK_ResourceBucket RK_ResourceBucket;
struct RK_ResourceBucket
{
    Arena                 *arena_ref;
    RK_ResourceBucketSlot *hash_table;
    U64                   hash_table_size;
    U64                   resource_count;

    // Resource management
    RK_Resource           *first_free_res;
    RK_Animation          *first_free_animation;
    RK_Track              *first_free_track;
    RK_TrackFrame         *first_free_track_frame;
};

/////////////////////////////////
// Scene

typedef struct RK_Scene RK_Scene;
struct RK_Scene
{
    // Allocation link
    RK_Scene          *next;
    // Storage
    Arena             *arena;
    RK_NodeBucket     *node_bucket;
    RK_NodeBucket     *res_node_bucket;
    RK_ResourceBucket *res_bucket;

    RK_Handle         root;
    RK_Handle         active_camera;

    RK_Key            hot_key;
    RK_Key            active_key;

    String8           name;
    String8           save_path;
};

////////////////////////////////
//~ k: Setting Types

typedef struct RK_SettingVal RK_SettingVal;
struct RK_SettingVal
{
    B32 set;
    // TODO(k): we may want to support different number type here later
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

typedef struct RK_SceneTemplate RK_SceneTemplate;
struct RK_SceneTemplate
{
    String8 name;
    RK_Scene*(*fn)(void);
};

typedef struct RK_State RK_State;
struct RK_State
{
    Arena                 *arena;
    RK_Scene              *active_scene;
    Arena                 *frame_arenas[2];
    U64                   frame_counter;

    //- Interaction
    OS_Handle             os_wnd;

    //- UI overlay signal (used for handle user input)
    UI_Signal             sig;

    //- Delta
    U64                   dt_us;
    F32                   dt_sec;
    F32                   dt_ms;

    //- Global storage buckets
    RK_NodeBucket         *node_bucket;
    RK_NodeBucket         *res_node_bucket;
    RK_ResourceBucket     *res_bucket;

    //- Drawing buckets
    D_Bucket              *bucket_rect;
    D_Bucket              *bucket_geo3d;

    //- Window
    Rng2F32               window_rect;
    Vec2F32               window_dim;
    F32                   dpi;
    F32                   last_dpi;
    B32                   window_should_close;

    //- Cursor
    Vec2F32               cursor;
    Vec2F32               last_cursor;
    B32                   cursor_hidden;

    //- Functions
    RK_FunctionSlot       *function_hash_table;
    U64                   function_hash_table_size;

    //- Scene templates
    RK_SceneTemplate      *templates;
    U64                   template_count;

    //- Theme
    RK_Theme              cfg_theme_target;
    RK_Theme              cfg_theme;
    F_Tag                 cfg_font_tags[RK_FontSlot_COUNT];

    //- Palette
    UI_Palette            cfg_ui_debug_palettes[RK_PaletteCode_COUNT]; // derivative from theme

    //- Global Settings
    RK_SettingVal         setting_vals[RK_SettingCode_COUNT];

    // Views (UI)
    RK_View               views[RK_ViewKind_COUNT];

    struct
    {
    U64                   frame_dt_us;
                          U64 cpu_dt_us;
                          U64 gpu_dt_us;
    } debug;

    RK_Scene              *first_to_free_scene;

    RK_DeclStackNils;
    RK_DeclStacks;
};

/////////////////////////////////
// Globals

global RK_State *rk_state;

/////////////////////////////////
// Basic Type Functions

internal U64     rk_hash_from_string(U64 seed, String8 string);
internal String8 rk_hash_part_from_key_string(String8 string);
internal String8 rk_display_part_from_key_string(String8 string);

/////////////////////////////////
// Key

internal RK_Key rk_key_from_string(RK_Key seed, String8 string);
internal RK_Key rk_key_from_stringf(RK_Key seed, char* fmt, ...);
internal RK_Key rk_key_merge(RK_Key a, RK_Key b);
internal B32    rk_key_match(RK_Key a, RK_Key b);
internal RK_Key rk_key_make(U64 v);
internal RK_Key rk_key_zero();

/////////////////////////////////
// Handle

internal RK_Handle rk_handle_zero();
internal B32       rk_handle_match(RK_Handle a, RK_Handle b);
#define rk_handle_is_zero(h) rk_handle_match((h), rk_handle_zero())

/////////////////////////////////
// Bucket

internal RK_NodeBucket*     rk_node_bucket_make(Arena *arena, U64 max_nodes);
internal RK_ResourceBucket* rk_res_bucket_make(Arena *arena, U64 max_resources);
internal void               rk_res_bucket_release(RK_ResourceBucket *res_bucket);
internal RK_Node*           rk_node_from_key(RK_Key key);

/////////////////////////////////
//~ Resource Type Functions

internal RK_Key       rk_res_key_from_string(RK_ResourceKind kind, RK_Key seed, String8 string);
internal RK_Key       rk_res_key_from_stringf(RK_ResourceKind kind, RK_Key seed, char* fmt, ...);
internal RK_Handle    rk_resh_alloc(RK_Key key, RK_ResourceKind kind, B32 acquire);
internal void         rk_res_release(RK_Resource *ses);
internal RK_Handle    rk_resh_from_key(RK_Key key);
internal RK_Handle    rk_resh_acquire_from_key(RK_Key key);
internal void         rk_resh_acquire(RK_Handle handle);
internal void         rk_resh_return(RK_Handle handle);
internal RK_Resource* rk_res_from_handle(RK_Handle handle);
internal void*        rk_res_data_from_handle(RK_Handle handle);
internal RK_Handle    rk_handle_from_res(RK_Resource *res);

/////////////////////////////////
//- Resourcea Building Helpers (subresource management etc...)

internal RK_Animation*  rk_animation_alloc();
internal RK_Track*      rk_track_alloc();
internal RK_TrackFrame* rk_track_frame_alloc();

/////////////////////////////////
//~ State accessor/mutator

internal void             rk_init(OS_Handle os_wnd);
internal Arena*           rk_frame_arena();
internal RK_FunctionNode* rk_function_from_string(String8 string);
internal RK_View*         rk_view_from_kind(RK_ViewKind kind);

/////////////////////////////////
//~ Node build api

internal RK_Node* rk_node_alloc(RK_NodeTypeFlags type_flags);
internal void     rk_node_release(RK_Node *node);
internal RK_Node* rk_build_node_from_string(RK_NodeTypeFlags type_flags, RK_NodeFlags flags, String8 name);
internal RK_Node* rk_build_node_from_stringf(RK_NodeTypeFlags type_flags, RK_NodeFlags flags, char *fmt, ...);
internal RK_Node* rk_build_node_from_key(RK_NodeTypeFlags type_flags, RK_NodeFlags flags, RK_Key key);
internal void     rk_node_equip_display_string(RK_Node* node, String8 string);

#define rk_build_node2d_from_string(tf,f,s)              rk_build_node_from_string(((tf)|RK_NodeTypeFlag_Node2D), (f), s)
#define rk_build_node2d_from_stringf(tf,f,...)           rk_build_node_from_stringf(((tf)|RK_NodeTypeFlag_Node2D), (f), __VA_ARGS__)
#define rk_build_node3d_from_string(tf,f,s)              rk_build_node_from_string(((tf)|RK_NodeTypeFlag_Node3D), (f), s)
#define rk_build_node3d_from_stringf(tf,f,...)           rk_build_node_from_stringf(((tf)|RK_NodeTypeFlag_Node3D), (f), __VA_ARGS__)
#define rk_build_camera3d_from_string(tf,f,s)            rk_build_node3d_from_string(((tf)|RK_NodeTypeFlag_Camera3D), (f), s)
#define rk_build_camera3d_from_stringf(tf,f,...)         rk_build_node3d_from_stringf(((tf)|RK_NodeTypeFlag_Camera3D), (f), __VA_ARGS__)
#define rk_build_mesh_inst3d_from_string(tf,f,s)         rk_build_node3d_from_string(((tf)|RK_NodeTypeFlag_MeshInstance3D), (f), s)
#define rk_build_mesh_inst3d_from_stringf(tf,f,...)      rk_build_node3d_from_stringf(((tf)|RK_NodeTypeFlag_MeshInstance3D), (f), __VA_ARGS__)
#define rk_build_animation_player_from_string(tf,f,s)    rk_build_node3d_from_string(((tf)|RK_NodeTypeFlag_AnimationPlayer), (f), s)
#define rk_build_animation_player_from_stringf(tf,f,...) rk_build_node3d_from_stringf(((tf)|RK_NodeTypeFlag_AnimationPlayer), (f), __VA_ARGS__)

/////////////////////////////////
//~ Node Type Functions

//- DFS (pre/pos order)
internal RK_NodeRec rk_node_df(RK_Node *n, RK_Node *root, U64 sib_member_off, U64 child_member_off);
#define rk_node_df_pre(node, root) rk_node_df(node, root, OffsetOf(RK_Node, next), OffsetOf(RK_Node, first))
#define rk_node_df_post(node, root) rk_node_df(node, root, OffsetOf(RK_Node, prev), OffsetOf(RK_Node, last))

internal void      rk_node_push_fn(RK_Node *n, RK_NodeCustomUpdateFunctionType *fn);
internal RK_Handle rk_handle_from_node(RK_Node *n);
internal RK_Node*  rk_node_from_handle(RK_Handle handle);

/////////////////////////////////
// Node scripting

RK_NODE_CUSTOM_UPDATE(base_fn);

/////////////////////////////////
//~ Magic

//- Support functions
#define RK_SHAPE_SUPPORT_FN(name) Vec2F32 name(void *shape_data, Vec2F32 direction)
typedef RK_SHAPE_SUPPORT_FN(RK_SHAPE_CUSTOM_SUPPORT_FN);
RK_SHAPE_SUPPORT_FN(RK_SHAPE_RECT_SUPPORT_FN);

//- GJK
// internal B32 gjk(Vec3F32 s1_center, Vec3F32 s2_center, void *s1_data, void *s2_data, RK_SHAPE_CUSTOM_SUPPORT_FN s1_support_fn, RK_SHAPE_CUSTOM_SUPPORT_FN s2_support_fn, Vec3F32 simplex[4]);
internal Vec2F32 triple_product_2f32(Vec2F32 A, Vec2F32 B, Vec2F32 C);

/////////////////////////////////
//~ Colors, Fonts, Config

//- colors
internal Vec4F32 rk_rgba_from_theme_color(RK_ThemeColor color);

//- code -> palette
internal UI_Palette *rk_palette_from_code(RK_PaletteCode code);

//- fonts/sizes
internal F_Tag rk_font_from_slot(RK_FontSlot slot);
internal F32   rk_font_size_from_slot(RK_FontSlot slot);

/////////////////////////////////
//~ UI widget

internal void rk_ui_stats(void);
internal void rk_ui_inspector(void);
internal void rk_ui_profiler(void);

/////////////////////////////////
//~ Frame

internal void      rk_ui_draw(void);
internal D_Bucket* rk_frame(RK_Scene *scene, OS_EventList os_events, U64 dt, U64 hot_key);

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
internal void rk_ijk_from_matrix(Mat4x4F32 m, Vec3F32 *i, Vec3F32 *j, Vec3F32 *k);

/////////////////////////////////
//~ Enum to string

internal String8 rk_string_from_projection_kind(RK_ProjectionKind kind);
internal String8 rk_string_from_viewport_shading_kind(RK_ViewportShadingKind kind);
internal String8 rk_string_from_polygon_kind(RK_ViewportShadingKind kind);

/////////////////////////////////
// Scene Type Functions

internal RK_Scene* rk_scene_alloc(String8 namee, String8 save_path);
internal void      rk_scene_release(RK_Scene *s);
internal void      rk_scene_active_camera_set(RK_Scene *s, RK_Node *camera_node);
internal void      rk_scene_hot_key_set(RK_Scene *s, RK_Key key, B32 only_navigation_root);
internal void      rk_scene_active_key_set(RK_Scene *s, RK_Key key, B32 only_navigation_root);

#endif
