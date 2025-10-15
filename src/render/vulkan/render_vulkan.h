#ifndef RENDER_VULKAN_H
#define RENDER_VULKAN_H

////////////////////////////////
//~ Generated Includes

#include "generated/render_vulkan.meta.h"

////////////////////////////////
//~ Limits & Constants

// Syncronization
// We choose the number 2 here because we don't want the CPU to get too far
// ahead of the GPU With 2 frames in flight, the CPU and the GPU can be working
// on their own tasks at the same time If the CPU finishes early, it will wait
// till the GPU finishes rendering before submitting more work With 3 or more
// frames in flight, the CPU could get ahead of the GPU, because the work load
// of the GPU could be too larger for it to handle, so the CPU would end up
// waiting a lot, adding frames of latency Generally extra latency isn't desired
#define R_VULKAN_MAX_FRAMES_IN_FLIGHT 1
#define R_VULKAN_STAGING_IN_FLIGHT_COUNT 2
// support max 4K with 32x32 sized tile
#define R_VULKAN_TILE_SIZE 32
#define R_VULKAN_MAX_TILES_PER_PASS ((3840*2160)/(R_VULKAN_TILE_SIZE*R_VULKAN_TILE_SIZE))
#define R_VULKAN_MAX_LIGHTS_PER_TILE 200

////////////////////////////////
//~ Enums

typedef enum R_Vulkan_VShadKind
{
  R_Vulkan_VShadKind_Rect,
  // R_Vulkan_VShadKind_Blur,
  R_Vulkan_VShadKind_Noise,
  R_Vulkan_VShadKind_Edge,
  R_Vulkan_VShadKind_Crt,
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
  R_Vulkan_FShadKind_Noise,
  R_Vulkan_FShadKind_Edge,
  R_Vulkan_FShadKind_Crt,
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
  // R_Vulkan_PipelineKind_GFX_Blur,
  R_Vulkan_PipelineKind_GFX_Noise,
  R_Vulkan_PipelineKind_GFX_Edge,
  R_Vulkan_PipelineKind_GFX_Crt,
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

typedef enum R_Vulkan_Light3DKind
{
  R_Vulkan_Light3DKind_Directional,
  R_Vulkan_Light3DKind_Point,
  R_Vulkan_Light3DKind_Spot,
  R_Vulkan_Light3DKind_COUNT,
} R_Vulkan_Light3DKind;

////////////////////////////////
//~ Light Calc Types

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

////////////////////////////////
//~ C-side Shader Types (ubo,push,sbo)

//- ubo

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
// StaticAssert(sizeof(R_Vulkan_UBO_Rect) % 256 == 0, NotAligned); // constant count/offset must be aligned to 256 bytes

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

//- push

typedef struct R_Vulkan_PUSH_Geo3D_Forward R_Vulkan_PUSH_Geo3D_Forward;
struct R_Vulkan_PUSH_Geo3D_Forward
{
  Vec2F32 viewport;
  Vec2U32 light_grid_size;
};

typedef struct R_Vulkan_PUSH_Noise R_Vulkan_PUSH_Noise;
struct R_Vulkan_PUSH_Noise
{
  Vec2F32 resolution;
  Vec2F32 mouse;
  F32 time;
};

typedef struct R_Vulkan_PUSH_Edge R_Vulkan_PUSH_Edge;
struct R_Vulkan_PUSH_Edge
{
  F32 time;
};

typedef struct R_Vulkan_PUSH_Crt R_Vulkan_PUSH_Crt;
struct R_Vulkan_PUSH_Crt
{
  Vec2F32 resolution;
  F32 warp;
  F32 scan;
  F32 time;
};

// -sbo

#define R_Vulkan_SBO_Geo3D_Joint Mat4x4F32

typedef struct R_Vulkan_SBO_Geo3D_Tile R_Vulkan_SBO_Geo3D_Tile;
struct R_Vulkan_SBO_Geo3D_Tile
{
  R_Vulkan_Frustum frustum;
};

#define R_Vulkan_SBO_Geo3D_Light R_Geo3D_Light

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

#define R_Vulkan_SBO_Geo3D_Material R_Geo3D_Material

////////////////////////////////
//~ Vulkan Device Types

typedef struct R_Vulkan_Surface R_Vulkan_Surface;
struct R_Vulkan_Surface
{
  VkSurfaceKHR h;
  VkSurfaceCapabilitiesKHR caps;
  U32 format_count;
  VkSurfaceFormatKHR formats[8];
  U32 prest_mode_count;
  VkPresentModeKHR prest_modes[8];
};

typedef struct R_Vulkan_Image R_Vulkan_Image;
struct R_Vulkan_Image
{
  VkImage h;
  VkFormat format;
  VkDeviceMemory memory;
  VkImageView view;
  VkExtent2D extent;
  VkImageLayout gpu_layout;
};

typedef struct R_Vulkan_Swapchain R_Vulkan_Swapchain;
struct R_Vulkan_Swapchain
{
  VkSwapchainKHR h;
  VkExtent2D extent;
  VkFormat format;
  VkColorSpaceKHR color_space;
  U32 image_count;
  VkImage images[8];
  VkImageView image_views[8];
  // Semaphores that are waited on by QueuePresent are buffered based on the bumber of swapchain images
  VkSemaphore submit_semaphores[8];
};

typedef struct R_Vulkan_PhysicalDevice R_Vulkan_PhysicalDevice;
struct R_Vulkan_PhysicalDevice
{
  VkPhysicalDevice h;
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceMemoryProperties mem_properties;
  VkPhysicalDeviceFeatures features;
  VkExtensionProperties *extensions;
  U64 extension_count;
  U32 gfx_queue_family_index;
  U32 cmp_queue_family_index;
  U32 prest_queue_family_index;
  // U32 xfer_queue_family_index;
  VkFormat depth_image_format;
};

typedef struct R_Vulkan_LogicalDevice R_Vulkan_LogicalDevice;
struct R_Vulkan_LogicalDevice
{
  VkDevice h;
  VkQueue gfx_queue;
  VkQueue prest_queue;
  // VkQueue  xfer_queue;
};

typedef struct R_Vulkan_Pipeline R_Vulkan_Pipeline;
struct R_Vulkan_Pipeline
{
  R_Vulkan_PipelineKind kind;
  VkPipeline h;
  VkPipelineLayout layout;
};

typedef struct R_Vulkan_Buffer R_Vulkan_Buffer;
struct R_Vulkan_Buffer
{
  R_Vulkan_Buffer *next;
  U64 generation;
  VkBuffer h;
  VkDeviceMemory memory;

  R_ResourceKind kind;
  U64 size;
  U64 cap;

  VkBuffer staging;
  VkDeviceMemory staging_memory;
  // NOTE(k): refer to staging buffer if staging is present
  void *mapped;
};

typedef struct R_Vulkan_DescriptorSetLayout R_Vulkan_DescriptorSetLayout;
struct R_Vulkan_DescriptorSetLayout
{
  VkDescriptorSetLayoutBinding *bindings;
  U64 binding_count;
  VkDescriptorSetLayout h;
};

typedef struct R_Vulkan_DescriptorSetPool R_Vulkan_DescriptorSetPool;
struct R_Vulkan_DescriptorSetPool
{
  R_Vulkan_DescriptorSetPool *next;
  R_Vulkan_DescriptorSetKind kind;
  VkDescriptorPool h;
  U64 cmt;
  U64 cap;
};

typedef struct R_Vulkan_DescriptorSet R_Vulkan_DescriptorSet;
struct R_Vulkan_DescriptorSet
{
  VkDescriptorSet h;
  R_Vulkan_DescriptorSetPool *pool;
};

typedef struct R_Vulkan_UBOBuffer R_Vulkan_UBOBuffer;
struct R_Vulkan_UBOBuffer
{
  R_Vulkan_Buffer buffer;
  R_Vulkan_DescriptorSet set;
  U64 unit_count;
  U64 stride;
};

typedef struct R_Vulkan_SBOBuffer R_Vulkan_SBOBuffer;
struct R_Vulkan_SBOBuffer
{
  R_Vulkan_Buffer buffer;
  R_Vulkan_DescriptorSet set;
  U64 unit_count;
  U64 stride;
};

typedef struct R_Vulkan_Tex2D R_Vulkan_Tex2D;
struct R_Vulkan_Tex2D
{
  R_Vulkan_Tex2D *next;
  U64 generation;
  R_Vulkan_DescriptorSet desc_set;
  R_Vulkan_Image image;
  R_Tex2DFormat format;
  R_Tex2DSampleKind sample_kind;
};

typedef struct R_Vulkan_RenderTargets R_Vulkan_RenderTargets;
struct R_Vulkan_RenderTargets
{
  R_Vulkan_RenderTargets *next;
  // NOTE(k): If swapchain image format changed, we would need to recreate renderpass and pipeline
  //          If only extent of swapchain image changed, we could just recrete the swapchain
  R_Vulkan_Swapchain     swapchain;

  R_Vulkan_Image         stage_color_image;
  R_Vulkan_DescriptorSet stage_color_ds;
  R_Vulkan_Image         stage_id_image;
  R_Vulkan_Buffer        stage_id_cpu;
  // scratch
  R_Vulkan_Image         scratch_color_image;
  R_Vulkan_DescriptorSet scratch_color_ds;
  // edge
  R_Vulkan_Image         edge_image;
  R_Vulkan_DescriptorSet edge_ds;
  // 2d
  R_Vulkan_Image         geo2d_color_image;
  R_Vulkan_DescriptorSet geo2d_color_ds;

  // 3d
  // TODO(XXX): this is a mess, cleanup unused image, also add toon shading post pass
  R_Vulkan_Image         geo3d_color_image;
  R_Vulkan_DescriptorSet geo3d_color_ds;
  R_Vulkan_Image         geo3d_normal_depth_image;
  R_Vulkan_DescriptorSet geo3d_normal_depth_ds;
  R_Vulkan_Image         geo3d_depth_image;
  R_Vulkan_Image         geo3d_pre_depth_image;
  R_Vulkan_DescriptorSet geo3d_pre_depth_ds;

  U64                    rc;
  U64                    deprecated_at_frame;
};

typedef struct R_Vulkan_Frame R_Vulkan_Frame;
struct R_Vulkan_Frame
{
  VkSemaphore img_acq_sem;
  VkFence inflt_fence;
  U32 img_idx;
  VkCommandBuffer cmd_buf;

  R_Vulkan_RenderTargets *render_targets_ref;

  // UBO buffer and descriptor set
  R_Vulkan_UBOBuffer ubo_buffers[R_Vulkan_UBOTypeKind_COUNT];

  // Storage buffer and descriptor set
  R_Vulkan_SBOBuffer sbo_buffers[R_Vulkan_SBOTypeKind_COUNT];

  // Instance buffer
  R_Vulkan_Buffer inst_buffer_rect[R_MAX_RECT_PASS];
  R_Vulkan_Buffer inst_buffer_mesh2d[R_MAX_GEO2D_PASS];
  R_Vulkan_Buffer inst_buffer_mesh3d[R_MAX_GEO3D_PASS];
};

typedef struct R_Vulkan_Window R_Vulkan_Window;
struct R_Vulkan_Window
{
  // allocation link
  U64 generation;
  R_Vulkan_Window *next;

  OS_Handle os_wnd;

  R_Vulkan_Surface surface;
  R_Vulkan_RenderTargets *render_targets;
  union
  {
    struct
    {
      R_Vulkan_Pipeline rect;
      // R_Vulkan_Pipeline blur;
      R_Vulkan_Pipeline noise;
      R_Vulkan_Pipeline edge;
      R_Vulkan_Pipeline crt;
      struct
      {
        R_Vulkan_Pipeline forward[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT];
        R_Vulkan_Pipeline composite;
      } geo2d;
      struct
      {
        R_Vulkan_Pipeline tile_frustum;
        R_Vulkan_Pipeline light_culling;
        R_Vulkan_Pipeline z_pre[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT];
        R_Vulkan_Pipeline debug[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT];
        R_Vulkan_Pipeline forward[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT];
        R_Vulkan_Pipeline composite;
      } geo3d;
      R_Vulkan_Pipeline finalize;
    };
    // TODO(XXX): not sure if we need some paddings here
    R_Vulkan_Pipeline arr[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT * 4 + 9];
  } pipelines;

  R_Vulkan_Frame frames[R_VULKAN_MAX_FRAMES_IN_FLIGHT];
  U64 curr_frame_idx;

  // buffer index/offset per frame (for dynamic buffer)
  U64 ui_group_index;   // ubo is per ui group
  U64 ui_pass_index;    // inst buffer is per ui pass
  U64 geo2d_pass_index; // ubo and inst buffer is per geo2d pass
  U64 geo3d_pass_index; // ubo and inst buffer is per geo3d pass
};

////////////////////////////////
//~ Staging Ring Buffer

typedef struct R_Vulkan_StagingRing R_Vulkan_StagingRing;
struct R_Vulkan_StagingRing
{
  VkBuffer buffer;
  VkDeviceMemory memory;
  void *mapped;

  U64 cap;
  U64 head; // next write offset
  U64 tail; // oldest in-flight offset (safe to reuse before this) 
};

typedef struct R_Vulkan_StagingSlice R_Vulkan_StagingSlice;
struct R_Vulkan_StagingSlice
{
  U64 offset;
  U64 size;
  void *ptr; // mapped + offset
};

// per frame
typedef struct R_Vulkan_StagingBatch R_Vulkan_StagingBatch;
struct R_Vulkan_StagingBatch
{
  // per-upload state
  U64 size;
  U64 start;
  U64 end;
  B32 submitted;
  // per-frame artifacts (darray)
  R_Vulkan_Image **images;
};

typedef struct R_Vulkan_Stage R_Vulkan_Stage;
struct R_Vulkan_Stage
{
  R_Vulkan_StagingRing ring;
  VkCommandBuffer cmds[R_VULKAN_STAGING_IN_FLIGHT_COUNT];
  VkFence fences[R_VULKAN_STAGING_IN_FLIGHT_COUNT];
  R_Vulkan_StagingBatch batches[R_VULKAN_STAGING_IN_FLIGHT_COUNT];
  U64 idx; // in-flight rotation index
  U64 last_touch_frame_index;
};

////////////////////////////////
//~ State Types

typedef struct R_Vulkan_State R_Vulkan_State;
struct R_Vulkan_State
{
  bool                                debug;
  VkDebugUtilsMessengerEXT            debug_messenger;
  PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

  Arena                               *arena;
  Arena                               *frame_arena;
  R_Vulkan_Window                     *first_free_window;
  R_Vulkan_Tex2D                      *first_free_tex2d;
  R_Vulkan_Buffer                     *first_free_buffer;
  R_Vulkan_RenderTargets              *first_free_render_targets;
  RWMutex                             device_rw_mutex;

  R_Vulkan_RenderTargets              *first_to_free_render_targets;
  R_Vulkan_RenderTargets              *last_to_free_render_targets;

  VkInstance                          instance;
  U32                                 instance_version_major;
  U32                                 instance_version_minor;
  U32                                 instance_version_patch;

  // physical device candidates
  R_Vulkan_PhysicalDevice             *physical_devices;
  U64                                 physical_device_count; 

  U64                                 physical_device_idx;
  R_Vulkan_LogicalDevice              logical_device;

  VkSampler                           samplers[R_Tex2DSampleKind_COUNT];
  R_Vulkan_DescriptorSetLayout        set_layouts[R_Vulkan_DescriptorSetKind_COUNT];

  VkShaderModule                      vshad_modules[R_Vulkan_VShadKind_COUNT];
  VkShaderModule                      fshad_modules[R_Vulkan_FShadKind_COUNT];
  VkShaderModule                      cshad_modules[R_Vulkan_CShadKind_COUNT];

  VkCommandPool                       cmd_pool;
  // TODO(Next): deprecated, remove this ASAP
  // For copying staging buffer or transition image layout
  VkCommandBuffer                     oneshot_cmd_buf;

  R_Vulkan_Stage                      stage;

  // resource free list
  R_Vulkan_DescriptorSetPool          *first_avail_ds_pool[R_Vulkan_DescriptorSetKind_COUNT];
  // TODO(k): first_free_descriptor, we could update descriptor to point a new buffer or image/sampler
  // TODO(k): we may want to keep track of filled ds_pool

  R_Handle                            backup_texture;
  U64                                 frame_index;

  R_Vulkan_DeclStackNils;
  R_Vulkan_DeclStacks;
};

////////////////////////////////
//~ Globals

global R_Vulkan_State *r_vulkan_state = 0;

////////////////////////////////
//~ Window Functions

internal R_Handle         r_vulkan_handle_from_window(R_Vulkan_Window *window);
internal R_Vulkan_Window* r_vulkan_window_from_handle(R_Handle handle);
internal void             r_vulkan_window_resize(R_Vulkan_Window *window);

////////////////////////////////
//~ Tex2D Functions

internal R_Vulkan_Tex2D* r_vulkan_tex2d_from_handle(R_Handle handle);
internal R_Handle        r_vulkan_handle_from_tex2d(R_Vulkan_Tex2D *texture);

////////////////////////////////
//~ Buffer Functions

internal R_Vulkan_Buffer* r_vulkan_buffer_from_handle(R_Handle handle);
internal R_Handle         r_vulkan_handle_from_buffer(R_Vulkan_Buffer *buffer);

////////////////////////////////
//~ State Getter

#define r_vulkan_pdevice() (&r_vulkan_state->physical_devices[r_vulkan_state->physical_device_idx])
#define r_vulkan_ldevice() (&r_vulkan_state->logical_device)

////////////////////////////////
//~ Stage Ring Buffer Functions

internal void                  r_vulkan_stage_init();
internal R_Vulkan_StagingSlice r_vulkan_staging_slice_from_size(U64 size, U64 alignment);
internal U64                   r_vulkan_free_size_from_staging_ring(U64 alignment);
internal void                  r_vulkan_stage_begin();
internal void                  r_vulkan_stage_end();
internal void                  r_vulkan_stage_bump();
internal void                  r_vulkan_stage_copy_image(void *src, U64 size, R_Vulkan_Image *dst, Vec3S32 offset, Vec3S32 extent);

////////////////////////////////
//~ Vulkan Resource Allocation

//- swapchain
internal R_Vulkan_Swapchain      r_vulkan_swapchain(R_Vulkan_Surface *surface, OS_Handle os_wnd, VkFormat format, VkColorSpaceKHR color_space, R_Vulkan_Swapchain *old);
internal void                    r_vulkan_format_for_swapchain(VkSurfaceFormatKHR *formats, U64 count, VkFormat *format, VkColorSpaceKHR *color_space);

//- surface
internal VkFormat                r_vulkan_optimal_depth_format_from_pdevice(VkPhysicalDevice pdevice);
internal void                    r_vulkan_surface_update(R_Vulkan_Surface *surface);

//- UBO, SBO
internal R_Vulkan_UBOBuffer      r_vulkan_ubo_buffer_alloc(R_Vulkan_UBOTypeKind kind, U64 unit_count);
internal R_Vulkan_SBOBuffer      r_vulkan_sbo_buffer_alloc(R_Vulkan_SBOTypeKind kind, U64 unit_count);

//- render targets
internal R_Vulkan_RenderTargets* r_vulkan_render_targets_alloc(OS_Handle os_wnd, R_Vulkan_Surface *surface, R_Vulkan_RenderTargets *old);
internal void                    r_vulkan_render_targets_destroy(R_Vulkan_RenderTargets *render_targets);

//- descriptor
internal void                    r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind kind, U64 set_count, U64 cap, VkBuffer *buffers, VkImageView *image_views, VkSampler *sampler, R_Vulkan_DescriptorSet *sets);
internal void                    r_vulkan_descriptor_set_destroy(R_Vulkan_DescriptorSet *set);

//- sync primitives
internal VkFence                 r_vulkan_fence();
internal VkSemaphore             r_vulkan_semaphore(VkDevice device);

//- sync helpers
internal void                    r_vulkan_image_transition(VkCommandBuffer cmd_buf, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage, VkAccessFlags src_access_flag, VkPipelineStageFlags dst_stage, VkAccessFlags dst_access_flag, VkImageAspectFlags aspect_mask);

//- pipeline
internal R_Vulkan_Pipeline       r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind kind, R_GeoTopologyKind topology, R_GeoPolygonKind polygon, VkFormat swapchain_format, R_Vulkan_Pipeline *old);
internal R_Vulkan_Pipeline       r_vulkan_cmp_pipeline(R_Vulkan_PipelineKind kind);

//- sampler
internal VkSampler               r_vulkan_sampler2d(R_Tex2DSampleKind kind);

//- helpers
internal S32                     r_vulkan_memory_index_from_type_filer(U32 type_bits, VkMemoryPropertyFlags properties);

//- instance buffers

// internal ID3D11Buffer *r_vulkan_instance_buffer_from_size(U64 size);
// internal void r_usage_access_flags_from_resource_kind(R_ResourceKind kind, D3D11_USAGE *out_vulkan_usage, UINT *out_cpu_access_flags);

////////////////////////////////
//~ Helper Functions

//- debug
internal VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_userdata);
#define VK_Assert(result) \
  do { \
    if((result) != VK_SUCCESS) { \
      Temp scratch = scratch_begin(0, 0); \
      String8 err_msg = push_str8f(scratch.arena, "[VK_ERROR] [CODE: %d] in (%s)(%s:%i)", result, __FUNCTION__, __FILE__, __LINE__); \
      os_graphical_message(1, str8_lit("Fatal Error"), err_msg); \
      Trap(); \
      os_abort(1); \
      scratch_end(scratch); \
    } \
  } while (0)

//- command buffer scope
internal void r_vulkan_cmd_begin(VkCommandBuffer cmd_buf);
internal void r_vulkan_cmd_end(VkCommandBuffer cmd_buf);
#define CmdScope(c) DeferLoop((r_vulkan_cmd_begin((c))), r_vulkan_cmd_end((c)))

#endif
