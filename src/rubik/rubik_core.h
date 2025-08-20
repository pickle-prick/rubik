#ifndef RUBIK_CORE_H
#define RUBIK_CORE_H

/////////////////////////////////////////////////////////////////////////////////////////
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

typedef enum RK_Gizmo3DMode
{
  RK_Gizmo3DMode_Translate,
  RK_Gizmo3DMode_Rotation,
  RK_Gizmo3DMode_Scale,
  RK_Gizmo3DMode_COUNT,
} RK_Gizmo3DMode;

typedef enum RK_Sprite2DAnchorKind
{
  RK_Sprite2DAnchorKind_TopLeft,
  RK_Sprite2DAnchorKind_TopRight,
  RK_Sprite2DAnchorKind_BottomLeft,
  RK_Sprite2DAnchorKind_BottomRight,
  RK_Sprite2DAnchorKind_Center,
  RK_Sprite2DAnchorKind_COUNT,
} RK_Sprite2DAnchorKind;

typedef enum RK_Sprite2DShapeKind
{
  RK_Sprite2DShapeKind_Rect,
  RK_Sprite2DShapeKind_Circle,
  RK_Sprite2DShapeKind_Triangle,
  RK_Sprite2DShapeKind_COUNT,
} RK_Sprite2DShapeKind;

typedef enum RK_Collider2DShapeKind
{
  RK_Collider2DShapeKind_Rect,
  RK_Collider2DShapeKind_Circle,
  RK_Collider2DShapeKind_Edge,
  RK_Collider2DShapeKind_Polygon,
  RK_Collider2DShapeKind_COUNT,
} RK_Collider2DShapeKind;

typedef enum RK_Rigidbody2DBodyKind
{
  RK_Rigidbody2DBodyKind_Dynamic,
  RK_Rigidbody2DBodyKind_Kinematic,
  RK_Rigidbody2DBodyKind_Static,
  RK_Rigidbody2DBodyKind_COUNT,
} RK_Rigidbody2DBodyKind;

/////////////////////////////////////////////////////////////////////////////////////////
//- Render bucket

typedef enum RK_GeoBucketKind
{
  RK_GeoBucketKind_Geo2D,
  RK_GeoBucketKind_Geo3D_Back,
  RK_GeoBucketKind_Geo3D_Front,
  RK_GeoBucketKind_COUNT,
} RK_GeoBucketKind;

/////////////////////////////////////////////////////////////////////////////////////////
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
  RK_MeshSourceKind_TorusPrimitive,
  RK_MeshSourceKind_Extern,
  RK_MeshSourceKind_COUNT,
} RK_MeshSourceKind;

// UI
typedef enum RK_ViewKind
{
  RK_ViewKind_Stats,
  RK_ViewKind_SceneInspector,
  RK_ViewKind_Profiler,
  RK_ViewKind_Terminal,
  RK_ViewKind_COUNT,
} RK_ViewKind;

typedef struct RK_Image RK_Image;
struct RK_Image
{
  U8 *data;
  S32 x;
  S32 y;
  S32 n;

  // ?
  String8 path;
  String8 filename;
};

// Anything can be loadedfromfile/computed/uploadedtogpu once and reuse
typedef enum RK_ResourceKind
{
  RK_ResourceKind_Invalid = -1,
  RK_ResourceKind_Skin,
  RK_ResourceKind_Mesh,
  RK_ResourceKind_PackedScene,
  RK_ResourceKind_Material,
  RK_ResourceKind_Animation,
  RK_ResourceKind_Texture2D,
  RK_ResourceKind_SpriteSheet,
  RK_ResourceKind_TileSet,
  // RK_ResourceKind_AudioStream,
  RK_ResourceKind_COUNT,
} RK_ResourceKind;

typedef enum RK_ResourceSrcKind
{
  RK_ResourceSrcKind_Invalid,
  // internal resource, created using internal description, no external resource is needed
  RK_ResourceSrcKind_Sub,
  // load directly from a file (e.g., .png, .mat, .glTF)
  RK_ResourceSrcKind_External,
  // came from a packed/bundled source (e.g., glTF)
  RK_ResourceSrcKind_Bundled,
} RK_ResourceSrcKind;

////////////////////////////////
// Drag/Drop Types

typedef enum RK_DragDropState
{
  RK_DragDropState_Null,
  RK_DragDropState_Dragging,
  RK_DragDropState_Dropping,
  RK_DragDropState_COUNT
}
RK_DragDropState;

// NOTE(k): we have 63 flags to use, don't exceed that 
typedef U64 RK_NodeFlags;
#define RK_NodeFlag_NavigationRoot       (RK_NodeFlags)(1ull<<0)
#define RK_NodeFlag_Float                (RK_NodeFlags)(1ull<<1)
#define RK_NodeFlag_Transient            (RK_NodeFlags)(1ull<<2) /* NOTE(k): don't use it recursily */

typedef U64                              RK_NodeTypeFlags;
#define RK_NodeTypeFlag_Node2D           (RK_NodeTypeFlags)(1ull<<0)
#define RK_NodeTypeFlag_Node3D           (RK_NodeTypeFlags)(1ull<<1)
#define RK_NodeTypeFlag_Camera3D         (RK_NodeTypeFlags)(1ull<<2)
// #define RK_NodeTypeFlag_Joint3D          (RK_NodeTypeFlags)(1ull<<3)
#define RK_NodeTypeFlag_MeshInstance3D   (RK_NodeTypeFlags)(1ull<<4)
#define RK_NodeTypeFlag_SceneInstance    (RK_NodeTypeFlags)(1ull<<5) /* instance from another scene */
#define RK_NodeTypeFlag_AnimationPlayer  (RK_NodeTypeFlags)(1ull<<6)
#define RK_NodeTypeFlag_DirectionalLight (RK_NodeTypeFlags)(1ull<<7)
#define RK_NodeTypeFlag_PointLight       (RK_NodeTypeFlags)(1ull<<8)
#define RK_NodeTypeFlag_SpotLight        (RK_NodeTypeFlags)(1ull<<9)
#define RK_NodeTypeFlag_Sprite2D         (RK_NodeTypeFlags)(1ull<<10)
#define RK_NodeTypeFlag_Collider2D       (RK_NodeTypeFlags)(1ull<<11)
// TODO(XXX): Rigidbody2D
#define RK_NodeTypeFlag_AnimatedSprite2D (RK_NodeTypeFlags)(1ull<<12)
#define RK_NodeTypeFlag_TileMapLayer     (RK_NodeTypeFlags)(1ull<<13)
#define RK_NodeTypeFlag_TileMap          (RK_NodeTypeFlags)(1ull<<14)
#define RK_NodeTypeFlag_Particle3D       (RK_NodeTypeFlags)(1ull<<15)
#define RK_NodeTypeFlag_HookSpring3D     (RK_NodeTypeFlags)(1ull<<16)
#define RK_NodeTypeFlag_Constraint3D     (RK_NodeTypeFlags)(1ull<<17)
#define RK_NodeTypeFlag_Rigidbody3D      (RK_NodeTypeFlags)(1ull<<18)

#define RK_NodeTypeFlag_Drawable (RK_NodeTypeFlag_MeshInstance3D | RK_NodeTypeFlag_Sprite2D | RK_NodeTypeFlag_AnimatedSprite2D)

/////////////////////////////////////////////////////////////////////////////////////////
//~ Key

typedef struct RK_Key RK_Key;
struct RK_Key
{
  U64 u64[2];
};

/////////////////////////////////////////////////////////////////////////////////////////
//~ Handle Type

typedef union RK_Handle RK_Handle;
union RK_Handle
{
  // 0 ptr
  // 1 generation,
  // 2 key_0,
  // 3 key_1,
  // 4 run_seed
  U64 u64[6];
  U32 u32[12];
  U16 u16[24];
};

/////////////////////////////////////////////////////////////////////////////////////////
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
  RK_Key target_key;
  RK_TrackTargetKind target_kind;
  RK_InterpolationKind interpolation;
  RK_TrackFrame *frame_btree_root;
  U64 frame_count;
  F32 duration_sec;
};

struct RK_Animation;
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

/////////////////////////////////////////////////////////////////////////////////////////
//~ Resource

typedef struct RK_Texture2D RK_Texture2D;
struct RK_Texture2D
{
  R_Tex2DSampleKind sample_kind;
  R_Handle          tex;
  Vec2F32           size;
};

// PBR material
typedef struct RK_Material RK_Material;
struct RK_Material
{
  RK_Handle textures[R_GeoTexKind_COUNT];
  R_Material3D v;
};

typedef struct RK_Skin RK_Skin;
struct RK_Skin
{
  RK_Bind     *binds;
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

  RK_Handle         material;

  // TODO(k): make use of morph targets
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
      F32 inner_radius;
      F32 outer_radius;
      U64 rings;
      U64 ring_segments;
    }
    torus_primitive;
  } src;
};

typedef struct RK_Animation RK_Animation;
struct RK_Animation
{
  String8 name;
  F32 duration_sec;

  RK_Track *tracks;
  U64 track_count;
};

typedef struct RK_SpriteSheetFrame RK_SpriteSheetFrame;
struct RK_SpriteSheetFrame
{
  F64 x;
  F64 y;
  F64 w;
  F64 h;
  B32 rotated;
  B32 trimmed;
  struct
  {
    F64 x;
    F64 y;
    F64 w;
    F64 h;
  }
  sprite_source_size;
  struct
  {
    F64 w;
    F64 h;
  }
  source_size;
  F64 duration;
};

typedef struct RK_SpriteSheetTag RK_SpriteSheetTag;
struct RK_SpriteSheetTag
{
  String8 name; 
  U64 from;
  U64 to;
  DirH direction;
  // TODO: what's for
  Vec4F32 color;
  F32 duration;
};

typedef struct RK_SpriteSheet RK_SpriteSheet;
struct RK_SpriteSheet
{
  RK_Handle           tex;
  Vec2F32             size;

  // animations
  U64                 frame_count;
  RK_SpriteSheetFrame *frames;
  U64                 tag_count;
  RK_SpriteSheetTag   *tags;
};

typedef struct RK_TileSet RK_TileSet;
struct RK_TileSet
{
  RK_Handle *textures;
  U64 texture_count;
};

typedef struct RK_PackedScene RK_PackedScene;
struct RK_PackedScene
{
  struct RK_Node *root;
};

typedef struct RK_Resource RK_Resource;
struct RK_Resource
{
  // Hash link (storage)
  RK_Resource *hash_next;
  RK_Resource *hash_prev;

  RK_Resource *next;

  U64 generation;
  RK_ResourceKind kind;
  RK_ResourceSrcKind src_kind;
  RK_Key key;

  // may contain multiple files
  union
  {
    String8 path;
    struct
    {
      String8 path_0;
      String8 path_1;
      String8 path_2;
    };
    String8 paths[3];
  };
  String8 name;

  struct RK_ResourceBucket *owner_bucket;

  union
  {
    RK_Texture2D tex2d;
    RK_Material mat;
    RK_Skin skin;
    RK_Mesh mesh;
    RK_Animation anim;
    RK_SpriteSheet spritesheet;
    RK_PackedScene packed;
    RK_TileSet tileset;
  } v;
};

/////////////////////////////////////////////////////////////////////////////////////////
//~ Node Equipment

typedef struct RK_Node2D RK_Node2D;
struct RK_Node2D
{
  // allocation link
  RK_Node2D      *next;
  RK_Transform2D transform;
  F32            z_index; /* used for sorting drawing order */
};

typedef struct RK_Node3D RK_Node3D;
struct RK_Node3D
{
  // allocation link
  RK_Node3D      *next;
  RK_Transform3D transform;
};

typedef struct RK_Camera3D RK_Camera3D;
struct RK_Camera3D
{
  RK_Camera3D            *next;
  RK_ProjectionKind      projection;
  RK_ViewportShadingKind viewport_shading;
  R_GeoPolygonKind       polygon_mode;
  Rng2F32                viewport;
  B32                    hide_cursor;
  B32                    lock_cursor;
  B32                    is_active;
  F32                    zn;
  F32                    zf;

  union
  {
    // Perspective
    struct
    {
      F32 fov;
    }
    perspective;

    // Orthographic
    struct
    {
      F32 left;
      F32 right;
      F32 bottom;
      F32 top;
    }
    orthographic;
    F32 v[4];
  };
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
  B32               omit_depth_test;
  B32               omit_light;
  B32               draw_edge;
};

typedef struct RK_SceneInstance RK_SceneInstance;
struct RK_SceneInstance
{
  RK_SceneInstance *next;
  RK_PackedScene   *packed_scene;
};

typedef struct RK_AnimationPlayer RK_AnimationPlayer;
struct RK_AnimationPlayer
{
  RK_AnimationPlayer *next;
  RK_Handle *animations;
  U64 animation_count;

  RK_Key target_seed;
  RK_AnimationPlayback playback;
};

typedef struct RK_DirectionalLight RK_DirectionalLight;
struct RK_DirectionalLight
{
  RK_DirectionalLight *next;
  Vec3F32 direction;
  Vec3F32 color;
  F32 intensity;
};

typedef struct RK_PointLight RK_PointLight;
struct RK_PointLight
{
  RK_PointLight *next;
  // light
  F32 intensity;
  F32 range;
  Vec3F32 color;
  Vec3F32 attenuation; // x: constant, y: linear, z: quadratic
};

typedef struct RK_SpotLight RK_SpotLight;
struct RK_SpotLight
{
  RK_SpotLight *next;
  // cone shape
  Vec3F32 direction;
  F32 range;
  F32 angle;

  // light
  F32 intensity;
  Vec3F32 color;
  Vec3F32 attenuation; // x: constant, y: linear, z: quadratic
};

typedef struct RK_Sprite2D RK_Sprite2D;
struct RK_Sprite2D
{
  RK_Sprite2D *next;
  RK_Handle tex;
  RK_Sprite2DShapeKind shape;
  union
  {
    Vec2F32 rect;
    struct
    {
      F32 radius;
    } circle;
    Vec2F32 v;
  } size;
  RK_Sprite2DAnchorKind anchor;
  // TODO(XXX): it's weird to put color here
  Vec4F32 color;
  B32 omit_texture;
  B32 draw_edge;

  // string stuffs
  String8 string;
  D_FancyRunList fancy_run_list;
  F_Tag font;
  F32 font_size;
  Vec4F32 font_color;
  U64 tab_size;
  F_RasterFlags text_raster_flags;
};

typedef struct RK_Collider2D RK_Collider2D;
struct RK_Collider2D
{
  RK_Collider2D *next;
  RK_Collider2DShapeKind shape;
  union
  {
    struct
    {
      F32 w;
      F32 h;
    } rect;

    struct
    {
      F32 radius;
    } circle;

    // TODO: triangle
  } size;
};

typedef struct RK_AnimatedSprite2D RK_AnimatedSprite2D;
struct RK_AnimatedSprite2D
{
  RK_AnimatedSprite2D *next;

  B32 flipped;

  // animations
  RK_Handle sheet;
  Vec2F32 size;
  U64 curr_tag;
  U64 next_tag;
  B32 loop;
  B32 is_animating;
  F32 ts_ms;
};

typedef struct RK_TileMapLayer RK_TileMapLayer;
struct RK_TileMapLayer
{
  String8 name;
  RK_TileMapLayer *next;
};

typedef struct RK_TileMap RK_TileMap;
struct RK_TileMap
{
  RK_TileMap *next;

  Vec2U32 size; /* row&col count */
  Vec2F32 tile_size;

  // y_sort

  Mat2x2F32 mat;
  Mat2x2F32 mat_inv;
};

typedef struct RK_Particle3D RK_Particle3D;
struct RK_Particle3D
{
  RK_Particle3D *next;
  PH_Particle3D v;
};

typedef struct RK_HookSpring3D RK_HookSpring3D;
struct RK_HookSpring3D
{
  RK_HookSpring3D *next;
  F32 kd;
  F32 ks;
  F32 rest;
  RK_Handle a;
  RK_Handle b;
};

typedef struct RK_Constraint3D RK_Constraint3D;
struct RK_Constraint3D
{
  RK_Constraint3D *next;
  PH_Constraint3DKind kind;
  F32 d; /* distance */

  union
  {
    // TODO(XXX): binary & unary for now
    struct {RK_Handle a; RK_Handle b;};
    RK_Handle v[2];
  } targets;
  U64 target_count;
};

typedef struct RK_Rigidbody3D RK_Rigidbody3D;
struct RK_Rigidbody3D
{
  RK_Rigidbody3D *next;
  PH_Rigidbody3D v;
};

/////////////////////////////////
// Other Helper Types

typedef struct RK_FrameContext RK_FrameContext;
struct RK_FrameContext
{
  Vec3F32 eye; /* world position of camera */
  Mat4x4F32 proj_m;
  Mat4x4F32 view_m;
  Mat4x4F32 proj_view_m;
  Mat4x4F32 proj_view_inv_m;
};

/////////////////////////////////
//~ Node Type

struct RK_Scene;
#define RK_NODE_CUSTOM_UPDATE(name) void name(struct RK_Node *node, struct RK_Scene *scene, RK_FrameContext *ctx)
typedef RK_NODE_CUSTOM_UPDATE(RK_NodeCustomUpdateFunctionType);

#define RK_NODE_CUSTOM_DRAW(name) void name(struct RK_Node *node, void *node_data)
typedef RK_NODE_CUSTOM_DRAW(RK_NodeCustomDrawFunctionType);

typedef struct RK_UpdateFnNode RK_UpdateFnNode;
struct RK_UpdateFnNode
{
  String8 name; 
  RK_UpdateFnNode *next;
  RK_UpdateFnNode *prev;
  RK_NodeCustomUpdateFunctionType *f;
};

typedef struct RK_Node RK_Node;
struct RK_Node
{
  // Instance
  RK_PackedScene                *instance; // if this node came from a PackedScene
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

  //~ Free list links (next_to_free or next_free)
  RK_Node                       *free_next;

  struct RK_NodeBucket          *owner_bucket;

  U64                           generation;
  RK_Key                        key;
  // NOTE(k): we need to reuse RK_Node, so the string has to be fixed sized
  String8                       name;
  U8                            name_buffer[300];
  RK_NodeFlags                  flags;
  RK_NodeTypeFlags              type_flags;
  U64                           custom_flags; /* for game logic */

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
  RK_SceneInstance              *scene_inst;
  RK_AnimationPlayer            *animation_player;
  RK_DirectionalLight           *directional_light;
  RK_PointLight                 *point_light;
  RK_SpotLight                  *spot_light;
  RK_Sprite2D                   *sprite2d;
  RK_Collider2D                 *collider2d;
  RK_AnimatedSprite2D           *animated_sprite2d;
  RK_TileMapLayer               *tilemap_layer;
  RK_TileMap                    *tilemap;
  RK_Particle3D                 *particle3d;
  RK_HookSpring3D               *hook_spring3d;
  RK_Constraint3D               *constraint3d;
  RK_Rigidbody3D                *rigidbody3d;

  // animations
  F32                           hot_t;
  F32                           active_t;

  //~ Custom update/draw functions
  // TODO(k): reuse custom_data memory block based on its size
  void                          *custom_data;

  // TODO(k): do we really need individual udpate function ptr for node or 
  //          maybe we just use one entry update fn for the scene
  // function pointers
  RK_UpdateFnNode               *first_setup_fn;
  RK_UpdateFnNode               *last_setup_fn;
  RK_UpdateFnNode               *first_update_fn;
  RK_UpdateFnNode               *last_update_fn;
  RK_UpdateFnNode               *first_fixed_update_fn;
  RK_UpdateFnNode               *last_fixed_update_fn;

  // RK_NodeCustomDrawFunctionType *custom_draw;

  //~ Artifacts (global xform,position,rotation,scale)
  Mat4x4F32                     fixed_xform;
  Vec3F32                       fixed_position;
  QuatF32                       fixed_rotation;
  Vec3F32                       fixed_scale;
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
  Arena               *arena_ref;

  RK_NodeBucketSlot   *hash_table;
  U64                 hash_table_size;
  U64                 node_count;

  // NOTE(k): we can't delete node when looping through the tree
  //          for example, we delete children node in parent update fn, the rec already store the next node
  RK_Node             *first_to_free_node;
  //~ Resource Management
  RK_Node             *first_free_node;
  RK_UpdateFnNode     *first_free_update_fn_node;

  //- Equipment
  RK_Node2D           *first_free_node2d;
  RK_Node3D           *first_free_node3d;
  RK_Camera3D         *first_free_camera3d;
  RK_MeshInstance3D   *first_free_mesh_inst3d;
  RK_SceneInstance    *first_free_scene_inst;
  RK_AnimationPlayer  *first_free_animation_player;
  RK_DirectionalLight *first_free_directional_light;
  RK_PointLight       *first_free_point_light;
  RK_SpotLight        *first_free_spot_light;
  RK_Sprite2D         *first_free_sprite2d;
  RK_Collider2D       *first_free_collider2d;
  RK_AnimatedSprite2D *first_free_animated_sprite2d;
  RK_TileMapLayer     *first_free_tilemap_layer;
  RK_TileMap          *first_free_tilemap;
  RK_Particle3D       *first_free_particle3d;
  RK_HookSpring3D     *first_free_hook_spring3d;
  RK_Constraint3D     *first_free_constraint3d;
  RK_Rigidbody3D      *first_free_rigidbody3d;
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
  Arena *arena_ref;
  RK_ResourceBucketSlot *hash_table;
  U64 hash_table_size;
  U64 res_count;
  RK_Resource *first_free_resource;
};

/////////////////////////////////
// Scene

typedef struct RK_Scene RK_Scene;
#define RK_SCENE_SETUP(name) void name(struct RK_Scene *scene)
#define RK_SCENE_UPDATE(name) void name(struct RK_Scene *scene, RK_FrameContext *ctx)
#define RK_SCENE_DEFAULT(name) RK_Scene* name()

typedef RK_SCENE_SETUP(RK_SceneSetupFunctionType);
typedef RK_SCENE_UPDATE(RK_SceneUpdateFunctionType);
typedef RK_SCENE_DEFAULT(RK_SceneDefaultFunctionType);

struct RK_Scene
{
  RK_Scene                    *next;

  U64                         frame_index;
  // Storage
  Arena*                      arena;
  RK_NodeBucket*              node_bucket;
  RK_NodeBucket*              res_node_bucket;
  RK_ResourceBucket*          res_bucket;

  RK_Handle                   root;
  RK_Handle                   active_camera;

  RK_Key                      hot_key;
  RK_Key                      active_key;

  RK_Handle                   active_node;

  B32                         omit_grid;
  B32                         omit_gizmo3d;
  B32                         omit_light;

  // physics states (particle 3d/2d, rigidbody 3d/2d)
  PH_Particle3DSystem         particle3d_system;
  PH_Rigidbody3DSystem        rigidbody3d_system;

  // ambient light
  Vec4F32                     ambient_light;
  Vec4F32                     ambient_scale;

  // gizmo
  RK_Gizmo3DMode              gizmo3d_mode;

  // custom data
  void*                       custom_data;

  // NOTE(k): for serialization
  String8                     setup_fn_name;    // run once after initialization
  String8                     update_fn_name;   // run once after initialization
  String8                     default_fn_name;  // load default configuration

  RK_SceneSetupFunctionType   *setup_fn;
  RK_SceneUpdateFunctionType  *update_fn;
  RK_SceneDefaultFunctionType *default_fn;

  String8                     name;
  String8                     save_path;

  U64                         handle_seed;
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
  RK_FontSlot_Game,
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
  String8         name;
  void            *ptr;
};

typedef struct RK_FunctionSlot RK_FunctionSlot;
struct RK_FunctionSlot
{
  RK_FunctionNode *first;
  RK_FunctionNode *last;
};

// for mdesk
typedef struct RK_FunctionEntry RK_FunctionEntry;
struct RK_FunctionEntry
{
  String8 name;
  void *fn;
};

/////////////////////////////////
// Dynamic drawing (in immediate mode fashion)

typedef struct RK_DrawNode RK_DrawNode;
struct RK_DrawNode
{
  RK_DrawNode *next;
  RK_DrawNode *draw_next;
  RK_DrawNode *draw_prev;

  R_GeoPolygonKind polygon;
  R_GeoTopologyKind topology;

  // vertex
  // src
  R_Vertex *vertices_src;
  U64 vertex_count;
  // dst
  R_Handle vertices;
  U64 vertices_buffer_offset;

  // indice
  // src
  U32 *indices_src;
  U64 indice_count;
  // dst
  R_Handle indices;
  U64 indices_buffer_offset;

  // for font altas
  R_Handle albedo_tex;
};

typedef struct RK_DrawList RK_DrawList;
struct RK_DrawList
{
  // per-build
  RK_DrawNode *first_node;
  RK_DrawNode *last_node;
  U64 node_count;
  U64 vertex_buffer_cmt;
  U64 indice_buffer_cmt;

  // persistent
  R_Handle vertices;
  R_Handle indices;
  U64 vertex_buffer_cap;
  U64 indice_buffer_cap;
};

/////////////////////////////////
// State 

typedef struct RK_State RK_State;
struct RK_State
{
  Arena                 *arena;
  RK_Scene              *active_scene;
  Arena                 *frame_arenas[2];

  R_Handle              r_wnd;
  OS_Handle             os_wnd;
  OS_EventList          os_events;

  //- UI overlay signal (used for handle user input)
  UI_Signal             sig;

  // global storage buckets
  RK_NodeBucket         *node_bucket;
  RK_NodeBucket         *res_node_bucket;
  RK_ResourceBucket     *res_bucket;

  // drawing buckets
  D_Bucket              *bucket_rect;
  // TODO(XXX): renaming is needed
  D_Bucket              *bucket_geo[RK_GeoBucketKind_COUNT];

  // drawlists
  RK_DrawList           *drawlists[2];

  // frame history info
  U64                   frame_index;
  U64                   frame_time_us_history[64];
  F64                   time_in_seconds;
  U64                   time_in_us;

  // frame parameters
  F32                   frame_dt;
  F32                   cpu_time_us;
  F32                   pre_cpu_time_us;

  // window
  Rng2F32               window_rect;
  Vec2F32               window_dim;
  Rng2F32               last_window_rect;
  Vec2F32               last_window_dim;
  B32                   window_res_changed;
  F32                   dpi;
  F32                   last_dpi;
  B32                   window_should_close;

  // cursor
  Vec2F32               cursor;
  Vec2F32               last_cursor;
  Vec2F32               cursor_delta;
  B32                   cursor_hidden;

  // hot pixel key from renderer
  U64                   hot_pixel_key;

  // TODO(k): for serilization, kind weird, find a better way
  // function registry
  RK_FunctionSlot       *function_registry;
  U64                   function_registry_size;

  // drag/drop state
  RK_DragDropState      drag_drop_state;
  U64                   drag_drop_slot;
  void                  *drag_drop_src;

  // theme
  RK_Theme              cfg_theme_target;
  RK_Theme              cfg_theme;
  F_Tag                 cfg_font_tags[RK_FontSlot_COUNT];

  // palette
  UI_Palette            cfg_ui_debug_palettes[RK_PaletteCode_COUNT]; // derivative from theme

  // global Settings
  RK_SettingVal         setting_vals[RK_SettingCode_COUNT];

  // views (UI)
  RK_View               views[RK_ViewKind_COUNT];
  B32                   views_enabled[RK_ViewKind_COUNT];

  // animation
  struct
  {
    F32 vast_rate;
    F32 fast_rate;
    F32 fish_rate;
    F32 slow_rate;
    F32 slug_rate;
    F32 slaf_rate;
  } animation;

  RK_Scene              *next_active_scene;
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
internal RK_Key rk_key_make(U64 a, U64 b);
internal RK_Key rk_key_zero();
#define rk_key_from_v2u64(v) ((RK_Key){(v).x, (v).y})

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

internal RK_Resource* rk_res_alloc(RK_Key key);
internal void         rk_res_release(RK_Resource *res);
internal RK_Resource* rk_res_store(RK_Key key, void *data);
internal RK_Resource* rk_res_from_key(RK_Key key);
internal RK_Handle    rk_handle_from_res(RK_Resource *res);
internal RK_Resource* rk_res_from_handle(RK_Handle *h);
#define rk_res_unwrap(res) (res != 0 ? &res->v : 0)
#define rk_res_from_inner(inner) CastFromMember(RK_Resource,v,inner)
#define rk_res_inner_from_handle(h) (rk_res_unwrap(rk_res_from_handle((h)))) 

#define rk_tex2d_from_handle(h)   ((RK_Texture2D*)rk_res_inner_from_handle((h)))
#define rk_sheet_from_handle(h)   ((RK_SpriteSheet*)rk_res_inner_from_handle((h)))
#define rk_tileset_from_handle(h) ((RK_TileSet*)rk_res_inner_from_handle((h)))
#define rk_mesh_from_handle(h)    ((RK_Mesh*)rk_res_inner_from_handle((h)))
#define rk_skin_from_handle(h)    ((RK_Skin*)rk_res_inner_from_handle((h)))
#define rk_mat_from_handle(h)     ((RK_Material*)rk_res_inner_from_handle((h)))
#define rk_packed_from_handle(h)  ((RK_PackedScene*)rk_res_inner_from_handle((h)))

/////////////////////////////////
//- Resourcea Building Helpers (subresource management etc...)

/////////////////////////////////
//~ State accessor/mutator

internal void             rk_init(OS_Handle os_wnd, R_Handle r_wnd);
internal B32              rk_drag_is_active(void);
internal void             rk_drag_begin(U64 slot, void *src);
internal B32              rk_drag_drop(void);
internal Arena*           rk_frame_arena();
internal RK_DrawList*     rk_frame_drawlist();
internal void             rk_register_function(String8 name, RK_NodeCustomUpdateFunctionType *ptr);
internal RK_FunctionNode* rk_function_from_string(String8 string);
internal RK_View*         rk_view_from_kind(RK_ViewKind kind);
internal void             rk_ps_push_particle3d(PH_Particle3D *p);
internal void             rk_ps_push_force3d(PH_Force3D *force);
internal void             rk_ps_push_constraint3d(PH_Constraint3D *c);
internal void             rk_rs_push_rigidbody3d(PH_Rigidbody3D *rb);
internal void             rk_rs_push_force3d(PH_Force3D *force);
internal void             rk_rs_push_constraint3d(PH_Constraint3D *c);

/////////////////////////////////
//~ Node build api

internal RK_Node* rk_node_alloc(RK_NodeTypeFlags type_flags);
internal void     rk_node_release(RK_Node *node);
internal void     rk_node_equip_type_flags(RK_Node *n, RK_NodeTypeFlags);
internal void     rk_node_unequip_type_flags(RK_Node *n, RK_NodeTypeFlags);
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
internal RK_NodeRec rk_node_df(RK_Node *n, RK_Node *root, U64 sib_member_off, U64 child_member_off, B32 force_leaf);
#define rk_node_df_pre(node, root, force_leaf) rk_node_df(node, root, OffsetOf(RK_Node, next), OffsetOf(RK_Node, first), force_leaf)
#define rk_node_df_post(node, root, force_leaf) rk_node_df(node, root, OffsetOf(RK_Node, prev), OffsetOf(RK_Node, last), force_leaf)

internal void rk_node_push_fn_(RK_Node *n, String8 fn_name);
#define rk_node_push_fn(n, f) rk_node_push_fn_(n, str8_lit(#f))
internal RK_Handle rk_handle_from_node(RK_Node *n);
internal RK_Node* rk_node_from_handle(RK_Handle *handle);
internal B32 rk_node_is_active(RK_Node *node);

#define rk_scene_push_custom_data(s, T) (T*)(s->custom_data = push_array_fat(s->arena, T, s))
#define rk_node_push_custom_data(n, T) (T*)(n->custom_data = push_array_fat(n->owner_bucket->arena_ref, T, n))

/////////////////////////////////////////////////////////////////////////////////////////
// Magic

////////////////////////////////
// Support functions

#define RK_SHAPE_SUPPORT_FN(name) Vec2F32 name(void *shape_data, Vec2F32 direction)
typedef RK_SHAPE_SUPPORT_FN(RK_SHAPE_CUSTOM_SUPPORT_FN);
RK_SHAPE_SUPPORT_FN(RK_SHAPE_RECT_SUPPORT_FN);

////////////////////////////////
// GJK
// internal B32 gjk(Vec3F32 s1_center, Vec3F32 s2_center, void *s1_data, void *s2_data, RK_SHAPE_CUSTOM_SUPPORT_FN s1_support_fn, RK_SHAPE_CUSTOM_SUPPORT_FN s2_support_fn, Vec3F32 simplex[4]);
internal Vec2F32 triple_product_2f32(Vec2F32 A, Vec2F32 B, Vec2F32 C);

/////////////////////////////////////////////////////////////////////////////////////////
// Colors, Fonts, Config

////////////////////////////////
// color 

internal Vec4F32 rk_rgba_from_theme_color(RK_ThemeColor color);

////////////////////////////////
// code -> palette

internal UI_Palette *rk_palette_from_code(RK_PaletteCode code);

////////////////////////////////
// fonts/sizes
internal F_Tag rk_font_from_slot(RK_FontSlot slot);
internal F32   rk_font_size_from_slot(RK_FontSlot slot);

/////////////////////////////////////////////////////////////////////////////////////////
// UI widget

internal void rk_ui_stats(void);
internal void rk_ui_inspector(void);
internal void rk_ui_profiler(void);
internal void rk_ui_terminal(void);

/////////////////////////////////////////////////////////////////////////////////////////
// Frame

internal void rk_ui_draw(void);
internal B32  rk_frame(void);

/////////////////////////////////////////////////////////////////////////////////////////
// Dynamic drawing (in immediate mode fashion)

internal RK_DrawList* rk_drawlist_alloc(Arena *arena, U64 vertex_buffer_cap, U64 indice_buffer_cap);
internal RK_DrawNode* rk_drawlist_push(Arena *arena, RK_DrawList *drawlist, R_Vertex *vertices_src, U64 vertex_count, U32 *indices_src, U64 indice_count);
internal void         rk_drawlist_reset(RK_DrawList *drawlist);
internal void         rk_drawlist_build(RK_DrawList *drawlist); /* upload buffer from cpu to gpu */

/////////////////////////////////////////////////////////////////////////////////////////
// Helpers

#define FileReadAll(arena, fp, return_data, return_size)                                 \
  do                                                                                     \
  {                                                                                      \
    OS_Handle f = os_file_open(OS_AccessFlag_Read, (fp));                                \
    FileProperties f_props = os_properties_from_file(f);                                 \
    *return_size = f_props.size;                                                         \
    *return_data = push_array(arena, U8, f_props.size);                                  \
    os_file_read(f, rng_1u64(0,f_props.size), *return_data);                             \
  } while (0);
internal void      rk_trs_from_xform(Mat4x4F32 *m, Vec3F32 *trans, QuatF32 *rot, Vec3F32 *scale);
internal void      rk_ijk_from_xform(Mat4x4F32 m, Vec3F32 *i, Vec3F32 *j, Vec3F32 *k);
internal Mat4x4F32 rk_xform_from_transform3d(RK_Transform3D *transform);
internal Mat4x4F32 rk_xform_from_trs(Vec3F32 translate, QuatF32 rotation, Vec3F32 scale);
internal F32       rk_plane_intersect(Vec3F32 ray_start, Vec3F32 ray_end, Vec3F32 plane_normal, Vec3F32 plane_point);
internal Rng2F32   rk_rect_from_sprite2d(RK_Sprite2D *sprite2d, Vec2F32 pos);
internal Vec2F32   rk_center_from_sprite2d(RK_Sprite2D *sprite2d, Vec2F32 pos);
internal void      rk_sprite2d_equip_string(Arena *arena, RK_Sprite2D *sprite2d, String8 string, F_Tag font, F32 font_size, Vec4F32 font_color, U64 tab_size, F_RasterFlags text_raster_flags);
internal int       rk_node2d_cmp_z_rev(const void *left, const void *right);
internal int       rk_path_cmp(const void *left_, const void *right_);
internal U64       rk_no_from_filename(String8 filename);
#define rk_handle_from_se(h) ((RK_Handle){ .u64 = {h.u64[0], h.u64[1], h.u64[2], h.u64[3], h.u64[4], h.u64[5]} })

// TODO: move these into base_arena
#define rk_ptr_from_fat(payload_ptr)  *(void**)((U8*)payload_ptr-16)
#define rk_size_from_fat(payload_ptr) *(U64*)((U8*)payload_ptr-8)

#define rk_text(font, size, base_align_px, tab_size_px, flags, p, color, string) \
D_BucketScope(rk_state->bucket_rect) {d_text(font,size,base_align_px,tab_size_px,flags,p,color,string);}
#define rk_debug_gfx(size,p,color,string) rk_text(rk_state->cfg_font_tags[RK_FontSlot_Code], size, 0, 2, 0, p, color, string)

/////////////////////////////////////////////////////////////////////////////////////////
// Enum to string

internal String8 rk_string_from_projection_kind(RK_ProjectionKind kind);
internal String8 rk_string_from_viewport_shading_kind(RK_ViewportShadingKind kind);
internal String8 rk_string_from_polygon_kind(RK_ViewportShadingKind kind);

/////////////////////////////////////////////////////////////////////////////////////////
// Scene Type Functions

internal RK_Scene* rk_scene_alloc();
internal void      rk_scene_release(RK_Scene *s);
internal void      rk_scene_active_camera_set(RK_Scene *s, RK_Node *camera_node);
internal void      rk_scene_active_node_set(RK_Scene *s, RK_Key key, B32 only_navigation_root);

/////////////////////////////////////////////////////////////////////////////////////////
// Gizmo Drawing Functions

internal void rk_gizmo3d_rect(RK_Key key, Rng2F32 dst, Rng2F32 src, R_GeoPolygonKind polygon, Vec4F32 clr, R_Handle albedo_tex, B32 draw_edge);
internal void rk_gizmo3d_line(RK_Key key, Vec3F32 start, Vec3F32 end, Vec4F32 clr, F32 line_width);
internal void rk_gizmo3d_cone(RK_Key key, Vec3F32 origin, Vec3F32 normal, F32 radius, F32 height, R_GeoPolygonKind polygon, Vec4F32 clr, B32 draw_edge);
internal void rk_gizmo3d_plane(RK_Key key, Vec3F32 origin, Vec3F32 i_hat, Vec3F32 j_hat, Vec2F32 size, R_GeoPolygonKind polygon, Vec4F32 clr, B32 draw_edge);
internal void rk_gizmo3d_sphere(RK_Key key, Vec3F32 origin, F32 radius, F32 height, B32 is_hemisphere, R_GeoPolygonKind polygon, Vec4F32 clr, B32 draw_edge);
internal void rk_gizmo3d_arc(RK_Key key, Vec3F32 origin, Vec3F32 start, Vec3F32 end, R_GeoPolygonKind polygon, Vec4F32 clr, B32 draw_edge);
internal void rk_gizmo3d_circle_lined(RK_Key key, Vec3F32 origin, Vec3F32 normal, F32 radius, Vec4F32 clr, F32 line_width, B32 draw_edge);
internal void rk_gizmo3d_box(RK_Key key, Vec3F32 origin, Vec3F32 i_hat, Vec3F32 j_hat, Vec3F32 size, R_GeoPolygonKind polygon, Vec4F32 clr, B32 draw_edge);

#define rk_gizmo3d_rect_filled(key, dst, src, clr, albedo_tex, draw_edge) rk_gizmo3d_rect(key, dst, src, R_GeoPolygonKind_Fill, clr, albedo_tex, draw_edge)
#define rk_gizmo3d_rect_wired(key, dst, src, clr, albedo_tex, draw_edge) rk_gizmo3d_rect(key, dst, src, R_GeoPolygonKind_Line, clr, albedo_tex, draw_edge)
#define rk_gizmo3d_cone_filled(key, origin, normal, radius, height, clr, draw_edge) rk_gizmo3d_cone(key, origin, normal, radius, height, R_GeoPolygonKind_Fill, clr, draw_edge)
#define rk_gizmo3d_cone_wired(key, origin, normal, radius, height, clr, draw_edge) rk_gizmo3d_cone(key, origin, normal, radius, height, R_GeoPolygonKind_Line, clr, draw_edge)
#define rk_gizmo3d_plane_filled(key, origin, i_hat, j_hat, size, clr, draw_edge) rk_gizmo3d_plane(key, origin, i_hat, j_hat, size, R_GeoPolygonKind_Fill, clr, draw_edge)
#define rk_gizmo3d_plane_wired(key, origin, i_hat, j_hat, size, clr, draw_edge) rk_gizmo3d_plane(key, origin, i_hat, j_hat, size, R_GeoPolygonKind_Line, clr, draw_edge)
#define rk_gizmo3d_sphere_filled(key, origin, radius, height, is_hemisphere, clr, draw_edge) rk_gizmo3d_sphere(key, origin, radius, height, is_hemisphere, R_GeoPolygonKind_Fill, clr, draw_edge)
#define rk_gizmo3d_sphere_wired(key, origin, radius, height, is_hemisphere, clr, draw_edge) rk_gizmo3d_sphere(key, origin, radius, height, is_hemisphere, R_GeoPolygonKind_Line, clr, draw_edge)
#define rk_gizmo3d_arc_filled(key, origin, normal, raidus, clr, draw_edge) rk_gizmo3d_arc(key, origin, normal, raidus, R_GeoPolygonKind_Fill, clr, draw_edge)
#define rk_gizmo3d_arc_wired(key, origin, normal, raidus, clr, draw_edge) rk_gizmo3d_arc(key, origin, normal, raidus, R_GeoPolygonKind_Line, clr, draw_edge)
#define rk_gizmo3d_box_filled(key, origin, i_hat, j_hat, size, clr, draw_edge) rk_gizmo3d_box(key, origin, i_hat, j_hat, size, R_GeoPolygonKind_Fill, clr, draw_edge)
#define rk_gizmo3d_box_wired(key, origin, i_hat, j_hat, size, clr, draw_edge) rk_gizmo3d_box(key, origin, i_hat, j_hat, size, R_GeoPolygonKind_Line, clr, draw_edge)

/////////////////////////////////////////////////////////////////////////////////////////
// OS events

////////////////////////////////
// event consumption helpers

internal void rk_eat_event(OS_EventList *list, OS_Event *event);
internal B32  rk_key_press(OS_Modifiers mods, OS_Key key);
internal B32  rk_key_release(OS_Modifiers mods, OS_Key key);

#endif
