#ifndef RENDER_CORE_H
#define RENDER_CORE_H

#define r_hook C_LINKAGE

////////////////////////////////
//~ rjf: Generated Code

#include "generated/render.meta.h"

////////////////////////////////
//~ Limits & Constants

// support max 2 rect pass per frame
#define R_MAX_RECT_PASS 2
#define R_MAX_RECT_GROUPS 6000
#define R_MAX_GEO2D_PASS 1
// support max 3 geo3d pass per frame
#define R_MAX_GEO3D_PASS 2
#define R_MAX_JOINTS_PER_PASS 3000
#define R_MAX_LIGHTS_PER_PASS 3000
#define R_MAX_MATERIALS_PER_PASS 3000
// inst count limits
#define R_MAX_RECT_INSTANCES 90000
#define R_MAX_MESH2D_INSTANCES 3000
#define R_MAX_MESH3D_INSTANCES 3000

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
//~ k: Geo3D Types

typedef enum R_Geo3D_TexKind
{
  R_Geo3D_TexKind_Ambient,
  R_Geo3D_TexKind_Emissive,
  R_Geo3D_TexKind_Diffuse,
  R_Geo3D_TexKind_Specular,
  R_Geo3D_TexKind_SpecularPower,
  R_Geo3D_TexKind_Normal,
  R_Geo3D_TexKind_Bump,
  R_Geo3D_TexKind_Opacity,
  R_Geo3D_TexKind_COUNT,
} R_Geo3D_TexKind;

typedef struct R_Geo3D_Vertex R_Geo3D_Vertex; 
struct R_Geo3D_Vertex
{
  Vec3F32 pos;  
  Vec3F32 nor;
  Vec2F32 tex;
  Vec3F32 tan;
  Vec4F32 col;
  Vec4U32 joints;
  Vec4F32 weights; // morph target weights
};

typedef struct R_Geo3D_Light R_Geo3D_Light;
struct R_Geo3D_Light
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

typedef struct R_Geo3D_Material R_Geo3D_Material;
struct R_Geo3D_Material
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

typedef struct R_Geo3D_PackedTextures R_Geo3D_PackedTextures;
struct R_Geo3D_PackedTextures
{
  R_Handle array[R_Geo3D_TexKind_COUNT];
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
  F32 line_thickness;
  F32 white_texture_override;
  F32 omit_texture;
  F32 has_pixel_id;
  Vec2F32 pixel_id;
  Vec4F32 line;
};

typedef struct R_Mesh2DInst R_Mesh2DInst;
struct R_Mesh2DInst
{
  Mat4x4F32 xform;
  Mat4x4F32 xform_inv;
  Vec2F32 key; // FIXME: rename to pixel_id
  B32 has_texture;
  Vec4F32 color;
  B32 has_color;
  B32 draw_edge;
};

typedef struct R_Mesh3DInst R_Mesh3DInst;
struct R_Mesh3DInst
{
  // TODO(k): kind a mess here, some attributes were sent to instance buffer, some were copied to storage buffer 
  Mat4x4F32 xform;
  Mat4x4F32 xform_inv;
  Vec2F32 key; // FIXME: rename to pixel_id
  U32 material_idx;
  B32 draw_edge;
  // NOTE(k): joint_xforms is stored in storage buffer instead of instance buffer
  Mat4x4F32 *joint_xforms;
  U32 joint_count;
  U32 first_joint;
  B32 depth_test;
  B32 omit_light;
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
  Vec2F32 viewport;
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
  R_Handle mesh_vertices;
  R_Handle mesh_indices;
  U64 vertex_buffer_offset;
  U64 indice_buffer_offset;
  U64 indice_count;
  R_GeoTopologyKind mesh_geo_topology;
  R_GeoPolygonKind mesh_geo_polygon;
  R_GeoVertexFlags mesh_geo_vertex_flags;
  F32 line_width;
  U64 mat_idx;
  R_Tex2DSampleKind diffuse_tex_sample_kind;
  // Mat4x4F32 xform;
};

typedef struct R_BatchGroup2DParams R_BatchGroup2DParams;
struct R_BatchGroup2DParams
{
  R_Handle vertices;
  R_Handle indices;
  U64 vertex_buffer_offset;
  U64 indice_buffer_offset;
  U64 indice_count;
  R_GeoTopologyKind topology;
  R_GeoPolygonKind polygon;
  R_GeoVertexFlags vertex_flags;
  F32 line_width;
  R_Handle tex;
  R_Tex2DSampleKind tex_sample_kind;
};

typedef struct R_BatchGroup2DMapNode R_BatchGroup2DMapNode;
struct R_BatchGroup2DMapNode
{
  R_BatchGroup2DMapNode *next;
  // FIXME: insertion order?
  R_BatchGroup2DMapNode *insert_next;
  R_BatchGroup2DMapNode *insert_prev;
  U64 hash;
  R_BatchList batches;
  R_BatchGroup2DParams params;
};

typedef struct R_BatchGroup2DMap R_BatchGroup2DMap;
struct R_BatchGroup2DMap
{
  R_BatchGroup2DMapNode **slots;
  U64 slots_count;
  // FIXME
  // NOTE(k): list in insertion order
  R_BatchGroup2DMapNode *first;
  R_BatchGroup2DMapNode *last;
  U64 array_size;
};

typedef struct R_BatchGroup3DMapNode R_BatchGroup3DMapNode;
struct R_BatchGroup3DMapNode
{
  R_BatchGroup3DMapNode *next;
  // FIXME: insertion order?
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
  // FIXME
  // NOTE(k): list in insertion order
  R_BatchGroup3DMapNode *first;
  R_BatchGroup3DMapNode *last;
  U64 array_size;
};

////////////////////////////////
//~ rjf: Pass Types

typedef struct R_PassParams_Rect R_PassParams_Rect;
struct R_PassParams_Rect
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

typedef struct R_PassParams_Noise R_PassParams_Noise;
struct R_PassParams_Noise
{
  Rng2F32 rect;
  Rng2F32 clip;
  F32 elapsed_secs;
};

typedef struct R_PassParams_Edge R_PassParams_Edge;
struct R_PassParams_Edge
{
  F32 elapsed_secs;
};

typedef struct R_PassParams_Crt R_PassParams_Crt;
struct R_PassParams_Crt
{
  F32 warp;
  F32 scan;
  F32 elapsed_secs;
};

typedef struct R_PassParams_Geo2D R_PassParams_Geo2D;
struct R_PassParams_Geo2D
{
  Rng2F32 viewport;
  Rng2F32 clip;
  Mat4x4F32 view;
  Mat4x4F32 projection;

  R_BatchGroup2DMap batches;
};

typedef struct R_PassParams_Geo3D R_PassParams_Geo3D;
struct R_PassParams_Geo3D
{
  Rng2F32 viewport;
  Rng2F32 clip;
  Mat4x4F32 view;
  Mat4x4F32 projection;
  R_BatchGroup3DMap mesh_batches;
  B32 omit_light;
  R_Geo3D_Light *lights;
  U64 light_count;
  R_Geo3D_PackedTextures *textures;
  R_Geo3D_Material *materials;
  U64 material_count;
  B32 omit_grid;
};

typedef struct R_Pass R_Pass;
struct R_Pass
{
    R_PassKind kind;
    union
    {
      void *params;
      R_PassParams_Rect *params_rect;
      R_PassParams_Blur *params_blur;
      R_PassParams_Noise *params_noise;
      R_PassParams_Edge *params_edge;
      R_PassParams_Crt *params_crt;
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
//~ rjf: Helpers

internal Mat4x4F32 r_sample_channel_map_from_tex2dformat(R_Tex2DFormat fmt);

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

internal R_Pass *r_pass_from_kind(Arena *arena, R_PassList *list, R_PassKind kind);

////////////////////////////////
//~ rjf: Backend Hooks

//- rjf: top-level layer initialization
r_hook void              r_init(OS_Handle window, B32 debug);

//- rjf: window setup/teardown
r_hook R_Handle          r_window_equip(OS_Handle window);
r_hook void              r_window_unequip(OS_Handle window, R_Handle window_equip);

//- rjf: textures
r_hook R_Handle          r_tex2d_alloc(R_ResourceKind kind, R_Tex2DSampleKind sample_kind, Vec2S32 size, R_Tex2DFormat format, void *data);
r_hook void              r_tex2d_release(R_Handle texture);
r_hook R_ResourceKind    r_kind_from_tex2d(R_Handle texture);
r_hook Vec2S32           r_size_from_tex2d(R_Handle texture);
r_hook R_Tex2DFormat     r_format_from_tex2d(R_Handle texture);
r_hook void              r_fill_tex2d_region(R_Handle texture, Rng2S32 subrect, void *data);

//- rjf: buffers
r_hook R_Handle          r_buffer_alloc(R_ResourceKind kind, U64 cap, void *data, U64 data_size);
r_hook internal void     r_buffer_copy(R_Handle buffer, void *data, U64 size);
r_hook void              r_buffer_release(R_Handle buffer);

//- rjf: frame markers
r_hook void              r_begin_frame(void);
r_hook void              r_end_frame(void);
r_hook void              r_window_begin_frame(OS_Handle window, R_Handle window_equip);
r_hook Vec2F32           r_window_end_frame(OS_Handle window, R_Handle window_equip, Vec2F32 mouse_ptr);

//- rjf: render pass submission
r_hook void              r_window_submit(OS_Handle window, R_Handle window_equip, R_PassList *passes);

#endif // RENDER_CORE_H
