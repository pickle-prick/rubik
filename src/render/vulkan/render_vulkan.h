#ifndef RENDER_VULKAN_H
#define RENDER_VULKAN_H

// Syncronization
// We choose the number 2 here because we don't want the CPU to get too far
// ahead of the GPU With 2 frames in flight, the CPU and the GPU can be working
// on their own tasks at the same time If the CPU finishes early, it will wait
// till the GPU finishes rendering before submitting more work With 3 or more
// frames in flight, the CPU could get ahead of the GPU, because the work load
// of the GPU could be too larger for it to handle, so the CPU would end up
// waiting a lot, adding frames of latency Generally extra latency isn't desired
#define MAX_FRAMES_IN_FLIGHT 2

#define VK_Assert(result) \
    do { \
        if((result) != VK_SUCCESS) { \
            fprintf(stderr, "[VK_ERROR] [CODE: %d] in (%s)(%s:%d)\n", result, __FUNCTION__, __FILE__, __LINE__); \
            __builtin_trap(); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

/////////////////////////////////////////////////////////////////////////////////////////
//~ C-side Shader Types

typedef struct R_Vulkan_Uniforms_Rect R_Vulkan_Uniforms_Rect;
struct R_Vulkan_Uniforms_Rect
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

typedef struct R_Vulkan_Uniforms_Mesh R_Vulkan_Uniforms_Mesh;
struct R_Vulkan_Uniforms_Mesh
{
    Mat4x4F32 view;
    Mat4x4F32 proj;

    Vec4F32   global_light;

    // Debug
    U32       show_grid;
    U32       _padding1_[3];
};

#define R_Vulkan_Storage_Mesh Mat4x4F32

/////////////////////////////////////////////////////////////////////////////////////////
//~ Enums

typedef enum R_Vulkan_VShadKind
{
    R_Vulkan_VShadKind_Rect,
    // R_Vulkan_VShadKind_Blur,
    R_Vulkan_VShadKind_Geo3dDebug,
    R_Vulkan_VShadKind_Geo3dForward,
    R_Vulkan_VShadKind_Geo3dComposite,
    R_Vulkan_VShadKind_Finalize,
    R_Vulkan_VShadKind_COUNT,
} R_Vulkan_VShadKind;

typedef enum R_Vulkan_FShadKind
{
    R_Vulkan_FShadKind_Rect,
    // R_Vulkan_FShadKind_Blur,
    R_Vulkan_FShadKind_Geo3dDebug,
    R_Vulkan_FShadKind_Geo3dForward,
    R_Vulkan_FShadKind_Geo3dComposite,
    R_Vulkan_FShadKind_Finalize,
    R_Vulkan_FShadKind_COUNT,
} R_Vulkan_FShadKind;

typedef enum R_Vulkan_UniformTypeKind
{
    R_Vulkan_UniformTypeKind_Rect,
    // R_Vulkan_UniformTypeKind_Blur,
    R_Vulkan_UniformTypeKind_Geo3d,
    R_Vulkan_UniformTypeKind_COUNT,
} R_Vulkan_UniformTypeKind;

typedef enum R_Vulkan_StorageTypeKind
{
    R_Vulkan_StorageTypeKind_Geo3d,
    R_Vulkan_StorageTypeKind_COUNT,
} R_Vulkan_StorageTypeKind;

typedef enum R_Vulkan_DescriptorSetKind
{
    R_Vulkan_DescriptorSetKind_UBO_Rect,
    R_Vulkan_DescriptorSetKind_UBO_Geo3d,
    R_Vulkan_DescriptorSetKind_Storage_Geo3d,
    R_Vulkan_DescriptorSetKind_Tex2D,
    R_Vulkan_DescriptorSetKind_COUNT,
} R_Vulkan_DescriptorSetKind;

typedef enum R_Vulkan_PipelineKind
{
    R_Vulkan_PipelineKind_Rect,
    // R_Vulkan_PipelineKind_Blur,
    R_Vulkan_PipelineKind_Geo3dDebug,
    R_Vulkan_PipelineKind_Geo3dForward,
    R_Vulkan_PipelineKind_Geo3dComposite,
    R_Vulkan_PipelineKind_Finalize,
    R_Vulkan_PipelineKind_COUNT,
} R_Vulkan_PipelineKind;

typedef enum R_Vulkan_RenderPassKind
{
    R_Vulkan_RenderPassKind_Rect,
    // R_Vulkan_RenderPassKind_Blur,
    R_Vulkan_RenderPassKind_Geo3d,
    R_Vulkan_RenderPassKind_Geo3dComposite,
    R_Vulkan_RenderPassKind_Finalize,
    R_Vulkan_RenderPassKind_COUNT,
} R_Vulkan_RenderPassKind;

// Vulkan
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

typedef struct R_Vulkan_RenderPass R_Vulkan_RenderPass;
struct R_Vulkan_RenderPass
{
    VkRenderPass h;

    union
    {
        R_Vulkan_Pipeline rect;
        // NOTE(k): geo3d renderpass use multiple pipelines
        struct
        {
            R_Vulkan_Pipeline debug[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT];
            R_Vulkan_Pipeline forward[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT];
        } geo3d;
        R_Vulkan_Pipeline geo3d_composite;
        R_Vulkan_Pipeline finalize;

        R_Vulkan_Pipeline v[R_GeoTopologyKind_COUNT * R_GeoPolygonKind_COUNT * 2];
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

typedef struct R_Vulkan_UniformBuffer R_Vulkan_UniformBuffer;
struct R_Vulkan_UniformBuffer
{
    R_Vulkan_Buffer        buffer;
    R_Vulkan_DescriptorSet set;
    U64                    unit_count;
    U64                    stride;
};

typedef struct R_Vulkan_StorageBuffer R_Vulkan_StorageBuffer;
struct R_Vulkan_StorageBuffer
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
    //           If only extent of swapchain image changed, we could just recrete the swapchain
    R_Vulkan_Swapchain     swapchain;

    R_Vulkan_Image         stage_color_image;
    R_Vulkan_DescriptorSet stage_color_ds;
    R_Vulkan_Image         stage_id_image;
    R_Vulkan_Buffer        stage_id_cpu;
    R_Vulkan_Image         geo3d_color_image;
    R_Vulkan_DescriptorSet geo3d_color_ds;
    R_Vulkan_Image         geo3d_normal_depth_image;
    R_Vulkan_DescriptorSet geo3d_normal_depth_ds;
    R_Vulkan_Image         geo3d_depth_image;

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
    R_Vulkan_UniformBuffer   uniform_buffers[R_Vulkan_UniformTypeKind_COUNT];

    // Storage buffer and descriptor set
    R_Vulkan_StorageBuffer   storage_buffers[R_Vulkan_StorageTypeKind_COUNT];

    // Instance buffer
    R_Vulkan_Buffer          inst_buffer_rect;
    R_Vulkan_Buffer          inst_buffer_mesh;

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
    bool                                enable_validation_layer;
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

    VkCommandPool                       cmd_pool;
    // For copying staging buffer or transition image layout
    VkCommandBuffer                     oneshot_cmd_buf;

    R_Vulkan_DescriptorSetPool          *first_avail_ds_pool[R_Vulkan_DescriptorSetKind_COUNT];
    // TODO(k): first_free_descriptor, we could update descriptor to point a new buffer or image/sampler
    // TODO(k): we may want to keep track of filled ds_pool

    R_Handle                            backup_texture;
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
internal S32                      r_vulkan_memory_index_from_type_filer(U32 type_filter, VkMemoryPropertyFlags properties);
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
internal R_Vulkan_Pipeline        r_vulkan_pipeline(R_Vulkan_PipelineKind kind, R_GeoTopologyKind topology, R_GeoPolygonKind polygon, VkRenderPass renderpass, R_Vulkan_Pipeline *old);
internal void                     r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind kind, U64 set_count, U64 cap, VkBuffer *buffers, VkImageView *image_views, VkSampler *sampler, R_Vulkan_DescriptorSet *sets);
internal void                     r_vulkan_rendpass_grp_submit(R_Vulkan_Bag *bag, R_Vulkan_RenderPassGroup *grp);
internal R_Vulkan_UniformBuffer   r_vulkan_uniform_buffer_alloc(R_Vulkan_UniformTypeKind kind, U64 unit_count);
internal R_Vulkan_StorageBuffer   r_vulkan_storage_buffer_alloc(R_Vulkan_StorageTypeKind kind, U64 unit_count);
internal VKAPI_ATTR               VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_userdata);
internal VkFence                  r_vulkan_fence(VkDevice device);
internal VkSemaphore              r_vulkan_semaphore(VkDevice device);
internal void                     r_vulkan_cmd_begin(VkCommandBuffer cmd_buf);
internal void                     r_vulkan_cmd_end(VkCommandBuffer cmd_buf);
internal void                     r_vulkan_image_transition(VkCommandBuffer cmd_buf, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage, VkAccessFlags src_access_flag, VkPipelineStageFlags dst_stage, VkAccessFlags dst_access_flag);
internal void                     r_vulkan_descriptor_pool_alloc();
internal VkSampler                r_vulkan_sampler2d(R_Tex2DSampleKind kind);

#define CmdScope(c)    DeferLoop((r_vulkan_cmd_begin((c))), r_vulkan_cmd_end((c)))
#define FileReadAll(arena, fp, return_data, return_size)                             \
    do {                                                                             \
            OS_Handle f = os_file_open(OS_AccessFlag_Read, (fp));                    \
            FileProperties f_props = os_properties_from_file(f);                     \
            *return_size = f_props.size;                                             \
            *return_data = push_array(arena, U8, f_props.size);                      \
            os_file_read(f, rng_1u64(0,f_props.size), *return_data);                 \
    } while (0);

#endif
