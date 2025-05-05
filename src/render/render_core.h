#ifndef RENDER_CORE_H
#define RENDER_CORE_H

////////////////////////////////
//~ some limits/constants

// support max 1 rect pass per frame
#define MAX_RECT_PASS 1
#define MAX_RECT_GROUPS 3000
#define MAX_GEO2D_PASS 1
// support max 3 geo3d pass per frame
#define MAX_GEO3D_PASS 3
#define MAX_JOINTS_PER_PASS 3000
#define MAX_LIGHTS_PER_PASS 3000
#define MAX_MATERIALS_PER_PASS 30000
// inst count limits
#define MAX_RECT_INSTANCES 10000
#define MAX_MESH2D_INSTANCES 10000
#define MAX_MESH3D_INSTANCES 10000

////////////////////////////////
//~ rjf: Enums

typedef U32 R_GeoVertexFlags;
enum
{
  R_GeoVertexFlag_TexCoord = (1 << 0),
  R_GeoVertexFlag_Normals  = (1 << 1),
  R_GeoVertexFlag_RGB      = (1 << 2),
  R_GeoVertexFlag_RGBA     = (1 << 3),
};

typedef enum R_Tex2DFormat
{
  R_Tex2DFormat_R8,
  R_Tex2DFormat_RG8,
  R_Tex2DFormat_RGBA8,
  R_Tex2DFormat_BGRA8,
  R_Tex2DFormat_R16,
  R_Tex2DFormat_RGBA16,
  R_Tex2DFormat_R32,
  R_Tex2DFormat_RG32,
  R_Tex2DFormat_RGBA32,
  R_Tex2DFormat_COUNT,
} R_Tex2DFormat;

typedef enum R_ResourceKind
{
  R_ResourceKind_Static,
  R_ResourceKind_Dynamic,
  R_ResourceKind_Stream,
  R_ResourceKind_COUNT,
} R_ResourceKind;

typedef enum R_Tex2DSampleKind
{
  R_Tex2DSampleKind_Nearest,
  R_Tex2DSampleKind_Linear,
  R_Tex2DSampleKind_COUNT,
} R_Tex2DSampleKind;

typedef enum R_GeoTopologyKind
{
  R_GeoTopologyKind_Lines,
  R_GeoTopologyKind_LineStrip,
  R_GeoTopologyKind_Triangles,
  R_GeoTopologyKind_TriangleStrip,
  R_GeoTopologyKind_COUNT,
} R_GeoTopologyKind;

typedef enum R_GeoPolygonKind
{
  R_GeoPolygonKind_Fill,
  R_GeoPolygonKind_Line,
  R_GeoPolygonKind_Point,
  R_GeoPolygonKind_COUNT,
} R_GeoPolygonKind;

typedef enum R_GeoTexKind
{
  R_GeoTexKind_Ambient,
  R_GeoTexKind_Emissive,
  R_GeoTexKind_Diffuse,
  R_GeoTexKind_Specular,
  R_GeoTexKind_SpecularPower,
  R_GeoTexKind_Normal,
  R_GeoTexKind_Bump,
  R_GeoTexKind_Opacity,
  R_GeoTexKind_COUNT,
} R_GeoTexKind;

// NOTE(k): whenever you change this enum, you will also need to change r_pass_kind_batch_table & r_pass_kind_params_size_table
typedef enum R_PassKind
{
  R_PassKind_UI,
  R_PassKind_Blur,
  R_PassKind_Geo2D,
  R_PassKind_Geo3D,
  R_PassKind_COUNT,
} R_PassKind;

////////////////////////////////
//~ rjf: Handle Type

typedef union R_Handle R_Handle;
union R_Handle
{
  U64 u64[2];
  U32 u32[4];
  U16 u16[8];
};

////////////////////////////////
//~ rjf: Primitive Types
typedef struct R_Vertex R_Vertex; 
struct R_Vertex
{
  Vec3F32 pos;  
  Vec3F32 nor;
  Vec2F32 tex;
  Vec3F32 tan;
  Vec4F32 col;
  Vec4U32 joints;
  Vec4F32 weights; // morph target weights
};

////////////////////////////////
//~ rjf: Instance Types

typedef struct R_Rect2DInst R_Rect2DInst;
struct R_Rect2DInst
{
  Rng2F32 dst;
  Rng2F32 src;
  Vec4F32 colors[Corner_COUNT];
  F32 corner_radii[Corner_COUNT];
  F32 border_thickness;
  F32 edge_softness;
  F32 white_texture_override;
  F32 _unused_[1];
};

typedef struct R_Mesh2DInst R_Mesh2DInst;
struct R_Mesh2DInst
{
  Mat4x4F32 xform;
  Mat4x4F32 xform_inv;
  U64       key;
  B32       has_texture;
  Vec4F32   color;
  B32       has_color;
};

typedef struct R_Mesh3DInst R_Mesh3DInst;
struct R_Mesh3DInst
{
  // TODO(k): kind a mess here, some attributes were sent to instance buffer, some were copied to storage buffer 
  Mat4x4F32 xform;
  Mat4x4F32 xform_inv;
  U64       key;
  U32       material_idx;
  B32       draw_edge;
  // NOTE(k): joint_xforms is stored in storage buffer instead of instance buffer
  Mat4x4F32 *joint_xforms;
  U32       joint_count;
  U32       first_joint;
  B32       depth_test;
  B32       omit_light;
};

////////////////////////////////
//~ k: Some basic types

typedef struct R_Light3D R_Light3D;
struct R_Light3D
{
  // position for point and spot lights (world space)
  Vec4F32 position_ws;
  // direction for spot and directional lights (world space)
  Vec4F32 direction_ws;
  // position for point and spot lights (view space)
  Vec4F32 position_vs;
  // direction for spot and directional lights (view space)
  Vec4F32 direction_vs;
  // color of the light, diffuse and specular colors are not seperated
  Vec4F32 color;
  // x: constant y: linear z: quadratic
  Vec4F32 attenuation;
  // the half angle of the spotlight cone
  F32 spot_angle;
  // the range of the light
  F32 range;
  F32 intensity;
  U32 kind;
};

typedef struct R_Material3D R_Material3D;
struct R_Material3D
{
  // textures
  B32 has_ambient_texture;
  B32 has_emissive_texture;
  B32 has_diffuse_texture;
  B32 has_specular_texture;
  B32 has_specular_power_texture;
  B32 has_normal_texture;
  B32 has_bump_texture;
  B32 has_opacity_texture;

  // color
  Vec4F32 ambient_color;
  Vec4F32 emissive_color;
  Vec4F32 diffuse_color;
  Vec4F32 specular_color;
  Vec4F32 reflectance;

  // sample_channel maps
  Mat4x4F32 diffuse_sample_channel_map;

  // f32
  F32 opacity;
  F32 specular_power;
  // for transparent materials, IOR > 0
  F32 index_of_refraction;
  F32 bump_intensity;
  F32 specular_scale;
  F32 alpha_cutoff;
  F32 _padding_0[2];
};

typedef struct R_PackedTextures R_PackedTextures;
struct R_PackedTextures
{
  R_Handle array[R_GeoTexKind_COUNT];
};

////////////////////////////////
//~ rjf: Batch Types

typedef struct R_Batch R_Batch;
struct R_Batch
{
  U8 *v;
  U64 byte_count;
  U64 byte_cap;
};

typedef struct R_BatchNode R_BatchNode;
struct R_BatchNode
{
  R_BatchNode *next;
  R_Batch v;
};

typedef struct R_BatchList R_BatchList;
struct R_BatchList
{
  R_BatchNode *first;
  R_BatchNode *last;
  U64 batch_count;
  U64 byte_count;
  U64 bytes_per_inst;
};

typedef struct R_BatchGroupRectParams R_BatchGroupRectParams;
struct R_BatchGroupRectParams
{
  R_Handle tex;
  R_Tex2DSampleKind tex_sample_kind;
  Mat3x3F32 xform;
  Rng2F32 clip;
  F32 transparency;
};

typedef struct R_BatchGroupRectNode R_BatchGroupRectNode;
struct R_BatchGroupRectNode
{
  R_BatchGroupRectNode *next;
  R_BatchList batches;
  R_BatchGroupRectParams params;
};

typedef struct R_BatchGroupRectList R_BatchGroupRectList;
struct R_BatchGroupRectList
{
  R_BatchGroupRectNode *first;
  R_BatchGroupRectNode *last;
  U64 count;
};

typedef struct R_BatchGroup3DParams R_BatchGroup3DParams;
struct R_BatchGroup3DParams
{
  R_Handle          mesh_vertices;
  R_Handle          mesh_indices;
  U64               vertex_buffer_offset;
  U64               indice_buffer_offset;
  U64               indice_count;
  R_GeoTopologyKind mesh_geo_topology;
  R_GeoPolygonKind  mesh_geo_polygon;
  R_GeoVertexFlags  mesh_geo_vertex_flags;
  F32               line_width;

  // material index
  U64               mat_idx;
  // NOTE(k): do we need sample kind for other texture (e.g. normal, emissive)
  // textures
  R_Tex2DSampleKind diffuse_tex_sample_kind;
};

typedef struct R_BatchGroup2DParams R_BatchGroup2DParams;
struct R_BatchGroup2DParams
{
  R_Handle          vertices;
  R_Handle          indices;
  U64               vertex_buffer_offset;
  U64               indice_buffer_offset;
  U64               indice_count;
  R_GeoTopologyKind topology;
  R_GeoPolygonKind  polygon;
  R_GeoVertexFlags  vertex_flags;
  F32               line_width;

  R_Handle          tex;
  R_Tex2DSampleKind tex_sample_kind;
};

typedef struct R_BatchGroup2DMapNode R_BatchGroup2DMapNode;
struct R_BatchGroup2DMapNode
{
  R_BatchGroup2DMapNode *next;
  R_BatchGroup2DMapNode *insert_next;
  R_BatchGroup2DMapNode *insert_prev;
  U64 hash;
  R_BatchList batches;
  R_BatchGroup2DParams params;
};

typedef struct R_BatchGroup2DMap R_BatchGroup2DMap;
struct R_BatchGroup2DMap
{
  // hashmap
  R_BatchGroup2DMapNode **slots;
  U64 slots_count;

  // NOTE(k): list in insertion order
  R_BatchGroup2DMapNode *first;
  R_BatchGroup2DMapNode *last;
  U64 array_size;
};

typedef struct R_BatchGroup3DMapNode R_BatchGroup3DMapNode;
struct R_BatchGroup3DMapNode
{
  R_BatchGroup3DMapNode *next;
  R_BatchGroup3DMapNode *insert_next;
  R_BatchGroup3DMapNode *insert_prev;
  U64 hash;
  R_BatchList batches;
  R_BatchGroup3DParams params;
};

typedef struct R_BatchGroup3DMap R_BatchGroup3DMap;
struct R_BatchGroup3DMap
{
  // hashmap
  R_BatchGroup3DMapNode **slots;
  U64 slots_count;

  // NOTE(k): list in insertion order
  R_BatchGroup3DMapNode *first;
  R_BatchGroup3DMapNode *last;
  U64 array_size;
};

////////////////////////////////
//~ rjf: Pass Types

typedef struct R_PassParams_UI R_PassParams_UI;
struct R_PassParams_UI
{
  R_BatchGroupRectList rects;
};

typedef struct R_PassParams_Blur R_PassParams_Blur;
struct R_PassParams_Blur
{
  Rng2F32 rect;
  Rng2F32 clip;
  F32 blur_size;
  F32 corner_radii[Corner_COUNT];
};

typedef struct R_PassParams_Geo2D R_PassParams_Geo2D;
struct R_PassParams_Geo2D
{
  Rng2F32           viewport;
  Rng2F32           clip;
  Mat4x4F32         view;
  Mat4x4F32         projection;

  R_BatchGroup2DMap batches;
};

typedef struct R_PassParams_Geo3D R_PassParams_Geo3D;
struct R_PassParams_Geo3D
{
  Rng2F32           viewport;
  Rng2F32           clip;
  Mat4x4F32         view;
  Mat4x4F32         projection;
  R_BatchGroup3DMap mesh_batches;
  B32               omit_light;

  R_Light3D         *lights;
  U64               light_count;

  R_PackedTextures  *textures;
  R_Material3D      *materials;
  U64               material_count;

  // Debug purpose
  B32               omit_grid;
};

typedef struct R_Pass R_Pass;
struct R_Pass
{
    R_PassKind kind;
    union
    {
      void *params;
      R_PassParams_UI *params_ui;
      // R_PassParams_Blur *params_blur;
      R_PassParams_Geo2D *params_geo2d;
      R_PassParams_Geo3D *params_geo3d;
    };
};

typedef struct R_PassNode R_PassNode;
struct R_PassNode
{
  R_PassNode *next;
  R_Pass v;
};

typedef struct R_PassList R_PassList;
struct R_PassList
{
  R_PassNode *first;
  R_PassNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Handle Type Functions

internal R_Handle r_handle_zero(void);
internal B32 r_handle_match(R_Handle a, R_Handle b);

////////////////////////////////
//~ rjf: Batch Type Functions

internal R_BatchList r_batch_list_make(U64 instance_size);
internal void *r_batch_list_push_inst(Arena *arena, R_BatchList *list, U64 batch_inst_cap);

////////////////////////////////
//~ rjf: Pass Type Functions

internal R_Pass *r_pass_from_kind(Arena *arena, R_PassList *list, R_PassKind kind, B32 merge_pass);

////////////////////////////////
//~ rjf: Backend Hooks

//- rjf: top-level layer initialization
internal void r_init(const char* app_name, OS_Handle window, bool debug);

//- rjf: window setup/teardown
internal R_Handle r_window_equip(OS_Handle os_wnd);
// internal void r_window_unequip(OS_Handle window, R_Handle window_equip);

//- rjf: textures
internal R_Handle r_tex2d_alloc(R_ResourceKind kind, R_Tex2DSampleKind sample_kind, Vec2S32 size, R_Tex2DFormat format, void *data);
internal void r_tex2d_release(R_Handle texture);
// internal R_ResourceKind r_kind_from_tex2d(R_Handle texture);
// internal Vec2S32 r_size_from_tex2d(R_Handle texture);
// internal R_Tex2DFormat r_format_from_tex2d(R_Handle texture);
// internal void r_fill_tex2d_region(R_Handle texture, Rng2S32 subrect, void *data);

//- rjf: buffers
internal R_Handle r_buffer_alloc(R_ResourceKind kind, U64 size, void *data, U64 data_size);
internal void     r_buffer_release(R_Handle buffer);
// NOTE(k): this function should be called within r_frame boundary
internal void     r_buffer_copy(R_Handle buffer, void *data, U64 size);

//- rjf: frame markers
internal void r_begin_frame(void);
internal void r_end_frame(void);
internal void r_window_begin_frame(OS_Handle os_wnd, R_Handle window_equip);
internal U64  r_window_end_frame(R_Handle window_equip, Vec2F32 mouse_ptr);

//- rjf: render pass submission
internal void r_window_submit(OS_Handle os_wnd, R_Handle window_equip, R_PassList *passes);

#endif // RENDER_CORE_H
