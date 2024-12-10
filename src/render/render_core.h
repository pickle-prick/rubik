#ifndef RENDER_CORE_H
#define RENDER_CORE_H

////////////////////////////////
//~ rjf: Enums

typedef U32 R_GeoVertexFlags;
enum {
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

typedef enum R_PassKind
{
    R_PassKind_UI,
    // R_PassKind_Blur,
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
    Vec3F32 col;
    Vec4U32 joints;
    Vec4F32 weights;
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

typedef struct R_Mesh3DInst R_Mesh3DInst;
struct R_Mesh3DInst
{
    // TODO(k): kind a mess here, some attributes were sent to instance buffer, some were copied to storage buffer 
    // NOTE: Only these two sent to the instance buffer
    Mat4x4F32 xform;
    U64       key;
    Vec4F32   color_texture;

    // TODO(k): material idx, a primitive could have array of materials
    Mat4x4F32 *joint_xforms;
    U32       joint_count;
    U32       first_joint; // TODO: Set it in render side, quite ugly, fix it later
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

typedef struct R_BatchGroup2DParams R_BatchGroup2DParams;
struct R_BatchGroup2DParams
{
    R_Handle tex;
    R_Tex2DSampleKind tex_sample_kind;
    Mat3x3F32 xform;
    Rng2F32 clip;
    F32 transparency;
};

typedef struct R_BatchGroup2DNode R_BatchGroup2DNode;
struct R_BatchGroup2DNode
{
    R_BatchGroup2DNode *next;
    R_BatchList batches;
    R_BatchGroup2DParams params;
};

typedef struct R_BatchGroup2DList R_BatchGroup2DList;
struct R_BatchGroup2DList
{
    R_BatchGroup2DNode *first;
    R_BatchGroup2DNode *last;
    U64 count;
};

typedef struct R_BatchGroup3DParams R_BatchGroup3DParams;
struct R_BatchGroup3DParams
{
    R_Handle          mesh_vertices;
    R_Handle          mesh_indices;
    R_GeoTopologyKind mesh_geo_topology;
    R_GeoPolygonKind  mesh_geo_polygon;
    R_GeoVertexFlags  mesh_geo_vertex_flags;
    R_Handle          albedo_tex;
    R_Tex2DSampleKind albedo_tex_sample_kind;
    // TODO: do we need this?
    Mat4x4F32         xform;
};

typedef struct R_BatchGroup3DMapNode R_BatchGroup3DMapNode;
struct R_BatchGroup3DMapNode
{
    R_BatchGroup3DMapNode *next;
    U64 hash;
    R_BatchList batches;
    R_BatchGroup3DParams params;
};

typedef struct R_BatchGroup3DMap R_BatchGroup3DMap;
struct R_BatchGroup3DMap
{
    R_BatchGroup3DMapNode **slots;
    U64 slots_count;
};

////////////////////////////////
//~ rjf: Pass Types

typedef struct R_PassParams_UI R_PassParams_UI;
struct R_PassParams_UI
{
    R_BatchGroup2DList rects;
};

// typedef struct R_PassParams_Blur R_PassParams_Blur;
// struct R_PassParams_Blur {
//         Rng2F32 rect;
//         Rng2F32 clip;
//         F32 blur_size;
//         F32 corner_radii[Corner_COUNT];
// };

typedef struct R_PassParams_Geo3D R_PassParams_Geo3D;
struct R_PassParams_Geo3D
{
    Rng2F32           viewport;
    Rng2F32           clip;
    Mat4x4F32         view;
    Mat4x4F32         projection;
    R_BatchGroup3DMap mesh_batches;
    Vec3F32           global_light;

    // Debug usage
    Vec4F32           gizmos_origin;
    Mat4x4F32         gizmos_xform;
    B32               show_grid;
    B32               show_gizmos;
};

typedef struct R_Pass R_Pass;
struct R_Pass
{
    R_PassKind kind;
    union {
        void *params;
        R_PassParams_UI *params_ui;
        // R_PassParams_Blur *params_blur;
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
internal void *r_batch_list_push_inst(Arena *arena, R_BatchList *list,
                                      U64 batch_inst_cap);

////////////////////////////////
//~ rjf: Pass Type Functions

// internal R_Pass *r_pass_from_kind(Arena *arena, R_PassList *list,
//                                   R_PassKind kind);

////////////////////////////////
//~ rjf: Backend Hooks

//- rjf: top-level layer initialization
internal void r_init(const char* app_name, OS_Handle window, bool debug);

//- rjf: window setup/teardown
// internal R_Handle r_window_equip(OS_Handle window);
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
internal R_Handle r_buffer_alloc(R_ResourceKind kind, U64 size, void *data);
// internal void r_buffer_release(R_Handle buffer);

//- rjf: frame markers
internal void r_begin_frame(void);
internal void r_end_frame(void);
// internal void r_window_begin_frame(OS_Handle window, R_Handle window_equip);
// internal void r_window_end_frame(OS_Handle window, R_Handle window_equip);
internal U64  r_window_begin_frame(OS_Handle os_wnd, R_Handle window_equip);
internal void r_window_end_frame(OS_Handle os_wnd, R_Handle window_equip);

//- rjf: render pass submission
internal void r_window_submit(OS_Handle os_wnd, R_Handle window_equip, R_PassList *passes, Vec2F32 ptr);

#endif // RENDER_CORE_H
