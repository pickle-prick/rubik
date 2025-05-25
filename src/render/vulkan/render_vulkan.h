#ifndef RENDER_VULKAN_H
#define RENDER_VULKAN_H

#include "generated/render_vulkan.meta.h"

#define VK_Assert(result) \
  do { \
    if((result) != VK_SUCCESS) { \
      fprintf(stderr, "[VK_ERROR] [CODE: %d] in (%s)(%s:%d)\n", result, __FUNCTION__, __FILE__, __LINE__); \
      Trap(); \
      exit(EXIT_FAILURE); \
    } \
  } while (0)

/////////////////////////////////////////////////////////////////////////////////////////
//~ Some limits/constants

// Syncronization
// We choose the number 2 here because we don't want the CPU to get too far
// ahead of the GPU With 2 frames in flight, the CPU and the GPU can be working
// on their own tasks at the same time If the CPU finishes early, it will wait
// till the GPU finishes rendering before submitting more work With 3 or more
// frames in flight, the CPU could get ahead of the GPU, because the work load
// of the GPU could be too larger for it to handle, so the CPU would end up
// waiting a lot, adding frames of latency Generally extra latency isn't desired
#define MAX_FRAMES_IN_FLIGHT 1
// support max 4K with 32x32 sized tile
#define TILE_SIZE 32
#define MAX_TILES_PER_PASS ((3840*2160)/(TILE_SIZE*TILE_SIZE))
#define MAX_LIGHTS_PER_TILE 200

/////////////////////////////////////////////////////////////////////////////////////////
//~ Enums

typedef enum R_Vulkan_VShadKind
{
  R_Vulkan_VShadKind_Rect,
  // R_Vulkan_VShadKind_Blur,
  R_Vulkan_VShadKind_Geo2D_Forward,
  R_Vulkan_VShadKind_Geo2D_Composite,
  R_Vulkan_VShadKind_Geo3D_ZPre,
  R_Vulkan_VShadKind_Geo3D_Debug,
  R_Vulkan_VShadKind_Geo3D_Forward,
  R_Vulkan_VShadKind_Geo3D_Composite,
  R_Vulkan_VShadKind_Finalize,
  R_Vulkan_VShadKind_COUNT,
} R_Vulkan_VShadKind;

typedef enum R_Vulkan_FShadKind
{
  R_Vulkan_FShadKind_Rect,
  // R_Vulkan_FShadKind_Blur,
  R_Vulkan_FShadKind_Geo2D_Forward,
  R_Vulkan_FShadKind_Geo2D_Composite,
  R_Vulkan_FShadKind_Geo3D_ZPre,
  R_Vulkan_FShadKind_Geo3D_Debug,
  R_Vulkan_FShadKind_Geo3D_Forward,
  R_Vulkan_FShadKind_Geo3D_Composite,
  R_Vulkan_FShadKind_Finalize,
  R_Vulkan_FShadKind_COUNT,
} R_Vulkan_FShadKind;

typedef enum R_Vulkan_CShadKind
{
  R_Vulkan_CShadKind_Geo3D_TileFrustum,
  R_Vulkan_CShadKind_Geo3D_LightCulling,
  R_Vulkan_CShadKind_COUNT,
} R_Vulkan_CShadKind;

typedef enum R_Vulkan_UBOTypeKind
{
  R_Vulkan_UBOTypeKind_Rect,
  // R_Vulkan_UBOTypeKind_Blur,
  R_Vulkan_UBOTypeKind_Geo2D,
  R_Vulkan_UBOTypeKind_Geo3D,
  R_Vulkan_UBOTypeKind_Geo3D_TileFrustum,
  R_Vulkan_UBOTypeKind_Geo3D_LightCulling,
  R_Vulkan_UBOTypeKind_COUNT,
} R_Vulkan_UBOTypeKind;

typedef enum R_Vulkan_SBOTypeKind
{
  R_Vulkan_SBOTypeKind_Geo3D_Joints,
  R_Vulkan_SBOTypeKind_Geo3D_Materials,
  R_Vulkan_SBOTypeKind_Geo3D_Tiles,
  R_Vulkan_SBOTypeKind_Geo3D_Lights,
  R_Vulkan_SBOTypeKind_Geo3D_LightIndices,
  R_Vulkan_SBOTypeKind_Geo3D_TileLights,
  R_Vulkan_SBOTypeKind_COUNT,
} R_Vulkan_SBOTypeKind;

typedef enum R_Vulkan_DescriptorSetKind
{
  R_Vulkan_DescriptorSetKind_Tex2D,
  // ui
  R_Vulkan_DescriptorSetKind_UBO_Rect,
  // 2d
  R_Vulkan_DescriptorSetKind_UBO_Geo2D,
  // 3d
  R_Vulkan_DescriptorSetKind_UBO_Geo3D,
  R_Vulkan_DescriptorSetKind_UBO_Geo3D_TileFrustum,
  R_Vulkan_DescriptorSetKind_UBO_Geo3D_LightCulling,
  R_Vulkan_DescriptorSetKind_SBO_Geo3D_Joints,
  R_Vulkan_DescriptorSetKind_SBO_Geo3D_Materials,
  R_Vulkan_DescriptorSetKind_SBO_Geo3D_Tiles,
  R_Vulkan_DescriptorSetKind_SBO_Geo3D_Lights,
  R_Vulkan_DescriptorSetKind_SBO_Geo3D_LightIndices,
  R_Vulkan_DescriptorSetKind_SBO_Geo3D_TileLights,
  R_Vulkan_DescriptorSetKind_COUNT,
} R_Vulkan_DescriptorSetKind;

typedef enum R_Vulkan_PipelineKind
{
  // gfx pipeline
  R_Vulkan_PipelineKind_GFX_Rect,
  // R_Vulkan_PipelineKind_Blur,
  // 2d
  R_Vulkan_PipelineKind_GFX_Geo2D_Forward,
  R_Vulkan_PipelineKind_GFX_Geo2D_Composite,
  // 3d
  R_Vulkan_PipelineKind_GFX_Geo3D_ZPre,
  R_Vulkan_PipelineKind_GFX_Geo3D_Debug,
  R_Vulkan_PipelineKind_GFX_Geo3D_Forward,
  R_Vulkan_PipelineKind_GFX_Geo3D_Composite,
  R_Vulkan_PipelineKind_GFX_Finalize,
  // compute pipeline
  R_Vulkan_PipelineKind_CMP_Geo3D_TileFrustum,
  R_Vulkan_PipelineKind_CMP_Geo3D_LightCulling,
  R_Vulkan_PipelineKind_COUNT,
} R_Vulkan_PipelineKind;

typedef enum R_Vulkan_RenderPassKind
{
  R_Vulkan_RenderPassKind_Rect,
  // R_Vulkan_RenderPassKind_Blur,
  R_Vulkan_RenderPassKind_Geo2D,
  R_Vulkan_RenderPassKind_Geo2D_Composite,
  R_Vulkan_RenderPassKind_Geo3D_ZPre,
  R_Vulkan_RenderPassKind_Geo3D,
  R_Vulkan_RenderPassKind_Geo3D_Composite,
  R_Vulkan_RenderPassKind_Finalize,
  R_Vulkan_RenderPassKind_COUNT,
} R_Vulkan_RenderPassKind;

typedef enum R_Vulkan_Light3DKind
{
  R_Vulkan_Light3DKind_Directional,
  R_Vulkan_Light3DKind_Point,
  R_Vulkan_Light3DKind_Spot,
  R_Vulkan_Light3DKind_COUNT,
} R_Vulkan_Light3DKind;

/////////////////////////////////////////////////////////////////////////////////////////
//~ Some basic types

typedef struct R_Vulkan_Plane R_Vulkan_Plane;
struct R_Vulkan_Plane
{
  Vec3F32 N; // plane normal
  F32 d; // distance to the origin
};

typedef struct R_Vulkan_Frustum R_Vulkan_Frustum;
struct R_Vulkan_Frustum
{
  R_Vulkan_Plane planes[6];
};

/////////////////////////////////////////////////////////////////////////////////////////
//~ C-side Shader Types (ubo,push,sbo)

// ubo
typedef struct R_Vulkan_UBO_Rect R_Vulkan_UBO_Rect;
struct R_Vulkan_UBO_Rect
{
  Vec2F32 viewport_size;
  F32 opacity;
  F32 _padding0_;
  Vec4F32 texture_sample_channel_map[4];
  Vec2F32 texture_t2d_size;
  Vec2F32 translate;
  Vec4F32 xform[3];
  Vec2F32 xform_scale;
  F32 _padding1_[2];
};

// typedef struct R_D3D11_Uniforms_BlurPass R_D3D11_Uniforms_BlurPass;
// struct R_D3D11_Uniforms_BlurPass
// {
//   Rng2F32 rect;
//   Vec4F32 corner_radii;
//   Vec2F32 direction;
//   Vec2F32 viewport_size;
//   U32 blur_count;
//   U8 _padding0_[204];
// };
// StaticAssert(sizeof(R_D3D11_Uniforms_BlurPass) % 256 == 0, NotAligned); // constant count/offset must be aligned to 256 bytes

// typedef struct R_D3D11_Uniforms_Blur R_D3D11_Uniforms_Blur;
// struct R_D3D11_Uniforms_Blur
// {
//   R_D3D11_Uniforms_BlurPass passes[Axis2_COUNT];
//   Vec4F32 kernel[32];
// };

typedef struct R_Vulkan_UBO_Geo2D R_Vulkan_UBO_Geo2D;
struct R_Vulkan_UBO_Geo2D
{
  Mat4x4F32 proj;
  Mat4x4F32 proj_inv;
  Mat4x4F32 view;
  Mat4x4F32 view_inv;
};

typedef struct R_Vulkan_UBO_Geo3D R_Vulkan_UBO_Geo3D;
struct R_Vulkan_UBO_Geo3D
{
  Mat4x4F32 view;
  Mat4x4F32 view_inv;
  Mat4x4F32 proj;
  Mat4x4F32 proj_inv;

  // Debug
  U32       show_grid;
  U32       _padding_0[3];
};

typedef struct R_Vulkan_UBO_Geo3D_TileFrustum R_Vulkan_UBO_Geo3D_TileFrustum;
struct R_Vulkan_UBO_Geo3D_TileFrustum
{
  Mat4x4F32 proj_inv;
  Vec2U32 grid_size;
  U32 _padding_0[2];
};

typedef struct R_Vulkan_UBO_Geo3D_LightCulling R_Vulkan_UBO_Geo3D_LightCulling;
struct R_Vulkan_UBO_Geo3D_LightCulling
{
  Mat4x4F32 proj_inv;
  U32 light_count;
  U32 _padding_0[3];
};

// push

typedef struct R_Vulkan_PUSH_Geo3D_Forward R_Vulkan_PUSH_Geo3D_Forward;
struct R_Vulkan_PUSH_Geo3D_Forward
{
  Vec2F32 viewport;
  Vec2U32 light_grid_size;
};

typedef struct R_Vulkan_PUSH_Post R_Vulkan_PUSH_Post;
struct R_Vulkan_PUSH_Post
{
  Vec2F32 resolution;
  Vec2F32 mouse;
  F32 time;
  U32 _padding_0[3];
};

// sbo

#define R_Vulkan_SBO_Geo3D_Joint Mat4x4F32

typedef struct R_Vulkan_SBO_Geo3D_Tile R_Vulkan_SBO_Geo3D_Tile;
struct R_Vulkan_SBO_Geo3D_Tile
{
  R_Vulkan_Frustum frustum;
};

#define R_Vulkan_SBO_Geo3D_Light R_Light3D

// NOTE(k): first element will be used as indice_count
// NOTE(K): we are using std140, so packed it to 16 bytes boundary
#define R_Vulkan_SBO_Geo3D_LightIndice U32[4]

typedef struct R_Vulkan_SBO_Geo3D_TileLights R_Vulkan_SBO_Geo3D_TileLights;
struct R_Vulkan_SBO_Geo3D_TileLights
{
  U32 offset;  
  U32 light_count;
  F32 _padding_0[2];
};

#define R_Vulkan_SBO_Geo3D_Material R_Material3D

/////////////////////////////////////////////////////////////////////////////////////////
// Vulkan types

#define MAX_SURFACE_FORMAT_COUNT 9
#define MAX_SURFACE_PRESENT_MODE_COUNT 9
typedef struct
{
  VkSurfaceKHR             h;
  VkSurfaceCapabilitiesKHR caps;

  U32                      format_count;
  VkSurfaceFormatKHR       formats[MAX_SURFACE_FORMAT_COUNT];

  U32                      prest_mode_count;
  VkPresentModeKHR         prest_modes[MAX_SURFACE_PRESENT_MODE_COUNT];
} R_Vulkan_Surface;

typedef struct
{
  VkImage        h;
  VkFormat       format;
  VkDeviceMemory memory;
  VkImageView    view;
  VkExtent2D     extent;
} R_Vulkan_Image;

#define MAX_IMAGE_COUNT 6
typedef struct
{
  VkSwapchainKHR       h;
  VkExtent2D           extent;
  VkFormat             format;
  VkColorSpaceKHR      color_space;

  U32                  image_count;
  VkImage              images[MAX_IMAGE_COUNT];
  VkImageView          image_views[MAX_IMAGE_COUNT];
} R_Vulkan_Swapchain;

typedef struct
{
  VkPhysicalDevice                 h;
  VkPhysicalDeviceProperties       properties;
  VkPhysicalDeviceMemoryProperties mem_properties;
  VkPhysicalDeviceFeatures         features;
  U32                              gfx_queue_family_index;
  U32                              cmp_queue_family_index;
  U32                              prest_queue_family_index;
  VkFormat                         depth_image_format;
} R_Vulkan_GPU;

typedef struct
{
  VkDevice h;
  VkQueue  gfx_queue;
  VkQueue  prest_queue;
} R_Vulkan_Device;

typedef struct R_Vulkan_Pipeline R_Vulkan_Pipeline;
struct R_Vulkan_Pipeline
{
  R_Vulkan_PipelineKind kind;
  VkPipeline            h;
  VkPipelineLayout      layout;
};

typedef struct R_Vulkan_Buffer R_Vulkan_Buffer;
struct R_Vulkan_Buffer
{
  R_Vulkan_Buffer *next;
  U64             generation;

  VkBuffer        h;
  VkDeviceMemory  memory;

  R_ResourceKind  kind;
  U64             size;
  U64             cap;

  VkBuffer        staging;
  VkDeviceMemory  staging_memory;
  // NOTE(k): refer to staging buffer if staging is present
  void            *mapped;
};

typedef struct R_Vulkan_DescriptorSetLayout R_Vulkan_DescriptorSetLayout ;
struct R_Vulkan_DescriptorSetLayout
{
  VkDescriptorSetLayoutBinding *bindings;
  U64                          binding_count;
  VkDescriptorSetLayout        h;
};

typedef struct R_Vulkan_DescriptorSetPool R_Vulkan_DescriptorSetPool;
struct R_Vulkan_DescriptorSetPool
{
  R_Vulkan_DescriptorSetPool *next;
  R_Vulkan_DescriptorSetKind kind;

  VkDescriptorPool           h;
  U64                        cmt;
  U64                        cap;
};

// TODO(XXX): revisit is needed
typedef struct R_Vulkan_RenderPass R_Vulkan_RenderPass;
struct R_Vulkan_RenderPass
{
  VkRenderPass h;

  union
  {
    R_Vulkan_Pipeline rect;
    struct
    {
      R_Vulkan_Pipeline forward[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT];
    } geo2d;
    R_Vulkan_Pipeline geo2d_composite;
    // NOTE(k): we are forced to use a dedicated renderpass for z pre
    //          since compute shader can only run outside of the renderpass
    R_Vulkan_Pipeline z_pre[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT];
    // NOTE(k): geo3d renderpass use multiple pipelines
    struct
    {
      R_Vulkan_Pipeline tile_frustum;
      R_Vulkan_Pipeline light_culling;
      R_Vulkan_Pipeline debug[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT];
      R_Vulkan_Pipeline forward[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT];
    } geo3d;
    R_Vulkan_Pipeline geo3d_composite;
    R_Vulkan_Pipeline finalize;

    R_Vulkan_Pipeline v[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT * 2 + 2];
  } pipelines;
  U64 pipeline_count;
};

typedef struct R_Vulkan_RenderPassGroup R_Vulkan_RenderPassGroup;
struct R_Vulkan_RenderPassGroup
{
  R_Vulkan_RenderPassGroup *next;
  R_Vulkan_RenderPass      passes[R_Vulkan_RenderPassKind_COUNT];
};

typedef struct R_Vulkan_DescriptorSet R_Vulkan_DescriptorSet;
struct R_Vulkan_DescriptorSet
{
  VkDescriptorSet  h;
  R_Vulkan_DescriptorSetPool *pool;
};

typedef struct R_Vulkan_UBOBuffer R_Vulkan_UBOBuffer;
struct R_Vulkan_UBOBuffer
{
  R_Vulkan_Buffer        buffer;
  R_Vulkan_DescriptorSet set;
  U64                    unit_count;
  U64                    stride;
};

typedef struct R_Vulkan_SBOBuffer R_Vulkan_SBOBuffer;
struct R_Vulkan_SBOBuffer
{
  R_Vulkan_Buffer        buffer;
  R_Vulkan_DescriptorSet set;
  U64                    unit_count;
  U64                    stride;
};

typedef struct R_Vulkan_Tex2D R_Vulkan_Tex2D;
struct R_Vulkan_Tex2D
{
  R_Vulkan_Tex2D         *next;
  U64                    generation;

  R_Vulkan_DescriptorSet desc_set;
  R_Vulkan_Image         image;
  R_Tex2DFormat          format;
  R_Tex2DSampleKind      sample_kind;
};

// TODO(k): find a better name
typedef struct R_Vulkan_Bag R_Vulkan_Bag;
struct R_Vulkan_Bag
{
  R_Vulkan_Bag           *next;

  // NOTE(k): If swapchain image format changed, we would need to recreate renderpass and pipeline
  //          If only extent of swapchain image changed, we could just recrete the swapchain
  R_Vulkan_Swapchain     swapchain;

  R_Vulkan_Image         stage_color_image;
  R_Vulkan_DescriptorSet stage_color_ds;
  R_Vulkan_Image         stage_id_image;
  R_Vulkan_Buffer        stage_id_cpu;
  // 2d (we need 2 images for ping-pong)
  R_Vulkan_Image         geo2d_color_image;
  R_Vulkan_DescriptorSet geo2d_color_ds;
  // 3d
  R_Vulkan_Image         geo3d_color_image;
  R_Vulkan_DescriptorSet geo3d_color_ds;
  R_Vulkan_Image         geo3d_normal_depth_image;
  R_Vulkan_DescriptorSet geo3d_normal_depth_ds;
  R_Vulkan_Image         geo3d_depth_image;
  R_Vulkan_Image         geo3d_pre_depth_image;
  R_Vulkan_DescriptorSet geo3d_pre_depth_ds;

  // NOTE(k): last renderpass (finalize) contains mutiple framebuffer (count is equal to swapchain.image_count)
  VkFramebuffer          framebuffers[R_Vulkan_RenderPassKind_COUNT + MAX_IMAGE_COUNT - 1];
};

typedef struct
{
  VkSemaphore              img_acq_sem;
  VkSemaphore              rend_comp_sem;
  VkFence                  inflt_fence;
  U32                      img_idx;
  VkCommandBuffer          cmd_buf;

  // ref
  R_Vulkan_Bag             *bag_ref;
  R_Vulkan_RenderPassGroup *rendpass_grp_ref;

  // UBO buffer and descriptor set
  R_Vulkan_UBOBuffer       ubo_buffers[R_Vulkan_UBOTypeKind_COUNT];

  // Storage buffer and descriptor set
  R_Vulkan_SBOBuffer       sbo_buffers[R_Vulkan_SBOTypeKind_COUNT];

  // Instance buffer
  R_Vulkan_Buffer          inst_buffer_rect[MAX_RECT_PASS];
  R_Vulkan_Buffer          inst_buffer_mesh2d[MAX_GEO2D_PASS];
  R_Vulkan_Buffer          inst_buffer_mesh3d[MAX_GEO3D_PASS];
} R_Vulkan_Frame;


typedef struct R_Vulkan_Window R_Vulkan_Window;
struct R_Vulkan_Window
{
  Arena                    *arena;

  // allocation link
  U64                      generation;
  R_Vulkan_Window          *next;

  OS_Handle                os_wnd;

  R_Vulkan_Surface         surface;
  R_Vulkan_Bag             *bag;

  // TODO(k): we could store a table of renderpass and pipelines
  //           renderpass and pipelines can be reused if color and depth format are the same
  R_Vulkan_RenderPassGroup *rendpass_grp;

  // These resources can be reused
  R_Vulkan_Frame           frames[MAX_FRAMES_IN_FLIGHT];
  U64                      curr_frame_idx;

  R_Vulkan_Bag             *first_free_bag;
  R_Vulkan_Bag             *first_to_free_bag;
  R_Vulkan_RenderPassGroup *first_free_rendpass_grp;
  R_Vulkan_RenderPassGroup *first_to_free_rendpass_grp;
};

typedef struct R_Vulkan_State R_Vulkan_State;
struct R_Vulkan_State
{
  bool                                debug;
  VkDebugUtilsMessengerEXT            debug_messenger;
  // Dynamic loaded functions
  PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

  Arena                               *arena;
  R_Vulkan_Window                     *first_free_window;
  R_Vulkan_Tex2D                      *first_free_tex2d;
  R_Vulkan_Buffer                     *first_free_buffer;

  VkInstance                          instance;
  R_Vulkan_Device                     device;
  R_Vulkan_GPU                        gpu;

  VkSampler                           samplers[R_Tex2DSampleKind_COUNT];
  R_Vulkan_DescriptorSetLayout        set_layouts[R_Vulkan_DescriptorSetKind_COUNT];

  VkShaderModule                      vshad_modules[R_Vulkan_VShadKind_COUNT];
  VkShaderModule                      fshad_modules[R_Vulkan_FShadKind_COUNT];
  VkShaderModule                      cshad_modules[R_Vulkan_CShadKind_COUNT];

  VkCommandPool                       cmd_pool;
  // For copying staging buffer or transition image layout
  VkCommandBuffer                     oneshot_cmd_buf;

  // resource free list
  /////////////////////////////////////////////////////////////////////////////////////

  R_Vulkan_DescriptorSetPool          *first_avail_ds_pool[R_Vulkan_DescriptorSetKind_COUNT];
  // TODO(k): first_free_descriptor, we could update descriptor to point a new buffer or image/sampler
  // TODO(k): we may want to keep track of filled ds_pool

  R_Handle                            backup_texture;

  R_Vulkan_DeclStackNils;
  R_Vulkan_DeclStacks;
};

/////////////////////////////////////////////////////////////////////////////////////////
//~ Globals

global R_Vulkan_State *r_vulkan_state = 0;

/////////////////////////////////////////////////////////////////////////////////////////
//~ Helpers

internal R_Vulkan_Window          *r_vulkan_window_from_handle(R_Handle handle);
internal void                     r_vulkan_window_resize(R_Vulkan_Window *window);
internal R_Handle                 r_vulkan_handle_from_window(R_Vulkan_Window *window);
internal R_Vulkan_Tex2D           *r_vulkan_tex2d_from_handle(R_Handle handle);
internal R_Handle                 r_vulkan_handle_from_tex2d(R_Vulkan_Tex2D *texture);
internal R_Vulkan_Buffer          *r_vulkan_buffer_from_handle(R_Handle handle);
internal R_Handle                 r_vulkan_handle_from_buffer(R_Vulkan_Buffer *buffer);
internal S32                      r_vulkan_memory_index_from_type_filer(U32 type_bits, VkMemoryPropertyFlags properties);
// internal ID3D11Buffer *r_vulkan_instance_buffer_from_size(U64 size);
// internal void r_usage_access_flags_from_resource_kind(R_ResourceKind kind, D3D11_USAGE *out_vulkan_usage, UINT *out_cpu_access_flags);
internal void                     r_vulkan_format_for_swapchain(VkSurfaceFormatKHR *formats, U64 count, VkFormat *format, VkColorSpaceKHR *color_space);
internal VkFormat                 r_vulkan_dep_format_optimal();
internal void                     r_vulkan_surface_update(R_Vulkan_Surface *surface);
internal R_Vulkan_Bag             *r_vulkan_bag(R_Vulkan_Window *window, R_Vulkan_Surface *surface, R_Vulkan_Bag *old_bag);
internal R_Vulkan_Swapchain       r_vulkan_swapchain(R_Vulkan_Surface *surface, OS_Handle os_wnd, VkFormat format, VkColorSpaceKHR color_space, R_Vulkan_Swapchain *old);
internal void                     r_vulkan_bag_destroy(R_Vulkan_Bag *bag);
internal R_Vulkan_RenderPassGroup *r_vulkan_rendpass_grp(R_Vulkan_Window *window, VkFormat color_format, R_Vulkan_RenderPassGroup *old);
internal void                     r_vulkan_rendpass_grp_destroy(R_Vulkan_RenderPassGroup *grp);
internal R_Vulkan_Pipeline        r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind kind, R_GeoTopologyKind topology, R_GeoPolygonKind polygon, VkRenderPass renderpass, R_Vulkan_Pipeline *old);
internal R_Vulkan_Pipeline        r_vulkan_cmp_pipeline(R_Vulkan_PipelineKind kind);
internal void                     r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind kind, U64 set_count, U64 cap, VkBuffer *buffers, VkImageView *image_views, VkSampler *sampler, R_Vulkan_DescriptorSet *sets);
internal void                     r_vulkan_descriptor_set_destroy(R_Vulkan_DescriptorSet *set);
internal void                     r_vulkan_rendpass_grp_submit(R_Vulkan_Bag *bag, R_Vulkan_RenderPassGroup *grp);
internal R_Vulkan_UBOBuffer       r_vulkan_ubo_buffer_alloc(R_Vulkan_UBOTypeKind kind, U64 unit_count);
internal R_Vulkan_SBOBuffer       r_vulkan_sbo_buffer_alloc(R_Vulkan_SBOTypeKind kind, U64 unit_count);
internal VKAPI_ATTR               VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_userdata);
internal VkFence                  r_vulkan_fence(VkDevice device);
internal VkSemaphore              r_vulkan_semaphore(VkDevice device);
internal void                     r_vulkan_cmd_begin(VkCommandBuffer cmd_buf);
internal void                     r_vulkan_cmd_end(VkCommandBuffer cmd_buf);
internal void                     r_vulkan_image_transition(VkCommandBuffer cmd_buf, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage, VkAccessFlags src_access_flag, VkPipelineStageFlags dst_stage, VkAccessFlags dst_access_flag);
internal void                     r_vulkan_descriptor_pool_alloc();
internal VkSampler                r_vulkan_sampler2d(R_Tex2DSampleKind kind);

#define CmdScope(c) DeferLoop((r_vulkan_cmd_begin((c))), r_vulkan_cmd_end((c)))
#define FileReadAll(arena, fp, return_data, return_size)                                 \
  do {                                                                                   \
    OS_Handle f = os_file_open(OS_AccessFlag_Read, (fp));                                \
    FileProperties f_props = os_properties_from_file(f);                                 \
    *return_size = f_props.size;                                                         \
    *return_data = push_array(arena, U8, f_props.size);                                  \
    os_file_read(f, rng_1u64(0,f_props.size), *return_data);                             \
  } while (0);

#endif
