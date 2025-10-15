#define R_Vulkan_StackTopImpl(state, name_upper, name_lower) \
return state->name_lower##_stack.top->v;

#define R_Vulkan_StackBottomImpl(state, name_upper, name_lower) \
return state->name_lower##_stack.bottom_val;

#define R_Vulkan_StackPushImpl(state, name_upper, name_lower, type, new_value) \
R_Vulkan_##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {SLLStackPop(state->name_lower##_stack.free);}\
else {node = push_array(r_vulkan_state->arena, R_Vulkan_##name_upper##Node, 1);}\
type old_value = state->name_lower##_stack.top->v;\
node->v = new_value;\
SLLStackPush(state->name_lower##_stack.top, node);\
if(node->next == &state->name_lower##_nil_stack_top)\
{\
state->name_lower##_stack.bottom_val = (new_value);\
}\
state->name_lower##_stack.auto_pop = 0;\
return old_value;

#define R_Vulkan_StackPopImpl(state, name_upper, name_lower) \
R_Vulkan_##name_upper##Node *popped = state->name_lower##_stack.top;\
if(popped != &state->name_lower##_nil_stack_top)\
{\
SLLStackPop(state->name_lower##_stack.top);\
SLLStackPush(state->name_lower##_stack.free, popped);\
state->name_lower##_stack.auto_pop = 0;\
}\
return popped->v;\

#define R_Vulkan_StackSetNextImpl(state, name_upper, name_lower, type, new_value) \
R_Vulkan_##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {SLLStackPop(state->name_lower##_stack.free);}\
else {node = push_array(r_vulkan_state->arena, R_Vulkan_##name_upper##Node, 1);}\
type old_value = state->name_lower##_stack.top->v;\
node->v = new_value;\
SLLStackPush(state->name_lower##_stack.top, node);\
state->name_lower##_stack.auto_pop = 1;\
return old_value;

////////////////////////////////
// Generated includes

#include "generated/render_vulkan.meta.c"

////////////////////////////////
// Shader includes

#include "shader/crt_frag.spv.h"
#include "shader/crt_vert.spv.h"
#include "shader/edge_frag.spv.h"
#include "shader/edge_vert.spv.h"
#include "shader/finalize_frag.spv.h"
#include "shader/finalize_vert.spv.h"
#include "shader/geo2d_composite_frag.spv.h"
#include "shader/geo2d_composite_vert.spv.h"
#include "shader/geo2d_forward_frag.spv.h"
#include "shader/geo2d_forward_vert.spv.h"
#include "shader/geo3d_composite_frag.spv.h"
#include "shader/geo3d_composite_vert.spv.h"
#include "shader/geo3d_debug_frag.spv.h"
#include "shader/geo3d_debug_vert.spv.h"
#include "shader/geo3d_forward_frag.spv.h"
#include "shader/geo3d_forward_vert.spv.h"
#include "shader/geo3d_light_culling_comp.spv.h"
#include "shader/geo3d_tile_frustum_comp.spv.h"
#include "shader/geo3d_zpre_frag.spv.h"
#include "shader/geo3d_zpre_vert.spv.h"
#include "shader/noise_frag.spv.h"
#include "shader/noise_vert.spv.h"
#include "shader/rect_frag.spv.h"
#include "shader/rect_vert.spv.h"

////////////////////////////////
//~ Window Functions

internal R_Handle
r_vulkan_handle_from_window(R_Vulkan_Window *window)
{
  R_Handle handle = {0};
  handle.u64[0] = (U64)window;
  handle.u64[1] = window->generation;
  return handle;
}

internal R_Vulkan_Window *
r_vulkan_window_from_handle(R_Handle handle)
{
  R_Vulkan_Window *window = (R_Vulkan_Window *)handle.u64[0];
  if(window && window->generation != handle.u64[1]) window = 0;
  return window;
}

internal void
r_vulkan_window_resize(R_Vulkan_Window *window)
{
  ProfBeginFunction();
  // Unpack some variables
  R_Vulkan_PhysicalDevice *pdevice = r_vulkan_pdevice();
  R_Vulkan_Surface *surface = &window->surface;
  R_Vulkan_LogicalDevice *ldevice = &r_vulkan_state->logical_device;
  R_Vulkan_RenderTargets *render_targets = window->render_targets;

  // NOTE(k): if the rate of resizing is too high, we could be lagged behind bag destruction, since we're not using generation/idx to track first_to_free_bag
  // frames's bag could be out of synced constantly, and the count of to_free_bag will keep increasing, and resulting out of gpu memory 
  // so we add some debounce here to avoid recreating swapchain unecessarly
  Vec2F32 window_dim = dim_2f32(os_client_rect_from_window(window->os_wnd,0));
  U64 last_window_dim_us = os_now_microseconds();

  // Handle minimization
  // There is another case where a swapchain may become out of date and that is a special kind of window resizing: window minimization
  // This case is special because it will result in a frame buffer size of 0
  // We will handle that by pausing until the window is in the foreground again int w = 0, h = 0;
  while(window_dim.x == 0.f || window_dim.y == 0.f || (os_now_microseconds()-last_window_dim_us) < 50000)
  {
    Temp scratch = scratch_begin(0,0);
    Vec2F32 dim = dim_2f32(os_client_rect_from_window(window->os_wnd,0));
    if(dim.x != window_dim.x || dim.y != window_dim.y)
    {
      window_dim = dim;
      last_window_dim_us = os_now_microseconds();
    }

    // NOTE(k): keep processing events, or else we would block the window
    os_get_events(scratch.arena, 0);
    scratch_end(scratch);
  }

  // NOTE(k): The disadvantage of this approach is that we need to stop all rendering before create the new swap chain
  // It is possible to create a new swapchain while drawing commands on an image from the old swap chain are still in-flight
  // You would need to pass the previous swapchain to the oldSwapChain field in the VkSwapchainCreateInfoKHR struct and destroy 
  // the old swap chain as soon as you're finished using it
  // vkDeviceWaitIdle(device->h);

  // In theory it can be possible for the swapchain image format to change during an applications's lifetime, e.g. when moving a window from an standard range to an high dynamic range monitor
  // This may require the application to recreate the renderpass to make sure the change between dynamic ranges is properly reflected

  VkFormat old_swapchain_format = render_targets->swapchain.format;
  r_vulkan_surface_update(surface);

  // we need to recreate the render targets if either size or format of swapchain is chagned
  SLLStackPush(r_vulkan_state->first_to_free_render_targets, render_targets);
  render_targets->deprecated_at_frame = r_vulkan_state->frame_index;
  window->render_targets = r_vulkan_render_targets_alloc(window->os_wnd, &window->surface, window->render_targets);

  // TODO(XXX): NotImplemented 
  // NOTE(k): if format is changed, we would also need to recreate the render pass
  // bool swapchain_format_changed = window->render_targets->swapchain.format != old_swapchain_format;
  // if(swapchain_format_changed)
  // {
  //   SLLStackPush(window->first_to_free_rendpass_grp, window->rendpass_grp);
  //   window->rendpass_grp = r_vulkan_rendpass_grp(window, window->bag->swapchain.format, window->rendpass_grp);
  // }
  // // create framebuffers for this bag
  // r_vulkan_rendpass_grp_submit(window->bag, window->rendpass_grp);

  ProfEnd();
}

////////////////////////////////
//~ Tex2D Functions

internal R_Vulkan_Tex2D *
r_vulkan_tex2d_from_handle(R_Handle handle)
{
  R_Vulkan_Tex2D *texture = (R_Vulkan_Tex2D *)handle.u64[0];
  if(texture && texture->generation != handle.u64[1]) texture = 0;
  return texture;
}

internal R_Handle
r_vulkan_handle_from_tex2d(R_Vulkan_Tex2D *texture)
{
  R_Handle handle = {0};
  handle.u64[0] = (U64)texture;
  handle.u64[1] = texture->generation;
  return handle;
}

////////////////////////////////
//~ Buffer Functions

internal R_Vulkan_Buffer *
r_vulkan_buffer_from_handle(R_Handle handle)
{
  R_Vulkan_Buffer *buffer = (R_Vulkan_Buffer *)handle.u64[0];
  if(buffer && buffer->generation != handle.u64[1]) buffer = 0;
  return buffer;
}

internal R_Handle
r_vulkan_handle_from_buffer(R_Vulkan_Buffer *buffer)
{
  R_Handle handle = {0};
  handle.u64[0] = (U64)buffer;
  handle.u64[1] = buffer->generation;
  return handle;
}

////////////////////////////////
//~ Stage Ring Buffer Functions

internal void
r_vulkan_stage_init()
{
  R_Vulkan_Stage *stage = &r_vulkan_state->stage;
  R_Vulkan_PhysicalDevice *pdevice = r_vulkan_pdevice();
  R_Vulkan_LogicalDevice *ldevice = r_vulkan_ldevice();
  VkDevice device = ldevice->h;
  // NOTE(k): can't set it to 0 on startup, it will causing conflict on first frame
  stage->last_touch_frame_index = max_U64;

  // command pool & buffer
  VkCommandPool cp;
  VkCommandPoolCreateInfo cpci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
  cpci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cpci.queueFamilyIndex = pdevice->gfx_queue_family_index;
  VK_Assert(vkCreateCommandPool(device, &cpci, NULL, &cp));

  // commands
  for(U64 i = 0; i < R_VULKAN_STAGING_IN_FLIGHT_COUNT; i++)
  {
    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo cbai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cbai.commandPool = cp;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VK_Assert(vkAllocateCommandBuffers(device, &cbai, &cmd));
    stage->cmds[i] = cmd;
  }

  // fences
  for(U64 i = 0; i < R_VULKAN_STAGING_IN_FLIGHT_COUNT; i++)
  {
    stage->fences[i] = r_vulkan_fence();
  }

  // init staging ring buffer
  {
    // TODO(Next): this one will use local mem, and it's not safe to use for large batch 
    U64 size = MB(512);
    R_Vulkan_StagingRing *ring = &stage->ring;

    VkBuffer buffer;
    VkBufferCreateInfo bci = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bci.size = size;
    bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_Assert(vkCreateBuffer(device, &bci, NULL, &buffer));

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

    VkMemoryAllocateInfo mai = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    mai.allocationSize = mem_requirements.size;
    mai.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

    VkDeviceMemory memory;
    VK_Assert(vkAllocateMemory(device, &mai, NULL, &memory));
    VK_Assert(vkBindBufferMemory(device, buffer, memory, 0));

    void *mapped;
    VK_Assert(vkMapMemory(device, memory, 0, mem_requirements.size, 0, &mapped));

    ring->buffer = buffer; 
    ring->memory = memory; 
    ring->mapped = mapped;
    ring->cap = size;
    ring->head = 0;
    ring->tail = 0;
  }
}

internal R_Vulkan_StagingSlice
r_vulkan_staging_slice_from_size(U64 size, U64 alignment)
{
  R_Vulkan_StagingSlice ret = {0};

  R_Vulkan_StagingRing *ring = &r_vulkan_state->stage.ring;
  U64 aligned_head = AlignPow2(ring->head, alignment);
  if(ring->tail <= aligned_head)
  {
    // two free segments: [algined_head, cap] and [0, tail]
    if((ring->cap-aligned_head) >= size)
    {
      ret.offset = aligned_head;
      ret.size = size;
      ret.ptr = (U8*)(ring->mapped) + aligned_head;

      // update head
      ring->head = aligned_head+size;
      if(ring->head == ring->cap) ring->head = 0; // wrap if exact end
    }
    else if(ring->tail >= size)
    {
      // wrap
      ret.offset = 0;
      ret.size = size;
      ret.ptr = ring->mapped;
      ring->head = size;
    }
  }
  else
  {
    // single free segments: [aligned_head, tail]
    if(size <= (ring->tail-aligned_head))
    {
      ret.offset = aligned_head;
      ret.size = size;
      ret.ptr = (U8*)(ring->mapped)+aligned_head;
      ring->head = aligned_head+size;
    }
  }
  return ret;
}

internal U64
r_vulkan_free_size_from_staging_ring(U64 alignment)
{
  U64 size = 0;

  R_Vulkan_StagingRing *ring = &r_vulkan_state->stage.ring;
  U64 aligned_head = AlignPow2(ring->head, alignment);

  // two free segments: [algined_head, cap] and [0, tail]
  if(ring->tail <= aligned_head)
  {
    size = Max(ring->cap-aligned_head, ring->tail);
  }
  // single free segments: [aligned_head, tail]
  else
  {
    size = ring->tail - aligned_head;
  }
  return size;
}

internal void
r_vulkan_stage_begin()
{
  R_Vulkan_Stage *stage = &r_vulkan_state->stage;
  R_Vulkan_StagingBatch *batch = &stage->batches[stage->idx];
  VkCommandBuffer cmd = stage->cmds[stage->idx];

  // wait for current batch to be ready
  do
  {
    r_vulkan_stage_bump();
  } while(batch->size != 0);

  // begin recording
  VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  begin_info.pInheritanceInfo = 0;
  // if the command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicity reset it
  VK_Assert(vkBeginCommandBuffer(cmd, &begin_info));

  // bump stage frame index
  stage->last_touch_frame_index = r_vulkan_state->frame_index;
}

internal void
r_vulkan_stage_end()
{
  R_Vulkan_Stage *stage = &r_vulkan_state->stage;

  // unpack stage batch
  R_Vulkan_StagingBatch *batch = &stage->batches[stage->idx];
  VkCommandBuffer cmd = stage->cmds[stage->idx];
  VkFence fence = stage->fences[stage->idx];

  // check all images, transfer them into shader read stage
  for(U64 i = 0; i < darray_size(batch->images); i++)
  {
    R_Vulkan_Image *image = batch->images[i];
    Assert(image->gpu_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    // transfer image layout to shader read
    r_vulkan_image_transition(cmd, image->h,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
                              VK_IMAGE_ASPECT_COLOR_BIT);
    image->gpu_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  // end command buffer & submit
  {
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    VK_Assert(vkEndCommandBuffer(cmd));
    VK_Assert(vkResetFences(r_vulkan_ldevice()->h, 1, &fence));
    VK_Assert(vkQueueSubmit(r_vulkan_state->logical_device.gfx_queue, 1, &si, fence));
  }

  Assert(batch->size > 0);
  // flag batch
  batch->submitted = 1;
  // rotate stage idx
  stage->idx = stage->idx%R_VULKAN_STAGING_IN_FLIGHT_COUNT;
}

internal void
r_vulkan_stage_bump()
{
  // unpack stage buffer
  R_Vulkan_Stage *stage = &r_vulkan_state->stage;
  VkCommandBuffer cmd = stage->cmds[stage->idx];
  R_Vulkan_StagingBatch *batch = &stage->batches[stage->idx];

  // bump tail
  for(U64 stage_idx = stage->idx, i = 0; i < R_VULKAN_STAGING_IN_FLIGHT_COUNT; stage_idx = (stage_idx+1)%R_VULKAN_STAGING_IN_FLIGHT_COUNT, i++)
  {
    R_Vulkan_StagingBatch *batch = &stage->batches[stage_idx];

    // if-flight/dirty? -> check fence
    if(batch->submitted)
    {
      VkFence fence = stage->fences[stage_idx];
      VkResult r = vkGetFenceStatus(r_vulkan_ldevice()->h, fence);

      // stage batch done? -> move tail & reset status
      if(r == VK_SUCCESS)
      {
        stage->ring.tail = batch->end;

        // reset batch state
        MemoryZeroStruct(batch);
      }
      else
      {
        break;
      }
    }
  }
}

internal void
r_vulkan_stage_copy_image(void *src, U64 size, R_Vulkan_Image *dst, Vec3S32 offset, Vec3S32 extent)
{
  B32 touched_this_frame = r_vulkan_state->stage.last_touch_frame_index == r_vulkan_state->frame_index;
  if(!touched_this_frame)
  {
    r_vulkan_stage_begin();
  }

  // unpack batch
  R_Vulkan_Stage *stage = &r_vulkan_state->stage;
  VkBuffer staging_buffer = r_vulkan_state->stage.ring.buffer;
  R_Vulkan_StagingBatch *batch = &stage->batches[stage->idx];
  VkCommandBuffer cmd = stage->cmds[stage->idx];

  // alloc a ring slice
  R_Vulkan_StagingSlice slice = r_vulkan_staging_slice_from_size(size, 4);
  // FIXME: keep bumping if more space is needed, we may need to submit early for works this frame
  // also assert the size if smaller than the total size of ring
  AssertAlways(slice.size == size);

  // copy data to stage buffer
  MemoryCopy(slice.ptr, src, size);

  // fill batch
  B32 batch_touched_this_frame = batch->size != 0;
  if(batch_touched_this_frame) batch->start = slice.offset;
  batch->end = slice.offset+slice.size;
  batch->size += slice.size;

  B32 image_first_copy_this_frame = dst->gpu_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  // issue copy command
  { 
    if(image_first_copy_this_frame)
    {
      // There is actually a special type of image layout that supports all operations, VK_IMAGE_LAYOUT_GENERAL
      // The problem with it, of course, is that it doesn't necessarily offer the best performance for any operation
      // It is required for some special cases, like using an image as both input and output, or for reading an image after it has left the preinitialized layout
      r_vulkan_image_transition(cmd, dst->h,
                                dst->gpu_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_IMAGE_ASPECT_COLOR_BIT);
      dst->gpu_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }

    VkBufferImageCopy region =
    {
      .bufferOffset = slice.offset,
      // These two fields specify how the pixels are laid out in memory
      // For example, you could have some padding bytes between rows of the image
      // Specifying 0 for both indicates that the pixels are simply tightly packed like they are in our case
      // The imageSubresource, imageOffset and imageExtent fields indicate to which part of the image we want to copy the pixels
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .imageSubresource.mipLevel = 0,
      .imageSubresource.baseArrayLayer = 0,
      .imageSubresource.layerCount = 1,
      .imageOffset = {offset.x, offset.y, offset.z}, // These two fields indicate to which part of the image we want to copy the pixels
      .imageExtent = {extent.x, extent.y, extent.z},
    };

    // it's possible to specify an array of VkBufferImageCopy to perform many different copies from this buffer to the image in on operation 
    vkCmdCopyBufferToImage(cmd, staging_buffer, dst->h, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  }

  // push image list
  if(image_first_copy_this_frame) darray_push(r_vulkan_state->frame_arena, batch->images, dst);
}

////////////////////////////////
//~ Vulkan Resource Allocation
//

//- swapchain

internal R_Vulkan_Swapchain
r_vulkan_swapchain(R_Vulkan_Surface *surface, OS_Handle os_wnd, VkFormat format, VkColorSpaceKHR color_space, R_Vulkan_Swapchain *old_swapchain)
{
  R_Vulkan_Swapchain swapchain = {0};
  swapchain.format      = format;
  swapchain.color_space = color_space;

  // The presentation mode is arguably the most important setting for the swap chain, because it represents the actual conditions for showing images to 
  // the screen. There are four possible modes available in Vulkan
  // 1. VK_PRESENT_MODE_IMMEDIATE_KHR: images submitted by your application are transferred to the screen right away, which may result in tearing
  // 2. VK_PRESENT_MODE_FIFO_KHR: the swap chain is a queue where the display takes and image from the front of the queue when the display is refreshed and the program
  //    inserts rendered images at the back of the queue. If the queue is full then the program has to wait. This is most similar to vertical sync as found in modern
  //    games. The moment that the display is refreshed is known as "vertical blank"
  // 3. VK_PRESENT_MODE_FIFO_RELAXED_KHR: this mode only differs from the previous one if the application is late and the queue was empty at the last vertical blank.
  //    Instead of waiting for the next vertical blank, the image is transferred right away when if finally arrvies. This may result in visible tearing
  // 4. VK_PRESENT_MODE_MAILBOX_KHR: this is another variation of the second mode. Instead of blocking the application when the queue is full, the images that are already
  //    queued are simply replaced with the newer ones. This mode can be used to render frames as fast as possbile while still avoiding tearing, resulting in fewer latency
  //    issue than the standard vertical sync. This is commonly known as "triple buffering", although the existence of three buffers alone does not necessarily mean that
  //    the framerate is unlocked
  // Only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available, so we will default to that
  // VkPresentModeKHR preferred_prest_mode = VK_PRESENT_MODE_MAILBOX_KHR;
  VkPresentModeKHR preferred_prest_mode = VK_PRESENT_MODE_FIFO_KHR;
  // TODO: is this working?
  VkPresentModeKHR selected_prest_mode = VK_PRESENT_MODE_FIFO_KHR;
  for(U64 i = 0; i < surface->prest_mode_count; i++)
  {
    // VK_PRESENT_MODE_MAILBOX_KHR is a very nice trade-off if energy usage is not a concern.
    // It allows us to avoid tearing while still maintaining a fairly low latency by rendering new images that are as up-to-date as possible right until the vertical 
    // blank. On mobile devices, where energy usage is more important, you will probably want to use VK_PRESENT_MODE_FIFO_KHR instead
    if(surface->prest_modes[i] == preferred_prest_mode)
    {
      selected_prest_mode = preferred_prest_mode;
      break;
    }
  }

  ////////////////////////////////
  //~ Swap extent

  // The swap extent is the resolution of the swap chain images and it's almost always exactly equal to the resolution of the window that we're drawing to in *pixels*.
  // The range of the possbile resolutions is defined in the VkSurfaceCapabilitiesKHR structure
  // Vulkan tells us to match the resolution of the window by setting the width and height in the currentExtent member. However, some window managers do allow use to
  // differ here and this is indicated by setting the width and height in currentExtent to special value: the maximum value of uint32_t
  // In that case we'll pick the resolution that best matches the window within the minImageExtent and maxImageExtent bounds
  // But we must specify the resolution in the correct unit
  // GLFW uses two units when measuring sizes: pixels and screen coordinates. For example, the resolution {WIDTH, HEIGHT} that we specified earlier when creating the 
  // window is measured in screen coordinates. But Vulkan works with pixels, so the swapchain extent must be specified in pixels as well. Unfortunately, if you are using 
  // a high DPI display (like Apple's Retina display), screen coordinates don't correspond to pixels. Instead, due to the higher pixel density, the resolution of the window in pixel
  // will be larger than the resolution in screen coordinates. So if Vulkan doesn't fix the swap extent for use, we can't just use the original {WIDTH, HEIGHT} 
  // Instead, we must use glfwGetFramebufferSize to query the resolution of the window in pixels before matching it against the minimum and maximum image extent
  // VkExtent2D selected_surface_extent;

  if(surface->caps.currentExtent.width != 0xFFFFFFFF)
  {
    swapchain.extent = surface->caps.currentExtent;
  }
  else
  {
    Rng2F32 client_rect = os_client_rect_from_window(os_wnd,0);
    Vec2F32 dim = dim_2f32(client_rect);
    U32 width = dim.x;
    U32 height = dim.y;

    width  = Clamp(surface->caps.minImageExtent.width, width, surface->caps.maxImageExtent.width);
    height = Clamp(surface->caps.minImageExtent.height, height, surface->caps.maxImageExtent.height);
    swapchain.extent.width  = width;
    swapchain.extent.height = height;
  }

  // How many images we would like to have in the swap chain. The implementation specifies the minimum number that it requires to function
  // However, simply sticking to this minimum means that we may sometimes have to wait on the driver to complete internal operations before we can
  // acquire another image to render to. Therefore it is recommended to request at least one more image than the minimum
  U32 min_swapchain_image_count = surface->caps.minImageCount + 1;
  // We should also make sure to not exceed the maximum number of images while doing this, where 0 is special value that means that there is no maximum
  if(surface->caps.maxImageCount > 0 && min_swapchain_image_count > surface->caps.maxImageCount) min_swapchain_image_count = surface->caps.maxImageCount;

  ////////////////////////////////
  //~ Create swapchain

  // *imageArrayLayers*
  // The imageArrayLayers specifies the amount of layers each image consists of
  // This is always 1 unless you are developing a stereoscopic 3D application
  // TODO(k): kind of confused here, come back later
  // *imageUsage*
  // The imageUsage bit field specifies what kind of operations we'll use the images in the swap chain for
  // Here we are going to render directly to them, which means that they're used as color attachment.
  // It is also possbile that you'll render images to seprate iamge first to perform operations like post-processing
  // In that case you may use a value like VK_IMAGE_USAGE_DST_BIT instead and use a memory operation to transfer the rendered image to swap chain image
  // *preTransform*
  // We can specify that a certain transform should be applied to images in the swap chain if it is supported (supportedTransforms in capablities)
  // like a 90 degree clockwise rotation or horizontal flip
  // To specify you do not want any transformation, simply specify the current transformation
  // *compositeAlpha*
  // The compositeAlpha field specifies if the alpha channel should be used for blending with other windows in the window system
  // You'll almost always want to simply ignore the alpha channel
  // *clipped*
  // If the clipped member is set to VK_TRUE then that means that we don't care about the color of pixels that are obscured, for example because
  // another window is in front of them. Unless you really need to be able to read these pixels back and get predictable results, you'll get the best
  // performance by enabling clipping
  // *oldSwapchain*
  // With Vulkan it's possbile that your swapchain becomes invalid or unoptimized while your application is running, for example because the window
  // was resized. In that case the swapchain actually needs to be recreated from scratch and a reference to the old one must be specified in this field.
  VkSwapchainCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  create_info.surface          = surface->h;
  create_info.minImageCount    = min_swapchain_image_count;
  create_info.imageFormat      = swapchain.format;
  create_info.imageColorSpace  = swapchain.color_space;
  create_info.imageExtent      = swapchain.extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  // NOTE(k): if we want to manully clear the swap image, the VK_IMAGE_USAGE_TRANSFER_DST_BIT is necessary
  create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  create_info.preTransform     = surface->caps.currentTransform;
  create_info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode      = selected_prest_mode;
  create_info.clipped          = VK_TRUE;
  create_info.oldSwapchain     = VK_NULL_HANDLE;
  if(old_swapchain != 0) create_info.oldSwapchain = old_swapchain->h;

  // We need to specify how to handle swap chain images that will be used across multiple queue families
  // That will be the case in our application if the grpahics queue family is different from the presentation queue
  // We'll be drawing on the images in the swap chain from the graphics queue and then submitting them on the presentation queue
  // There are two ways to handle images that are accessed from multiple type of queues
  // 1. VK_SHARING_MODE_EXCLUSIVE: an image is owned by one queue family at a time and ownership must be explicitly transfered before using
  //    it in another queue family. This option offers the best performance
  // 2. VK_SHARING_MODE_CONCURRENT: images can be used across multiple queue families without explicit ownership transfers
  U32 queue_family_indices[2] = { r_vulkan_pdevice()->gfx_queue_family_index, r_vulkan_pdevice()->prest_queue_family_index };
  if(r_vulkan_pdevice()->gfx_queue_family_index != r_vulkan_pdevice()->prest_queue_family_index)
  {
    create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices   = queue_family_indices;
  }
  else
  {
    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;    // Optional
    create_info.pQueueFamilyIndices   = NULL; // Optional
  }

  VK_Assert(vkCreateSwapchainKHR(r_vulkan_state->logical_device.h, &create_info, NULL, &swapchain.h));

  ////////////////////////////////
  //~ Retrieving the swap chain images

  // The swapchain has been created now, so all the remains is retrieving the handles of the [VkImage]s in it
  // We will reference these during rendering operations
  {
    U32 image_count = 0;
    VK_Assert(vkGetSwapchainImagesKHR(r_vulkan_state->logical_device.h, swapchain.h, &image_count, NULL));
    image_count = ClampBot(image_count, ArrayCount(swapchain.images));
    swapchain.image_count = image_count;
    VK_Assert(vkGetSwapchainImagesKHR(r_vulkan_state->logical_device.h, swapchain.h, &swapchain.image_count, swapchain.images));
  }

  ////////////////////////////////
  //~ Create image views

  // To use any VkImage, including those in the swap chain, in the render pipeline we have to a VkImageView object
  // An image view is quite literally a view into an image.
  // It describes how to access the image and which part of the image to access
  //      for example if it should be treated as 2D texture depth texture without any mipmapping levels
  // Here, we are create a basic image view for every image in the swapchain so that we can use them as color targets later on
  // VkImageView swapchain_image_views[swapchain_image_count];
  for(U64 i = 0; i < swapchain.image_count; i++)
  {
    VkImageViewCreateInfo create_info =
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = swapchain.images[i],
      // The viewType and format fields specify how you the image data should be interpreted
      // The viewType parameter allows you to treat images as 1D textures, 2D textures, 3D textures and cube maps
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format   = swapchain.format,
      // The components field allows you to swizzle the color channels around
      // For example, you can map all of the channesl to the red channel for a monochrome texture.
      // You can also map constant values of 0 and 1 to a channel
      .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
      // The subresourceRange field describes what the image's purpose is and which part of the image should be accessed
      // Our images in the swapchain will be used as color targets without any mipmapping levels or multiple layers
      .subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount   = 1,
      // If you were working on a stereographic 3D application, then you would create a swap chain with multiple layers
      // You could then create multiple image views for each image representing the views for the left and right eyes by accessing different layers
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount     = 1,
    };

    VK_Assert(vkCreateImageView(r_vulkan_state->logical_device.h, &create_info, NULL, &swapchain.image_views[i]));
    // Unlike images, the image views were explicitly created by us, so we need to add a similar loop to destroy them again at the end of the program

    // create submit semaphores
    for(U64 i = 0; i < swapchain.image_count; i++)
    {
      swapchain.submit_semaphores[i] = r_vulkan_semaphore(r_vulkan_state->logical_device.h);
    }
  }
  return swapchain;
}

internal void
r_vulkan_format_for_swapchain(VkSurfaceFormatKHR *formats, U64 count, VkFormat *format, VkColorSpaceKHR *color_space)
{
  // There are tree types of settings to consider
  // 1. Surface format(color depth)
  // 2. Presentation mode (conditions for "swapping" images to the screen)
  // 3. Swap extent (resolution of images in swap chain)
  // For each of these settings we'll have an ideal value in mind that we'll go with
  //      if it's available and otherwise we'll find the next best thing
  //-------------------------------------------------------------------------------
  // Each VkSurfaceFormatKHR entry contains a format and a colorSpace member. The format member specifies the color channels and types
  // For example, VK_FORMAT_B8G8R8A8_SRGB means that we store the B,G,G and alpha channels in that order with an 8 bit unsigned integer for a total 32 bits
  // per pixel. The colorSpcae member indicates if the SRGB color space is supported or no using the VK_COLOR_SPACE_NONLINEAR_KHR flag
  // Note that this flag used to be called VK_COLORSPACE_SRGB_NONLINEAR_KHR in old versions of the specification
  Assert(count > 0);
  *format = formats[0].format;
  *color_space = formats[0].colorSpace;
  for(U64 i = 0; i < count; i++)
  {
    // For the color space, we'll use SRGB if it's available, because it results in more accurate perceived colors.
    // It is also pretty much the standard color space for images, like the textures we'll use later on. Because of that we should also 
    // use an SRGB color format, of which one of the most common ones is VK_FORMAT_B8G8R8A8_SRGB
    if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      *format = formats[i].format;
      *color_space = formats[i].colorSpace;
      break;
    }
  }
}

internal VkFormat
r_vulkan_optimal_depth_format_from_pdevice(VkPhysicalDevice pdevice)
{
  // Unlike the texture image, we don't necessarily need a specific format
  //      because we won't be directly accessing the texels from the program 
  // It just needs to have a reasonable accuracy, at least 24 bits is common in real-world applciation
  // There are several formats that fit this requirement
  // *         VK_FORMAT_D32_SFLOAT: 32-bit float for depth
  // * VK_FORMAT_D32_SFLOAT_S8_UINT: 32-bit signed float for depth and 8 bit stencil component
  // *  VK_FORMAT_D24_UNORM_S8_UINT: 24-bit (unsigned?) float for depth and 8 bit stencil component
  // The stencil component is used for stencil tests, which is an addtional test that can be combined with depth testing

  VkFormat format_candidates[3] = {
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT,
  };

  int the_one = -1;
  for(U64 i = 0; i < 3; i++)
  {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(pdevice, format_candidates[i], &props);

    // The VkFormatProperties struct contains three fields
    // * linearTilingFeatures: Use cases that are supported with linear tiling
    // * optimalTillingFeatures: Use cases that are supported with optimal tilling
    // * bufferFeatures: Use cases that are supported for buffers
    if((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) continue;
    the_one = i;
    break;
  }
  AssertAlways(the_one != -1 && "No suitable format for depth image");
  return format_candidates[the_one];
}

//- surface

internal void
r_vulkan_surface_update(R_Vulkan_Surface *surface)
{
  VK_Assert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r_vulkan_pdevice()->h, surface->h, &surface->caps));
  VK_Assert(vkGetPhysicalDeviceSurfaceFormatsKHR(r_vulkan_pdevice()->h, surface->h, &surface->format_count, NULL));
  VK_Assert(vkGetPhysicalDeviceSurfaceFormatsKHR(r_vulkan_pdevice()->h, surface->h, &surface->format_count, surface->formats));
  VK_Assert(vkGetPhysicalDeviceSurfacePresentModesKHR(r_vulkan_pdevice()->h, surface->h, &surface->prest_mode_count, NULL));
  VK_Assert(vkGetPhysicalDeviceSurfacePresentModesKHR(r_vulkan_pdevice()->h, surface->h, &surface->prest_mode_count, surface->prest_modes));
}


//- UBO, SBO

internal R_Vulkan_UBOBuffer
r_vulkan_ubo_buffer_alloc(R_Vulkan_UBOTypeKind kind, U64 unit_count)
{
  R_Vulkan_UBOBuffer ubo_buffer = {0};

  U64 stride = 0;
  switch(kind)
  {
    case R_Vulkan_UBOTypeKind_Rect:
    {
      stride = AlignPow2(sizeof(R_Vulkan_UBO_Rect), r_vulkan_pdevice()->properties.limits.minUniformBufferOffsetAlignment);
    }break;
    case R_Vulkan_UBOTypeKind_Geo2D:
    {
      stride = AlignPow2(sizeof(R_Vulkan_UBO_Geo2D), r_vulkan_pdevice()->properties.limits.minUniformBufferOffsetAlignment);
    }break;
    case R_Vulkan_UBOTypeKind_Geo3D:
    {
      stride = AlignPow2(sizeof(R_Vulkan_UBO_Geo3D), r_vulkan_pdevice()->properties.limits.minUniformBufferOffsetAlignment);
    }break;
    case R_Vulkan_UBOTypeKind_Geo3D_TileFrustum:
    {
      stride = AlignPow2(sizeof(R_Vulkan_UBO_Geo3D_TileFrustum), r_vulkan_pdevice()->properties.limits.minUniformBufferOffsetAlignment);
    }break;
    case R_Vulkan_UBOTypeKind_Geo3D_LightCulling:
    {
      stride = AlignPow2(sizeof(R_Vulkan_UBO_Geo3D_LightCulling), r_vulkan_pdevice()->properties.limits.minUniformBufferOffsetAlignment);
    }break;
    default:{InvalidPath;}break;
  }

  U64 buf_size = stride * unit_count;

  ubo_buffer.unit_count  = unit_count;
  ubo_buffer.stride      = stride;
  ubo_buffer.buffer.size = buf_size;
  ubo_buffer.buffer.cap  = buf_size;

  VkBufferCreateInfo buf_ci = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
  // Specifies the size of the buffer in bytes
  buf_ci.size = buf_size;
  // The usage field indicats for which purpose the data in the buffer is going to be used
  // It is possible to specify multiple purposes using a bitwise or
  // Our use case will be a vertex buffer
  // .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
  buf_ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  // Just like the images in the swapchain, buffers can also be owned by a specific queue family or be shared between multiple at the same time
  // Our buffer will only be used from the graphics queue, so we an stick to exclusive access
  // .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // The flags parameter is used to configure sparse buffer memory
  // Sparse bfufers in VUlkan refer to a memory management technique that allows more flexible and efficient use of GPU memory
  // This technique is particularly useful for handling large datasets, such as textures or vertex buffers, that might not fit contiguously in GPU
  // memory or that require efficient streaming of data in and out of GPU memory
  buf_ci.flags = 0;

  VK_Assert(vkCreateBuffer(r_vulkan_state->logical_device.h, &buf_ci, NULL, &ubo_buffer.buffer.h));
  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(r_vulkan_state->logical_device.h, ubo_buffer.buffer.h, &mem_requirements);

  VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
  VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  alloc_info.allocationSize  = mem_requirements.size;
  alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

  VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &ubo_buffer.buffer.memory));
  VK_Assert(vkBindBufferMemory(r_vulkan_state->logical_device.h, ubo_buffer.buffer.h, ubo_buffer.buffer.memory, 0));
  Assert(ubo_buffer.buffer.size != 0);
  VK_Assert(vkMapMemory(r_vulkan_state->logical_device.h, ubo_buffer.buffer.memory, 0, ubo_buffer.buffer.size, 0, &ubo_buffer.buffer.mapped));

  // Create descriptor set
  R_Vulkan_DescriptorSetKind ds_type = 0;
  switch(kind)
  {
    case R_Vulkan_UBOTypeKind_Rect:               {ds_type = R_Vulkan_DescriptorSetKind_UBO_Rect;}break;
    case R_Vulkan_UBOTypeKind_Geo2D:              {ds_type = R_Vulkan_DescriptorSetKind_UBO_Geo2D;}break;
    case R_Vulkan_UBOTypeKind_Geo3D:              {ds_type = R_Vulkan_DescriptorSetKind_UBO_Geo3D;}break;
    case R_Vulkan_UBOTypeKind_Geo3D_TileFrustum:  {ds_type = R_Vulkan_DescriptorSetKind_UBO_Geo3D_TileFrustum;}break;
    case R_Vulkan_UBOTypeKind_Geo3D_LightCulling: {ds_type = R_Vulkan_DescriptorSetKind_UBO_Geo3D_LightCulling;}break;
    default:                                      {InvalidPath;}break;
  }

  // TODO(k): we should set cap based on something, right?
  r_vulkan_descriptor_set_alloc(ds_type, 1, 3, &ubo_buffer.buffer.h, NULL, NULL, &ubo_buffer.set);
  return ubo_buffer;
}

internal R_Vulkan_SBOBuffer
r_vulkan_sbo_buffer_alloc(R_Vulkan_SBOTypeKind kind, U64 unit_count)
{
  R_Vulkan_SBOBuffer ret = {0};
  U64 stride = 0;

  B32 device_local = 0;
  B32 auto_mapped = 0;
  VkBufferUsageFlags flags = 0;
  // NOTE(k): we are expecting stride is equal to the size of the struct
  switch(kind)
  {
    case R_Vulkan_SBOTypeKind_Geo3D_Joints:
    {
      auto_mapped = 1;
      U64 array_size = sizeof(R_Vulkan_SBO_Geo3D_Joint) * R_MAX_JOINTS_PER_PASS;
      stride = AlignPow2(array_size, r_vulkan_pdevice()->properties.limits.minStorageBufferOffsetAlignment);
    }break;
    case R_Vulkan_SBOTypeKind_Geo3D_Materials:
    {
      auto_mapped = 1;
      U64 array_size = sizeof(R_Vulkan_SBO_Geo3D_Material) * R_MAX_MATERIALS_PER_PASS;
      stride = AlignPow2(array_size, r_vulkan_pdevice()->properties.limits.minStorageBufferOffsetAlignment);
    }break;
    case R_Vulkan_SBOTypeKind_Geo3D_Tiles:
    {
      device_local = 1;
      U64 array_size = sizeof(R_Vulkan_SBO_Geo3D_Tile) * R_VULKAN_MAX_TILES_PER_PASS;
      stride = AlignPow2(array_size, r_vulkan_pdevice()->properties.limits.minStorageBufferOffsetAlignment);
    }break;
    case R_Vulkan_SBOTypeKind_Geo3D_Lights:
    {
      auto_mapped = 1;
      U64 array_size = sizeof(R_Vulkan_SBO_Geo3D_Light) * R_MAX_LIGHTS_PER_PASS;
      stride = AlignPow2(array_size, r_vulkan_pdevice()->properties.limits.minStorageBufferOffsetAlignment);
    }break;
    case R_Vulkan_SBOTypeKind_Geo3D_LightIndices:
    {
      device_local = 1;
      // NOTE(k): we need to use vkFillBuffer to clear this buffer, hense this flag
      flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
      U64 array_size = sizeof(R_Vulkan_SBO_Geo3D_LightIndice) * R_VULKAN_MAX_LIGHTS_PER_TILE * R_VULKAN_MAX_TILES_PER_PASS;
      stride = AlignPow2(array_size, r_vulkan_pdevice()->properties.limits.minStorageBufferOffsetAlignment);
    }break;
    case R_Vulkan_SBOTypeKind_Geo3D_TileLights:
    {
      device_local = 1;
      U64 array_size = sizeof(R_Vulkan_SBO_Geo3D_TileLights) * R_VULKAN_MAX_TILES_PER_PASS;
      stride = AlignPow2(array_size, r_vulkan_pdevice()->properties.limits.minStorageBufferOffsetAlignment);
    }break;
    default:{InvalidPath;}break;
  }

  U64 buf_size = stride*unit_count;

  ret.unit_count  = unit_count;
  ret.stride      = stride;
  ret.buffer.size = buf_size;
  ret.buffer.cap  = buf_size;

  VkBufferCreateInfo buf_ci = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
  buf_ci.size = buf_size;
  buf_ci.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|flags;
  // Just like the images in the swapchain, buffers can also be owned by a specific queue family or be shared between multiple at the same time
  // Our buffer will only be used from the graphics queue, so we an stick to exclusive access
  buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // The flags parameter is used to configure sparse buffer memory
  // Sparse buffers in VUlkan refer to a memory management technique that allows more flexible and efficient use of GPU memory
  // This technique is particularly useful for handling large datasets, such as textures or vertex buffers, that might not fit contiguously in GPU
  // memory or that require efficient streaming of data in and out of GPU memory
  buf_ci.flags = 0;

  VK_Assert(vkCreateBuffer(r_vulkan_state->logical_device.h, &buf_ci, NULL, &ret.buffer.h));
  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(r_vulkan_state->logical_device.h, ret.buffer.h, &mem_requirements);

  VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
  VkMemoryPropertyFlags properties = device_local == 0 ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  alloc_info.allocationSize  = mem_requirements.size;
  alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

  VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &ret.buffer.memory));
  VK_Assert(vkBindBufferMemory(r_vulkan_state->logical_device.h, ret.buffer.h, ret.buffer.memory, 0));
  if(auto_mapped)
  {
    VK_Assert(vkMapMemory(r_vulkan_state->logical_device.h, ret.buffer.memory, 0, ret.buffer.size, 0, &ret.buffer.mapped));
  }

  // Create descriptor set
  R_Vulkan_DescriptorSetKind ds_type = 0;
  switch(kind)
  {
    case R_Vulkan_SBOTypeKind_Geo3D_Joints:       {ds_type = R_Vulkan_DescriptorSetKind_SBO_Geo3D_Joints;}break;
    case R_Vulkan_SBOTypeKind_Geo3D_Materials:    {ds_type = R_Vulkan_DescriptorSetKind_SBO_Geo3D_Materials;}break;
    case R_Vulkan_SBOTypeKind_Geo3D_Tiles:        {ds_type = R_Vulkan_DescriptorSetKind_SBO_Geo3D_Tiles;}break;
    case R_Vulkan_SBOTypeKind_Geo3D_Lights:       {ds_type = R_Vulkan_DescriptorSetKind_SBO_Geo3D_Lights;}break;
    case R_Vulkan_SBOTypeKind_Geo3D_LightIndices: {ds_type = R_Vulkan_DescriptorSetKind_SBO_Geo3D_LightIndices;}break;
    case R_Vulkan_SBOTypeKind_Geo3D_TileLights:   {ds_type = R_Vulkan_DescriptorSetKind_SBO_Geo3D_TileLights;}break;
    default:                                      {InvalidPath;}break;
  }

  r_vulkan_descriptor_set_alloc(ds_type, 1, 3, &ret.buffer.h, NULL, NULL, &ret.set);
  return ret;
}

//- render targets

internal R_Vulkan_RenderTargets *
r_vulkan_render_targets_alloc(OS_Handle os_wnd, R_Vulkan_Surface *surface, R_Vulkan_RenderTargets *old)
{
  R_Vulkan_RenderTargets *ret = r_vulkan_state->first_free_render_targets;
  R_Vulkan_Swapchain *old_swapchain = 0;

  if(ret == 0)
  {
    ret = push_array_no_zero(r_vulkan_state->arena, R_Vulkan_RenderTargets, 1);
  }
  else
  {
    SLLStackPop(r_vulkan_state->first_free_render_targets);
  }
  MemoryZeroStruct(ret);

  if(old != 0)
  {
    old_swapchain = &old->swapchain;
  }

  // Query format and color space
  VkFormat swp_format;
  VkColorSpaceKHR swp_color_space;
  r_vulkan_format_for_swapchain(surface->formats, surface->format_count, &swp_format, &swp_color_space);

  // Create swapchain
  ret->swapchain = r_vulkan_swapchain(surface, os_wnd, swp_format, swp_color_space, old_swapchain);

  R_Vulkan_Swapchain *swapchain                 = &ret->swapchain;
  R_Vulkan_Image *stage_color_image             = &ret->stage_color_image;
  R_Vulkan_DescriptorSet *stage_color_ds        = &ret->stage_color_ds;
  R_Vulkan_Image *stage_id_image                = &ret->stage_id_image;
  R_Vulkan_Buffer *stage_id_cpu                 = &ret->stage_id_cpu;
  R_Vulkan_Image *scratch_color_image           = &ret->scratch_color_image;
  R_Vulkan_DescriptorSet *scratch_color_ds      = &ret->scratch_color_ds;
  R_Vulkan_Image *geo2d_color_image             = &ret->geo2d_color_image;
  R_Vulkan_DescriptorSet *geo2d_color_ds        = &ret->geo2d_color_ds;
  R_Vulkan_Image *edge_image                    = &ret->edge_image;
  R_Vulkan_DescriptorSet *edge_ds               = &ret->edge_ds;
  R_Vulkan_Image *geo3d_color_image             = &ret->geo3d_color_image;
  R_Vulkan_DescriptorSet *geo3d_color_ds        = &ret->geo3d_color_ds;
  R_Vulkan_Image *geo3d_normal_depth_image      = &ret->geo3d_normal_depth_image;
  R_Vulkan_DescriptorSet *geo3d_normal_depth_ds = &ret->geo3d_normal_depth_ds;
  R_Vulkan_Image *geo3d_depth_image             = &ret->geo3d_depth_image;
  R_Vulkan_Image *geo3d_pre_depth_image         = &ret->geo3d_pre_depth_image;
  R_Vulkan_DescriptorSet *geo3d_pre_depth_ds    = &ret->geo3d_pre_depth_ds;

  // Create stage color image and its sampler descriptor set
  {
    stage_color_image->format         = swapchain->format;
    stage_color_image->extent.width   = swapchain->extent.width;
    stage_color_image->extent.height  = swapchain->extent.height;

    VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    create_info.imageType         = VK_IMAGE_TYPE_2D;
    create_info.extent.width      = stage_color_image->extent.width;
    create_info.extent.height     = stage_color_image->extent.height;
    create_info.extent.depth      = 1;
    create_info.mipLevels         = 1;
    create_info.arrayLayers       = 1;
    create_info.format            = stage_color_image->format;
    create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
    create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
    create_info.flags             = 0; // Optional
    VK_Assert(vkCreateImage(r_vulkan_state->logical_device.h, &create_info, NULL, &stage_color_image->h));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(r_vulkan_state->logical_device.h, stage_color_image->h, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; 
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);
    VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &stage_color_image->memory));
    VK_Assert(vkBindImageMemory(r_vulkan_state->logical_device.h, stage_color_image->h, stage_color_image->memory, 0));

    VkImageViewCreateInfo image_view_create_info = {
      .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image                           = stage_color_image->h,
      .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
      .format                          = stage_color_image->format,
      .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel   = 0,
      .subresourceRange.levelCount     = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount     = 1,
    };
    VK_Assert(vkCreateImageView(r_vulkan_state->logical_device.h, &image_view_create_info, NULL, &stage_color_image->view));

    // Create staging color sampler descriptor set
    r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind_Tex2D,
                                  1, 16, NULL, &stage_color_image->view,
                                  &r_vulkan_state->samplers[R_Tex2DSampleKind_Nearest],
                                  stage_color_ds);
  }

  // stage_id color image
  {
    stage_id_image->format        = VK_FORMAT_R32G32B32A32_SFLOAT;
    stage_id_image->extent.width  = swapchain->extent.width;
    stage_id_image->extent.height = swapchain->extent.height;

    VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    create_info.imageType         = VK_IMAGE_TYPE_2D;
    create_info.extent.width      = stage_id_image->extent.width;
    create_info.extent.height     = stage_id_image->extent.height;
    create_info.extent.depth      = 1;
    create_info.mipLevels         = 1;
    create_info.arrayLayers       = 1;
    create_info.format            = stage_id_image->format;
    create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
    create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
    create_info.flags             = 0;

    VK_Assert(vkCreateImage(r_vulkan_state->logical_device.h, &create_info, NULL, &stage_id_image->h));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(r_vulkan_state->logical_device.h, stage_id_image->h, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);
    VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &stage_id_image->memory));
    VK_Assert(vkBindImageMemory(r_vulkan_state->logical_device.h, stage_id_image->h, stage_id_image->memory, 0));

    VkImageViewCreateInfo image_view_create_info =
    {
      .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image                           = stage_id_image->h,
      .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
      .format                          = stage_id_image->format,
      .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel   = 0,
      .subresourceRange.levelCount     = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount     = 1,
    };
    VK_Assert(vkCreateImageView(r_vulkan_state->logical_device.h, &image_view_create_info, NULL, &stage_id_image->view));
  }

  // stage_id_cpu
  {
    // VkDeviceSize size = geo3d_id_image->extent.width * geo3d_id_image->extent.height * sizeof(U64);
    VkDeviceSize size = 1*1*sizeof(Vec4F32);
    VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    create_info.size = size;
    create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_Assert(vkCreateBuffer(r_vulkan_state->logical_device.h, &create_info, NULL, &stage_id_cpu->h));
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(r_vulkan_state->logical_device.h, stage_id_cpu->h, &mem_requirements);
    stage_id_cpu->size = mem_requirements.size;

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

    VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &stage_id_cpu->memory));
    VK_Assert(vkBindBufferMemory(r_vulkan_state->logical_device.h, stage_id_cpu->h, stage_id_cpu->memory, 0));
    Assert(stage_id_cpu->size != 0);
    VK_Assert(vkMapMemory(r_vulkan_state->logical_device.h, stage_id_cpu->memory, 0, stage_id_cpu->size, 0, &stage_id_cpu->mapped));
  }

  // edge image and its sampler descriptor set
  {
    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    VkExtent2D extent = swapchain->extent;
    VkImage image_handle = {0};

    VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    create_info.imageType     = VK_IMAGE_TYPE_2D;
    create_info.extent.width  = extent.width;
    create_info.extent.height = extent.height;
    create_info.extent.depth  = 1;
    create_info.mipLevels     = 1;
    create_info.arrayLayers   = 1;
    create_info.format        = format;
    create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    create_info.flags         = 0; // Optional
    VK_Assert(vkCreateImage(r_vulkan_state->logical_device.h, &create_info, NULL, &image_handle));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(r_vulkan_state->logical_device.h, image_handle, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

    VkDeviceMemory memory = {0};
    VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &memory));
    VK_Assert(vkBindImageMemory(r_vulkan_state->logical_device.h, image_handle, memory, 0));

    VkImageViewCreateInfo image_view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    
    image_view_create_info.image                           = image_handle;
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = format;
    image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel   = 0;
    image_view_create_info.subresourceRange.levelCount     = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;
    
    VkImageView image_view = {0};
    VK_Assert(vkCreateImageView(r_vulkan_state->logical_device.h, &image_view_create_info, NULL, &image_view));

    R_Vulkan_DescriptorSet ds = {0};
    // create sampler descriptor set
    r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind_Tex2D,
                                  1, 16, NULL, &image_view,
                                  &r_vulkan_state->samplers[R_Tex2DSampleKind_Nearest],
                                  &ds);

    // copy values
    edge_image->h = image_handle;
    edge_image->format = format;
    edge_image->extent = extent;
    edge_image->memory = memory;
    edge_image->view = image_view;
    *edge_ds = ds;
  }

  // geo2d color image and its sampler descriptor set
  {
    geo2d_color_image->format        = swapchain->format;
    geo2d_color_image->extent.width  = swapchain->extent.width;
    geo2d_color_image->extent.height = swapchain->extent.height;

    VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    create_info.imageType         = VK_IMAGE_TYPE_2D;
    create_info.extent.width      = geo2d_color_image->extent.width;
    create_info.extent.height     = geo2d_color_image->extent.height;
    create_info.extent.depth      = 1;
    create_info.mipLevels         = 1;
    create_info.arrayLayers       = 1;
    create_info.format            = geo2d_color_image->format;
    create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
    create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
    create_info.flags             = 0; // Optional
    VK_Assert(vkCreateImage(r_vulkan_state->logical_device.h, &create_info, NULL, &geo2d_color_image->h));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(r_vulkan_state->logical_device.h, geo2d_color_image->h, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

    VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &geo2d_color_image->memory));
    VK_Assert(vkBindImageMemory(r_vulkan_state->logical_device.h, geo2d_color_image->h, geo2d_color_image->memory, 0));

    VkImageViewCreateInfo image_view_create_info = {
      .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image                           = geo2d_color_image->h,
      .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
      .format                          = geo2d_color_image->format,
      .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel   = 0,
      .subresourceRange.levelCount     = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount     = 1,
    };
    VK_Assert(vkCreateImageView(r_vulkan_state->logical_device.h, &image_view_create_info, NULL, &geo2d_color_image->view));

    // Create geo3d_color sampler descriptor set
    r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind_Tex2D,
                                  1, 16, NULL, &geo2d_color_image->view,
                                  &r_vulkan_state->samplers[R_Tex2DSampleKind_Nearest],
                                  geo2d_color_ds);
  }

  // scratch color image and its sampler descriptor set
  {
    R_Vulkan_Image *image = scratch_color_image;
    R_Vulkan_DescriptorSet *ds = scratch_color_ds;
    VkFormat format = swapchain->format;
    VkExtent2D extent = swapchain->extent;

    VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    create_info.imageType         = VK_IMAGE_TYPE_2D;
    create_info.extent.width      = extent.width;
    create_info.extent.height     = extent.height;
    create_info.extent.depth      = 1;
    create_info.mipLevels         = 1;
    create_info.arrayLayers       = 1;
    create_info.format            = format;
    create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
    create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    // TODO(XXX): revisit the usage flags
    create_info.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
    create_info.flags             = 0; // Optional
    VK_Assert(vkCreateImage(r_vulkan_state->logical_device.h, &create_info, NULL, &image->h));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(r_vulkan_state->logical_device.h, image->h, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; 
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);
    VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &image->memory));
    VK_Assert(vkBindImageMemory(r_vulkan_state->logical_device.h, image->h, image->memory, 0));

    VkImageViewCreateInfo image_view_create_info = {
      .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image                           = image->h,
      .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
      .format                          = format,
      .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel   = 0,
      .subresourceRange.levelCount     = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount     = 1,
    };
    VK_Assert(vkCreateImageView(r_vulkan_state->logical_device.h, &image_view_create_info, NULL, &image->view));

    // Create staging color sampler descriptor set
    r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind_Tex2D,
                                  1, 16, NULL, &image->view,
                                  &r_vulkan_state->samplers[R_Tex2DSampleKind_Nearest],
                                  ds);
    image->format = format;
    image->extent = extent;
  }

  // geo3d color image and its sampler descriptor set
  {
    geo3d_color_image->format        = swapchain->format;
    geo3d_color_image->extent.width  = swapchain->extent.width;
    geo3d_color_image->extent.height = swapchain->extent.height;

    VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    create_info.imageType         = VK_IMAGE_TYPE_2D;
    create_info.extent.width      = geo3d_color_image->extent.width;
    create_info.extent.height     = geo3d_color_image->extent.height;
    create_info.extent.depth      = 1;
    create_info.mipLevels         = 1;
    create_info.arrayLayers       = 1;
    create_info.format            = geo3d_color_image->format;
    create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
    create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
    create_info.flags             = 0; // Optional
    VK_Assert(vkCreateImage(r_vulkan_state->logical_device.h, &create_info, NULL, &geo3d_color_image->h));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(r_vulkan_state->logical_device.h, geo3d_color_image->h, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

    VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &geo3d_color_image->memory));
    VK_Assert(vkBindImageMemory(r_vulkan_state->logical_device.h, geo3d_color_image->h, geo3d_color_image->memory, 0));

    VkImageViewCreateInfo image_view_create_info = {
      .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image                           = geo3d_color_image->h,
      .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
      .format                          = geo3d_color_image->format,
      .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel   = 0,
      .subresourceRange.levelCount     = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount     = 1,
    };
    VK_Assert(vkCreateImageView(r_vulkan_state->logical_device.h, &image_view_create_info, NULL, &geo3d_color_image->view));

    // create geo3d_color sampler descriptor set
    r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind_Tex2D,
        1, 16, NULL, &geo3d_color_image->view,
        &r_vulkan_state->samplers[R_Tex2DSampleKind_Nearest],
        geo3d_color_ds);
  }

  // create geo3d_normal_depth_image and its sampler descriptor set
  {
    geo3d_normal_depth_image->format        = VK_FORMAT_R32G32B32A32_SFLOAT; // first 3 is normal, last w is depth
    geo3d_normal_depth_image->extent.width  = swapchain->extent.width;
    geo3d_normal_depth_image->extent.height = swapchain->extent.height;

    VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    create_info.imageType         = VK_IMAGE_TYPE_2D;
    create_info.extent.width      = geo3d_normal_depth_image->extent.width;
    create_info.extent.height     = geo3d_normal_depth_image->extent.height;
    create_info.extent.depth      = 1;
    create_info.mipLevels         = 1;
    create_info.arrayLayers       = 1;
    create_info.format            = geo3d_normal_depth_image->format;
    create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
    create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
    create_info.flags             = 0; // Optional
    VK_Assert(vkCreateImage(r_vulkan_state->logical_device.h, &create_info, NULL, &geo3d_normal_depth_image->h));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(r_vulkan_state->logical_device.h, geo3d_normal_depth_image->h, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

    VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &geo3d_normal_depth_image->memory));
    VK_Assert(vkBindImageMemory(r_vulkan_state->logical_device.h, geo3d_normal_depth_image->h, geo3d_normal_depth_image->memory, 0));

    VkImageViewCreateInfo image_view_create_info =
    {
      .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image                           = geo3d_normal_depth_image->h,
      .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
      .format                          = geo3d_normal_depth_image->format,
      .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel   = 0,
      .subresourceRange.levelCount     = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount     = 1,
    };
    VK_Assert(vkCreateImageView(r_vulkan_state->logical_device.h, &image_view_create_info, NULL, &geo3d_normal_depth_image->view));

    // create sampler descriptor set
    r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind_Tex2D,
        1, 16, NULL, &geo3d_normal_depth_image->view,
        &r_vulkan_state->samplers[R_Tex2DSampleKind_Nearest],
        geo3d_normal_depth_ds);
  }

  // create geo3d_depth_image
  {
    geo3d_depth_image->format        = r_vulkan_pdevice()->depth_image_format;
    geo3d_depth_image->extent.width  = swapchain->extent.width;
    geo3d_depth_image->extent.height = swapchain->extent.height;

    VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    // Tells Vulkan with what kind of coordinate system the texels in the image are going to be addressed
    // It's possbiel to create 1D, 2D and 3D images
    // One dimensional images can be used to store an array of data or gradient
    // Two dimensional images are mainly used for textures, and three dimensional images can be used to store voxel volumes, for example
    // The extent field specifies the dimensions of the image, basically how many texels there are on each axis
    // That's why depth must be 1 instead of 0
    create_info.imageType     = VK_IMAGE_TYPE_2D;
    create_info.extent.width  = geo3d_depth_image->extent.width;
    create_info.extent.height = geo3d_depth_image->extent.height;
    create_info.extent.depth  = 1;
    create_info.mipLevels     = 1;
    create_info.arrayLayers   = 1;
    // Vulkan supports many possible image formats, but we should use the same format for the texels as the pixels in the buffer
    //      otherwise the copy operations will fail
    // It is possible that the VK_FORMAT_R8G8B8A8_SRGB is not supported by the graphics hardware
    // You should have a list of acceptable alternatives and go with the best one that is supported
    // However, support for this particular for this particular format is so widespread that we'll skip this step
    // Using different formats would also require annoying conversions
    create_info.format = r_vulkan_pdevice()->depth_image_format;
    // The tiling field can have one of two values:
    // 1.  VK_IMAGE_TILING_LINEAR: texels are laid out in row-major order like our pixels array
    // 2. VK_IMAGE_TILING_OPTIMAL: texels are laid out in an implementation defined order for optimal access 
    // Unlike the layout of an image, the tiling mode cannot be changed at a later time 
    // If you want to be able to directly access texels in the memory of the image, then you must use VK_IMAGE_TILING_LINEAR layout
    // We will be using a staging buffer instead of staging image, so this won't be necessary
    //      we will be using VK_IMAGE_TILING_OPTIMAL for efficient access from the shader
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    // There are only two possbile values for the initialLayout of an image
    // 1.      VK_IMAGE_LAYOUT_UNDEFINED: not usable by the GPU and the very first transition will discard the texels
    // 2. VK_IMAGE_LAYOUT_PREINITIALIZED: not usable by the GPU, but the first transition will preserve the texels
    // There are few situations where it is necessary for the texels to be preserved during the first transition
    // One example, however, would be if you wanted to use an image as staging image in combination with VK_IMAGE_TILING_LINEAR layout
    // In that case, you'd want to upload the texel data to it and then transition the image to be transfer source without losing the data
    // In out case, however, we're first going to transition the image to be transfer destination and then copy texel data to it from a buffer object
    // So we don't need this property and can safely use VK_IMAGE_LAYOUT_UNDEFINED
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // The image is going to be used as destination for the buffer copy
    // We also want to be able to access the image from the shader to color our mesh
    create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    // The image will only be used by one queue family: the one that supports graphics (and therefor also) transfer operations
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // The samples flag is related to multisampling
    // This is only relevant for images that will be used as attachments, so stick to one sample
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    // There are some optional flags for images that are related to sparse images
    // Sparse images are images where only certain regions are actually backed by memory
    // If you were using a 3D texture for a voxel terrain, for example
    //      then you could use this avoid allocating memory to storage large volumes of "air" values
    create_info.flags = 0; // Optional
    VK_Assert(vkCreateImage(r_vulkan_state->logical_device.h, &create_info, NULL, &geo3d_depth_image->h));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(r_vulkan_state->logical_device.h, geo3d_depth_image->h, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

    VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &geo3d_depth_image->memory));
    VK_Assert(vkBindImageMemory(r_vulkan_state->logical_device.h, geo3d_depth_image->h, geo3d_depth_image->memory, 0));

    VkImageViewCreateInfo image_view_create_info =
    {
      .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image                           = geo3d_depth_image->h,
      .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
      .format                          = geo3d_depth_image->format,
      .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
      .subresourceRange.baseMipLevel   = 0,
      .subresourceRange.levelCount     = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount     = 1,
    };
    VK_Assert(vkCreateImageView(r_vulkan_state->logical_device.h, &image_view_create_info, NULL, &geo3d_depth_image->view));
  }

  // Create geo3d_pre_depth_image (for z pre pass)
  {
    geo3d_pre_depth_image->format        = r_vulkan_pdevice()->depth_image_format;
    geo3d_pre_depth_image->extent.width  = swapchain->extent.width;
    geo3d_pre_depth_image->extent.height = swapchain->extent.height;

    VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    create_info.imageType     = VK_IMAGE_TYPE_2D;
    create_info.extent.width  = geo3d_pre_depth_image->extent.width;
    create_info.extent.height = geo3d_pre_depth_image->extent.height;
    create_info.extent.depth  = 1;
    create_info.mipLevels     = 1;
    create_info.arrayLayers   = 1;
    create_info.format        = r_vulkan_pdevice()->depth_image_format;
    create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    create_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    create_info.flags         = 0; // Optional
    VK_Assert(vkCreateImage(r_vulkan_state->logical_device.h, &create_info, NULL, &geo3d_pre_depth_image->h));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(r_vulkan_state->logical_device.h, geo3d_pre_depth_image->h, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

    VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &geo3d_pre_depth_image->memory));
    VK_Assert(vkBindImageMemory(r_vulkan_state->logical_device.h, geo3d_pre_depth_image->h, geo3d_pre_depth_image->memory, 0));

    VkImageViewCreateInfo image_view_create_info =
    {
      .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image                           = geo3d_pre_depth_image->h,
      .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
      .format                          = geo3d_pre_depth_image->format,
      .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
      .subresourceRange.baseMipLevel   = 0,
      .subresourceRange.levelCount     = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount     = 1,
    };
    VK_Assert(vkCreateImageView(r_vulkan_state->logical_device.h, &image_view_create_info, NULL, &geo3d_pre_depth_image->view));

    // create sampler descriptor set
    r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind_Tex2D,
                                  1,16,NULL,&geo3d_pre_depth_image->view,
                                  &r_vulkan_state->samplers[R_Tex2DSampleKind_Nearest],
                                  geo3d_pre_depth_ds);
  }
  return ret;
}

internal void
r_vulkan_render_targets_destroy(R_Vulkan_RenderTargets *render_targets)
{
  // destroy swapchain
  for(U64 i = 0; i < render_targets->swapchain.image_count; i++)
  {
    vkDestroyImageView(r_vulkan_state->logical_device.h, render_targets->swapchain.image_views[i], NULL);

    // TODO(XXX): we should be able to reuse these semaphores
    // TODO(BUG): it's not safe to destroy semaphore here, fix it later
    //            can't be called on VkSemaphore 0x4a000000004a that is currently in use by VkQueue
    vkDestroySemaphore(r_vulkan_state->logical_device.h, render_targets->swapchain.submit_semaphores[i], NULL);
  }
  vkDestroySwapchainKHR(r_vulkan_state->logical_device.h, render_targets->swapchain.h, NULL);

  // stage color image
  vkDestroyImageView(r_vulkan_state->logical_device.h, render_targets->stage_color_image.view, NULL);
  vkDestroyImage(r_vulkan_state->logical_device.h, render_targets->stage_color_image.h, NULL);
  r_vulkan_descriptor_set_destroy(&render_targets->stage_color_ds);
  vkFreeMemory(r_vulkan_state->logical_device.h, render_targets->stage_color_image.memory, NULL);

  // stage id image
  vkDestroyImageView(r_vulkan_state->logical_device.h, render_targets->stage_id_image.view, NULL);
  vkDestroyImage(r_vulkan_state->logical_device.h, render_targets->stage_id_image.h, NULL);
  vkFreeMemory(r_vulkan_state->logical_device.h, render_targets->stage_id_image.memory, NULL);

  // stage id buffer (cpu)
  vkUnmapMemory(r_vulkan_state->logical_device.h, render_targets->stage_id_cpu.memory);
  vkDestroyBuffer(r_vulkan_state->logical_device.h, render_targets->stage_id_cpu.h, NULL);
  vkFreeMemory(r_vulkan_state->logical_device.h, render_targets->stage_id_cpu.memory, NULL);

  // edge image
  vkDestroyImageView(r_vulkan_state->logical_device.h, render_targets->edge_image.view, NULL);
  vkDestroyImage(r_vulkan_state->logical_device.h, render_targets->edge_image.h, NULL);
  r_vulkan_descriptor_set_destroy(&render_targets->edge_ds);
  vkFreeMemory(r_vulkan_state->logical_device.h, render_targets->edge_image.memory, NULL);

  // geo2d color image
  vkDestroyImageView(r_vulkan_state->logical_device.h, render_targets->geo2d_color_image.view, NULL);
  vkDestroyImage(r_vulkan_state->logical_device.h, render_targets->geo2d_color_image.h, NULL);
  r_vulkan_descriptor_set_destroy(&render_targets->geo2d_color_ds);
  vkFreeMemory(r_vulkan_state->logical_device.h, render_targets->geo2d_color_image.memory, NULL);

  // geo3d color image
  vkDestroyImageView(r_vulkan_state->logical_device.h, render_targets->geo3d_color_image.view, NULL);
  vkDestroyImage(r_vulkan_state->logical_device.h, render_targets->geo3d_color_image.h, NULL);
  r_vulkan_descriptor_set_destroy(&render_targets->geo3d_color_ds);
  vkFreeMemory(r_vulkan_state->logical_device.h, render_targets->geo3d_color_image.memory, NULL);

  // geo3d normal depth image
  vkDestroyImageView(r_vulkan_state->logical_device.h, render_targets->geo3d_normal_depth_image.view, NULL);
  vkDestroyImage(r_vulkan_state->logical_device.h, render_targets->geo3d_normal_depth_image.h, NULL);
  r_vulkan_descriptor_set_destroy(&render_targets->geo3d_normal_depth_ds);
  vkFreeMemory(r_vulkan_state->logical_device.h, render_targets->geo3d_normal_depth_image.memory, NULL);

  // geo3d depth image
  vkDestroyImageView(r_vulkan_state->logical_device.h, render_targets->geo3d_depth_image.view, NULL);
  vkDestroyImage(r_vulkan_state->logical_device.h, render_targets->geo3d_depth_image.h, NULL);
  vkFreeMemory(r_vulkan_state->logical_device.h, render_targets->geo3d_depth_image.memory, NULL);

  // geo3d_pre_depth_image
  vkDestroyImageView(r_vulkan_state->logical_device.h, render_targets->geo3d_pre_depth_image.view, NULL);
  vkDestroyImage(r_vulkan_state->logical_device.h, render_targets->geo3d_pre_depth_image.h, NULL);
  r_vulkan_descriptor_set_destroy(&render_targets->geo3d_pre_depth_ds);
  vkFreeMemory(r_vulkan_state->logical_device.h, render_targets->geo3d_pre_depth_image.memory, NULL);
}

//- descriptor

// TODO(k): ugly, split into alloc and update
internal void
r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind kind, U64 set_count, U64 cap, VkBuffer *buffers, VkImageView *image_views, VkSampler *samplers, R_Vulkan_DescriptorSet *sets)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);
  R_Vulkan_DescriptorSetLayout set_layout = r_vulkan_state->set_layouts[kind];

  U64 remaining = set_count;
  R_Vulkan_DescriptorSetPool *pool = 0;

  while(remaining > 0)
  {
    pool = r_vulkan_state->first_avail_ds_pool[kind];
    if(pool == 0)
    {
      pool = push_array(r_vulkan_state->arena, R_Vulkan_DescriptorSetPool, 1);
      pool->kind = kind;
      pool->cmt  = 0;
      pool->cap  = AlignPow2(cap, 16);

      VkDescriptorPoolSize *pool_sizes = push_array(scratch.arena, VkDescriptorPoolSize, set_layout.binding_count);
      for(U64 i = 0; i < set_layout.binding_count; i++)
      {
        pool_sizes[i].type = set_layout.bindings[i].descriptorType;
        pool_sizes[i].descriptorCount = set_layout.bindings[i].descriptorCount * pool->cap;
      }

      VkDescriptorPoolCreateInfo pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = pool_sizes,
        // Aside from the maxium number of individual descriptors that are available, we also need to specify the maxium number of descriptor sets that may be allcoated
        .maxSets = pool->cap,
        // The structure has an optional flag similar to command pools that determines if individual descriptor sets can be freed or not
        // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
        // We're not going to touch the descriptor set after creating it, so we don't nedd this flag 
        // You can leave flags to 0
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      };
      VK_Assert(vkCreateDescriptorPool(r_vulkan_state->logical_device.h, &pool_create_info, NULL, &pool->h));

    }
    else
    {
      SLLStackPop(r_vulkan_state->first_avail_ds_pool[kind]);
    }

    U64 alloc_count = (pool->cap - pool->cmt);
    Assert(alloc_count > 0);
    if(remaining < alloc_count) alloc_count = remaining;

    VkDescriptorSet *temp_sets = push_array(scratch.arena, VkDescriptorSet, alloc_count);
    VkDescriptorSetAllocateInfo set_alloc_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    set_alloc_info.descriptorPool     = pool->h;
    set_alloc_info.descriptorSetCount = alloc_count;
    set_alloc_info.pSetLayouts        = &set_layout.h;
    VK_Assert(vkAllocateDescriptorSets(r_vulkan_state->logical_device.h, &set_alloc_info, temp_sets));

    for(U64 i = 0; i < alloc_count; i++) 
    {
      U64 offset = set_count-remaining;
      sets[i+offset].h = temp_sets[i];
      sets[i+offset].pool = pool;
    }

    pool->cmt += alloc_count;
    remaining -= alloc_count;

    if(pool->cmt < pool->cap)
    {
      SLLStackPush(r_vulkan_state->first_avail_ds_pool[kind], pool);
    }
  }

  // NOTE(k): set could have multiple writes due multiple bindings
  U64 writes_count = set_count * set_layout.binding_count;
  VkWriteDescriptorSet *writes = push_array(scratch.arena, VkWriteDescriptorSet, writes_count);

  for(U64 i = 0; i < set_count; i++)
  {
    for(U64 j = 0; j < set_layout.binding_count; j++)
    {
      U64 set_idx = i + j;
      // TODO(XXX): revisit is needed
      switch(kind)
      {
        case R_Vulkan_DescriptorSetKind_UBO_Rect:
        {
          VkDescriptorBufferInfo *buffer_info = push_array(scratch.arena, VkDescriptorBufferInfo, 1);
          buffer_info->buffer = buffers[i];
          buffer_info->offset = 0;
          buffer_info->range  = sizeof(R_Vulkan_UBO_Rect);

          writes[set_idx].sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writes[set_idx].dstSet = sets[i].h;
          // The first two fields specify the descriptor set to update and the binding 
          // We gave our uniform buffer binding index 0
          // Remember that descriptors can be arrays, so we also need to specify the first index in the array that we want to update
          // It's possible to update multiple descriptors at once in an array
          writes[set_idx].dstBinding      = j;
          writes[set_idx].dstArrayElement = 0; // start updating from the first element
          writes[set_idx].descriptorCount = set_layout.bindings[j].descriptorCount; // number of descriptors to update
          writes[set_idx].descriptorType  = set_layout.bindings[j].descriptorType;
          // The pBufferInfo field is used for descriptors that refer to buffer data, pImageInfo is used for descriptors aht refer to iamge data, and pTexelBufferView is used for descriptor that refer to buffer views
          // Our descriptor is based on buffers, so we're using pBufferInfo
          writes[set_idx].pBufferInfo      = buffer_info;
          writes[set_idx].pImageInfo       = NULL; // Optional
          writes[set_idx].pTexelBufferView = NULL; // Optional
        }break;
        case R_Vulkan_DescriptorSetKind_UBO_Geo2D:
        {
          VkDescriptorBufferInfo *buffer_info = push_array(scratch.arena, VkDescriptorBufferInfo, 1);
          buffer_info->buffer = buffers[i];
          buffer_info->offset = 0;
          buffer_info->range  = sizeof(R_Vulkan_UBO_Geo2D);

          writes[set_idx].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writes[set_idx].dstSet           = sets[i].h;
          writes[set_idx].dstBinding       = j;
          writes[set_idx].dstArrayElement  = 0; // start updating from the first element
          writes[set_idx].descriptorCount  = set_layout.bindings[j].descriptorCount;
          writes[set_idx].descriptorType   = set_layout.bindings[j].descriptorType;
          writes[set_idx].pBufferInfo      = buffer_info;
          writes[set_idx].pImageInfo       = NULL; // Optional
          writes[set_idx].pTexelBufferView = NULL; // Optional
        }break;
        case R_Vulkan_DescriptorSetKind_UBO_Geo3D:
        {
          VkDescriptorBufferInfo *buffer_info = push_array(scratch.arena, VkDescriptorBufferInfo, 1);
          buffer_info->buffer = buffers[i];
          buffer_info->offset = 0;
          buffer_info->range  = sizeof(R_Vulkan_UBO_Geo3D);

          writes[set_idx].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writes[set_idx].dstSet           = sets[i].h;
          writes[set_idx].dstBinding       = j;
          writes[set_idx].dstArrayElement  = 0; // start updating from the first element
          writes[set_idx].descriptorCount  = set_layout.bindings[j].descriptorCount;
          writes[set_idx].descriptorType   = set_layout.bindings[j].descriptorType;
          writes[set_idx].pBufferInfo      = buffer_info;
          writes[set_idx].pImageInfo       = NULL; // Optional
          writes[set_idx].pTexelBufferView = NULL; // Optional
        }break;
        case R_Vulkan_DescriptorSetKind_SBO_Geo3D_Joints:
        {
          VkDescriptorBufferInfo *buffer_info = push_array(scratch.arena, VkDescriptorBufferInfo, 1);
          buffer_info->buffer = buffers[i];
          buffer_info->offset = 0;
          // NOTE: we want to access it as an array of R_Vulkan_Storage_Mesh 
          buffer_info->range  = R_MAX_JOINTS_PER_PASS*sizeof(R_Vulkan_SBO_Geo3D_Joint);

          writes[set_idx].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writes[set_idx].dstSet           = sets[i].h;
          writes[set_idx].dstBinding       = j;
          writes[set_idx].dstArrayElement  = 0; // start updating from the first element
          writes[set_idx].descriptorCount  = set_layout.bindings[j].descriptorCount;
          writes[set_idx].descriptorType   = set_layout.bindings[j].descriptorType;
          writes[set_idx].pBufferInfo      = buffer_info;
          writes[set_idx].pImageInfo       = NULL; // Optional
          writes[set_idx].pTexelBufferView = NULL; // Optional
        }break;
        case R_Vulkan_DescriptorSetKind_SBO_Geo3D_Materials:
        {
          VkDescriptorBufferInfo *buffer_info = push_array(scratch.arena, VkDescriptorBufferInfo, 1);
          buffer_info->buffer = buffers[i];
          buffer_info->offset = 0;
          // NOTE: we want to access it as an array of R_Vulkan_Storage_Mesh 
          buffer_info->range  = R_MAX_MATERIALS_PER_PASS*sizeof(R_Vulkan_SBO_Geo3D_Material);

          writes[set_idx].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writes[set_idx].dstSet           = sets[i].h;
          writes[set_idx].dstBinding       = j;
          writes[set_idx].dstArrayElement  = 0; // start updating from the first element
          writes[set_idx].descriptorCount  = set_layout.bindings[j].descriptorCount;
          writes[set_idx].descriptorType   = set_layout.bindings[j].descriptorType;
          writes[set_idx].pBufferInfo      = buffer_info;
          writes[set_idx].pImageInfo       = NULL; // Optional
          writes[set_idx].pTexelBufferView = NULL; // Optional
        }break;
        case R_Vulkan_DescriptorSetKind_Tex2D:
        {
          VkDescriptorImageInfo *image_info = push_array(scratch.arena, VkDescriptorImageInfo, 1);
          image_info->sampler     = samplers[i];
          image_info->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
          image_info->imageView   = image_views[i];

          writes[set_idx].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writes[set_idx].dstSet           = sets[i].h;
          writes[set_idx].dstBinding       = j;
          writes[set_idx].dstArrayElement  = 0; // start updating from the first element
          writes[set_idx].descriptorCount  = set_layout.bindings[j].descriptorCount;
          writes[set_idx].descriptorType   = set_layout.bindings[j].descriptorType;
          writes[set_idx].pBufferInfo      = NULL;
          writes[set_idx].pImageInfo       = image_info; // Optional
          writes[set_idx].pTexelBufferView = NULL; // Optional
        }break;
        case R_Vulkan_DescriptorSetKind_UBO_Geo3D_TileFrustum:
        {
          VkDescriptorBufferInfo *buffer_info = push_array(scratch.arena, VkDescriptorBufferInfo, 1);
          buffer_info->buffer = buffers[i];
          buffer_info->offset = 0;
          buffer_info->range  = sizeof(R_Vulkan_UBO_Geo3D_TileFrustum);

          writes[set_idx].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writes[set_idx].dstSet           = sets[i].h;
          writes[set_idx].dstBinding       = j;
          writes[set_idx].dstArrayElement  = 0; // start updating from the first element
          writes[set_idx].descriptorCount  = set_layout.bindings[j].descriptorCount;
          writes[set_idx].descriptorType   = set_layout.bindings[j].descriptorType;
          writes[set_idx].pBufferInfo      = buffer_info;
          writes[set_idx].pImageInfo       = NULL; // Optional
          writes[set_idx].pTexelBufferView = NULL; // Optional
        }break;
        case R_Vulkan_DescriptorSetKind_UBO_Geo3D_LightCulling:
        {
          VkDescriptorBufferInfo *buffer_info = push_array(scratch.arena, VkDescriptorBufferInfo, 1);
          buffer_info->buffer = buffers[i];
          buffer_info->offset = 0;
          buffer_info->range  = sizeof(R_Vulkan_UBO_Geo3D_LightCulling);

          writes[set_idx].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writes[set_idx].dstSet           = sets[i].h;
          writes[set_idx].dstBinding       = j;
          writes[set_idx].dstArrayElement  = 0; // start updating from the first element
          writes[set_idx].descriptorCount  = set_layout.bindings[j].descriptorCount;
          writes[set_idx].descriptorType   = set_layout.bindings[j].descriptorType;
          writes[set_idx].pBufferInfo      = buffer_info;
          writes[set_idx].pImageInfo       = NULL; // Optional
          writes[set_idx].pTexelBufferView = NULL; // Optional
        }break;
        case R_Vulkan_DescriptorSetKind_SBO_Geo3D_Tiles:
        {
          VkDescriptorBufferInfo *buffer_info = push_array(scratch.arena, VkDescriptorBufferInfo, 1);
          buffer_info->buffer = buffers[i];
          buffer_info->offset = 0;
          buffer_info->range  = R_VULKAN_MAX_TILES_PER_PASS*sizeof(R_Vulkan_SBO_Geo3D_Tile);

          writes[set_idx].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writes[set_idx].dstSet           = sets[i].h;
          writes[set_idx].dstBinding       = j;
          writes[set_idx].dstArrayElement  = 0; // start updating from the first element
          writes[set_idx].descriptorCount  = set_layout.bindings[j].descriptorCount;
          writes[set_idx].descriptorType   = set_layout.bindings[j].descriptorType;
          writes[set_idx].pBufferInfo      = buffer_info;
          writes[set_idx].pImageInfo       = NULL;
          writes[set_idx].pTexelBufferView = NULL;
        }break;
        case R_Vulkan_DescriptorSetKind_SBO_Geo3D_Lights:
        {
          VkDescriptorBufferInfo *buffer_info = push_array(scratch.arena, VkDescriptorBufferInfo, 1);
          buffer_info->buffer = buffers[i];
          buffer_info->offset = 0;
          // NOTE: we want to access it as an array of R_Vulkan_SBO_Lights
          buffer_info->range  = R_MAX_LIGHTS_PER_PASS*sizeof(R_Vulkan_SBO_Geo3D_Light);

          writes[set_idx].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writes[set_idx].dstSet           = sets[i].h;
          writes[set_idx].dstBinding       = j;
          writes[set_idx].dstArrayElement  = 0; // start updating from the first element
          writes[set_idx].descriptorCount  = set_layout.bindings[j].descriptorCount;
          writes[set_idx].descriptorType   = set_layout.bindings[j].descriptorType;
          writes[set_idx].pBufferInfo      = buffer_info;
          writes[set_idx].pImageInfo       = NULL;
          writes[set_idx].pTexelBufferView = NULL;
        }break;
        case R_Vulkan_DescriptorSetKind_SBO_Geo3D_LightIndices:
        {
          VkDescriptorBufferInfo *buffer_info = push_array(scratch.arena, VkDescriptorBufferInfo, 1);
          buffer_info->buffer = buffers[i];
          buffer_info->offset = 0;
          // NOTE: we want to access it as an array of U32
          buffer_info->range  = R_VULKAN_MAX_LIGHTS_PER_TILE*R_VULKAN_MAX_TILES_PER_PASS*sizeof(R_Vulkan_SBO_Geo3D_LightIndice);

          writes[set_idx].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writes[set_idx].dstSet           = sets[i].h;
          writes[set_idx].dstBinding       = j;
          writes[set_idx].dstArrayElement  = 0; // start updating from the first element
          writes[set_idx].descriptorCount  = set_layout.bindings[j].descriptorCount;
          writes[set_idx].descriptorType   = set_layout.bindings[j].descriptorType;
          writes[set_idx].pBufferInfo      = buffer_info;
          writes[set_idx].pImageInfo       = NULL;
          writes[set_idx].pTexelBufferView = NULL;
        }break;
        case R_Vulkan_DescriptorSetKind_SBO_Geo3D_TileLights:
        {
          VkDescriptorBufferInfo *buffer_info = push_array(scratch.arena, VkDescriptorBufferInfo, 1);
          buffer_info->buffer = buffers[i];
          buffer_info->offset = 0;
          buffer_info->range  = R_VULKAN_MAX_TILES_PER_PASS*sizeof(R_Vulkan_SBO_Geo3D_TileLights);

          writes[set_idx].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          writes[set_idx].dstSet           = sets[i].h;
          writes[set_idx].dstBinding       = j;
          writes[set_idx].dstArrayElement  = 0; // start updating from the first element
          writes[set_idx].descriptorCount  = set_layout.bindings[j].descriptorCount;
          writes[set_idx].descriptorType   = set_layout.bindings[j].descriptorType;
          writes[set_idx].pBufferInfo      = buffer_info;
          writes[set_idx].pImageInfo       = NULL;
          writes[set_idx].pTexelBufferView = NULL;
        }break;
        default: {InvalidPath;}break;
      }
    }
  }

  // The updates are applied using vkUpdateDescriptorSets
  // It accepts two kinds of arrays as parameters: an array of VkWriteDescriptorSet and an array of VkCopyDescriptorSet
  // The latter can be used to copy descriptors to each other, as its name implies
  vkUpdateDescriptorSets(r_vulkan_state->logical_device.h, writes_count, writes, 0, NULL);
  scratch_end(scratch);
  ProfEnd();
}

internal void
r_vulkan_descriptor_set_destroy(R_Vulkan_DescriptorSet *set)
{
  VK_Assert(vkFreeDescriptorSets(r_vulkan_state->logical_device.h, set->pool->h, 1, &set->h));
  set->pool->cmt -= 1;
  if((set->pool->cmt+1) == set->pool->cap)
  {
    SLLStackPush(r_vulkan_state->first_avail_ds_pool[set->pool->kind], set->pool);
  }
}

//- sync primitives

internal VkFence
r_vulkan_fence()
{
  VkDevice device = r_vulkan_ldevice()->h;
  VkFence ret;
  VkFenceCreateInfo create_info =
  {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    // Create the fence in the signaled state, so that the first call to vkWaitForFences() returns immediately
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  VK_Assert(vkCreateFence(device, &create_info, NULL, &ret));
  return ret;
}

internal VkSemaphore
r_vulkan_semaphore(VkDevice device)
{
  // In current version of the VK API it doesn't actually have any required fields besides sType
  VkSemaphoreCreateInfo create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkSemaphore sem;
  VK_Assert(vkCreateSemaphore(device, &create_info, NULL, &sem));
  return sem;
}

//- sync helpers

/*
 * Using image memory barrier to transition image layout
 * One of the most common ways to perform layout transitions is using an image memory barrier
 * A pipeline barrier like that is generally used to synchronize access to resources, like ensuring that a write to a buffer completes before reading from it
 * But it can also be used to transition image layouts and transfer queue family ownership when VK_SHARING_MODE_EXCLUSIVE is used
 * There is an equivalent buffer memory barrier to do this for buffers
 */
internal void
r_vulkan_image_transition(VkCommandBuffer cmd_buf, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkPipelineStageFlags src_stage, VkAccessFlags src_access_flag, VkPipelineStageFlags dst_stage, VkAccessFlags dst_access_flag, VkImageAspectFlags aspect_mask)
{
  ProfBeginFunction();
  VkImageMemoryBarrier barrier =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    // It is possible to use VK_image_layout_undefined as oldLayout if you don't care about the existing contents of the image
    .oldLayout = old_layout,
    .newLayout = new_layout,
    // If you are using the barrier to transfer queue family ownership, then these two fields should be the indices of the queue families
    // They must be set to VK_QUEUE_FAMILY_IGNORED if you don't want to do this (not the default value)
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    // The image and subresourceRange specify the image that is affected and the specific part of the image
    // Our image is not an array and does not have mipmapping levels, so only one level and layer are specified 
    .image                           = image,
    .subresourceRange.aspectMask     = aspect_mask,
    .subresourceRange.baseMipLevel   = 0,
    .subresourceRange.levelCount     = 1,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount     = 1,
    // Barriers are primaryly used for synchronization purpose, so you must specify which types of operations that involve the resource must 
    // happen before the barrier, and which operations that involve the resource must wait on the barrier
    // We need to do that despite already using vkQueueWaitIdle to manually synchronize
    // The right values depend on the old and new layout
    .srcAccessMask = src_access_flag,
    .dstAccessMask = dst_access_flag,
  };

  // Transfer writes must occur in the pipeline transfer stage
  // Since the writes don't have to wait on anything, you may specify an empty access mask and the earliest possible pipeline stage
  //     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT for the pre-barrier operations
  // It shoule be noted that VK_PIPELINE_STAGE_TRANSFER_BIT is not real stage within the graphics and compute pipelines
  // It's more of a pseudo-stage where transfer happen
  // ref: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#VkPipelineStageFlagBits
  // One thing to note is that command buffer submission results in implicit VK_ACCESS_HOST_WRITE_BIT synchronization at the beginning
  // Since the transition_image_layout function executes a command, you could use this implicit synchronization and set srcAccessMask to 0 
  // If you ever needed a VK_ACCESS_HOST_WRITE_BIT dependency in a layout transition, you could explicity specify it in srcAccessMask

  // - `VkCommandBuffer commandBuffer (aka struct VkCommandBuffer_T *)`
  // - `VkPipelineStageFlags srcStageMask (aka unsigned int)`
  // - `VkPipelineStageFlags dstStageMask (aka unsigned int)`
  // - `VkDependencyFlags dependencyFlags (aka unsigned int)`
  // - `uint32_t memoryBarrierCount (aka unsigned int)`
  // - `const VkMemoryBarrier * pMemoryBarriers (aka const struct VkMemoryBarrier *)`
  // - `uint32_t bufferMemoryBarrierCount (aka unsigned int)`
  // - `const VkBufferMemoryBarrier * pBufferMemoryBarriers (aka const struct VkBufferMemoryBarrier *)`
  // - `uint32_t imageMemoryBarrierCount (aka unsigned int)`
  // - `const VkImageMemoryBarrier * pImageMemoryBarriers (aka const struct VkImageMemoryBarrier *)`
  // ref: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#synchronization-access-types-supported
  // The third parameter is either 0 or VK_DEPENDENCY_BY_REGION_BIT
  // The latter turns the barrier into a per-region condition
  // That means that the implementation is allowed to already begin reading from the parts of a resource that were written so far, for example
  vkCmdPipelineBarrier(cmd_buf, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
  ProfEnd();
}

//- pipeline

internal R_Vulkan_Pipeline
r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind kind, R_GeoTopologyKind topology, R_GeoPolygonKind polygon, VkFormat swapchain_format, R_Vulkan_Pipeline *old)
{
  R_Vulkan_Pipeline pipeline;
  pipeline.kind = kind;
  Temp scratch = scratch_begin(0,0);

  // We will need to recreate the pipeline if the format of swapchain image changed thus changed renderpass
  VkShaderModule vshad_mo, fshad_mo;
  switch(kind)
  {
    case(R_Vulkan_PipelineKind_GFX_Rect):
    {
      vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Rect];
      fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Rect];
    }break;
    case(R_Vulkan_PipelineKind_GFX_Noise):
    {
      vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Noise];
      fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Noise];
    }break;
    case(R_Vulkan_PipelineKind_GFX_Edge):
    {
      vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Edge];
      fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Edge];
    }break;
    case(R_Vulkan_PipelineKind_GFX_Crt):
    {
      vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Crt];
      fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Crt];
    }break;
    case(R_Vulkan_PipelineKind_GFX_Geo2D_Forward):
    {
      vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Geo2D_Forward];
      fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Geo2D_Forward];
    }break;
    case(R_Vulkan_PipelineKind_GFX_Geo2D_Composite):
    {
      vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Geo2D_Composite];
      fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Geo2D_Composite];
    }break;
    case(R_Vulkan_PipelineKind_GFX_Geo3D_ZPre):
    {
      vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Geo3D_ZPre];
      fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Geo3D_ZPre];
    }break;
    case(R_Vulkan_PipelineKind_GFX_Geo3D_Debug):
    {
      vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Geo3D_Debug];
      fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Geo3D_Debug];
    }break;
    case(R_Vulkan_PipelineKind_GFX_Geo3D_Forward):
    {
      vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Geo3D_Forward];
      fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Geo3D_Forward];
    }break;
    case(R_Vulkan_PipelineKind_GFX_Geo3D_Composite):
    {
      vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Geo3D_Composite];
      fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Geo3D_Composite];
    }break;
    case(R_Vulkan_PipelineKind_GFX_Finalize):
    {
      vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Finalize];
      fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Finalize];
    }break;
    default: {InvalidPath;}break;
  }

  // shader stage
  // To actually use the shaders we'll need to assign them to a specific pipeline stage through VkPipelineShaderStageCreateInfo structures as part of the actual pipeline creation process
  VkPipelineShaderStageCreateInfo vshad_stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
  vshad_stage.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vshad_stage.module = vshad_mo;
  // The function to invoke, known as the entrypoint.
  // That means that it's possbile to combine multiple fragment shaders into a single shader module and use different entry points to differentiate between their behaviors
  vshad_stage.pName  = "main";
  // Optional
  // There is one more (optional) member, .pSpecializationInfo, which we won't be using here, but is worth discussing.
  // It allows you to specify values for shader constants.
  // You can use a single shader module where its behavior can be configured at pipeline creation by specifying different values 
  // for the constants used in it.
  // This is more efficient than configuring the shader using variables at render time, because the compiler can do optimizations like eliminating if statements that 
  // depend on these values. If you don't have any constants like that, then you can set the member to NULL, which our struct initialization does automatically

  VkPipelineShaderStageCreateInfo fshad_stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
  fshad_stage.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fshad_stage.module = fshad_mo;
  fshad_stage.pName  = "main";
  VkPipelineShaderStageCreateInfo shad_stages[2] = { vshad_stage, fshad_stage };

  // Programmable stages (vertex and fragment)
  /////////////////////////////////////////////////////////////////////////////////
  // Fixed function stages
  // The older graphics APIs provided default state for most of the stages of the graphcis pipeline
  // In Vulkan you have to be explicit about most pipeline states as it'll be baked into an immutable pipeline state object

  // Dynamic state
  // While most of the pipeline state needs to be backed into the pipeline state, a limited amount of the state can actually be changed without
  // recreating the pipeline at draw time
  // Examples are *the size of the viewport*, *line width* and *blend constants*
  // If you want to use dynamic state and keep these properties out, then you'll have to fill in a structure called VkPipelineDynamicStateCreateInfo
  // Viewports
  // A viewport basically describes the region of the framebuffer that the output will be rendered to
  // This will almost always be (0, 0) to (width, height)
  // Remember that the size of the swap chain and its images may differ from the WIDTH and HEIGHT of the window
  // Since screen coordinates are not always equal in pixel sized image
  // The minDepth and maxDepth values specify the range of depth values to use for the framebuffer
  // These values must be within the [0.0f, 1.0f] range, but minDepth may be higher than maxDepth
  // If you aren't doing anything special, then you should stick to standard values of 0.0f and 1.0f
  // viewport = (VkViewport){
  //        .x = 0.0f,
  //        .y = 0.0f,
  //        .width = (float) swapchain.extent.width,
  //        .height = (float) swapchain.extent.height,
  //        .minDepth = 0.0f,
  //        .maxDepth = 1.0f,
  // };
  // Scissor rectangle
  // While viewports define the transformation from the image to the framebuffer, scissor rectangles define in which
  // regions pixels will actually be stored. Any pixels outside the scissor rectangles will be discarded by the rasterizer
  // They function like a filter rather than a transformation
  // scissor = (VkRect2D){
  //         .offset = {0, 0},
  //         .extent = swapchain.extent,
  // };
  // Viewport and scissor rectangle can either be specified as a static part of the pipeline or as a dynamic state set in the command buffer
  // While the former is more in line with the other states it's often convenient to make viewport and scissor state dynamic as it gives you a lot
  //      more flexibility
  // This is very common and all implementations can handle this dynamic state without performance penalty

  // When opting for dynamic viewport(s) and scissor rectangle(s) you need to enable the respective dynamic states for the pipeline
  VkDynamicState dynamic_states[3] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
    VK_DYNAMIC_STATE_LINE_WIDTH
  };
  VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = 3,
    .pDynamicStates    = dynamic_states,
  };
  // And then you only need to specify their count at pipeline creation time
  // The actual viewport(s) and scissor rectangle(s) will then later be set up at drawing time
  // With dynamic state it's even possbile to specify different viewports and or scissor rectangles within a single command buffer
  // TODO(k): Maybe we could use that to draw some minimap
  // Independent of how you set them, it's possible to use multiple viewports and scissor rectangles on some graphics cards, so the structure members reference an 
  // array of them. Using multiple requries enabling a GPU feature (see logical device creation)
  // -> Viewport state
  VkPipelineViewportStateCreateInfo viewport_state_create_info = {
    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount  = 1,
  };
  // Without dynamic state, the viewport and scissor rectangle need to be set in the pipeline using the VkPipelineViewportStateCreateInfo struct
  // This makes the viewport and scissor rectangle for this pipeline immutable. Any changes required to these values would require a new pipeline to be created with the new values
  // VkPipelineViewportStateCreateInfo viewportState{};
  // viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  // viewportState.viewportCount = 1;
  // viewportState.pViewports = &viewport;
  // viewportState.scissorCount = 1;
  // viewportState.pScissors = &scissor;

  // Vertex input stage
  // The VkPipelineVertexInputStateCreateInfo structure describes the format of the vertex data that will be passed to the vertex shader. It describes this in roughly two ways
  // 1. Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)
  // 2. Attrubite descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
  // Tell Vulkan how to pass vertices data format to the vertex shader once it's been uploaded into GPU memory
  // There are two types of structures needed to convey this information
  // *VkVertexInputBindingDescription* and *VkVertexInputAttributeDescription*

  // A vertex binding describes at which rate to load data from memory throughout the vertices
  // It specifies the number of bytes between data entries and whether to move to the next data entry after each vertex or after each instance
  VkPipelineVertexInputStateCreateInfo vtx_input_state_create_info = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

  VkVertexInputBindingDescription vtx_binding_desc[2] = {0};
#define MAX_VERTEX_ATTRIBUTE_DESCRIPTION_COUNT 22
  VkVertexInputAttributeDescription vtx_attr_descs[MAX_VERTEX_ATTRIBUTE_DESCRIPTION_COUNT];

  U64 vtx_binding_desc_count = 0;
  U64 vtx_attr_desc_cnt      = 0;
  switch(kind)
  {
    case R_Vulkan_PipelineKind_GFX_Geo3D_Debug:
    {
      vtx_binding_desc_count = 0;
      vtx_attr_desc_cnt = 0;
    }break;
    case R_Vulkan_PipelineKind_GFX_Geo2D_Forward:
    {
      vtx_binding_desc_count = 2;
      vtx_attr_desc_cnt = 20;

      vtx_binding_desc[0].binding   = 0;
      vtx_binding_desc[0].stride    = sizeof(R_Geo3D_Vertex);
      vtx_binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

      vtx_attr_descs[0].binding = 0;
      vtx_attr_descs[0].location = 0;
      vtx_attr_descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
      vtx_attr_descs[0].offset = offsetof(R_Geo3D_Vertex, pos);

      vtx_attr_descs[1].binding  = 0;
      vtx_attr_descs[1].location = 1;
      vtx_attr_descs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
      vtx_attr_descs[1].offset   = offsetof(R_Geo3D_Vertex, nor);

      vtx_attr_descs[2].binding  = 0;
      vtx_attr_descs[2].location = 2;
      vtx_attr_descs[2].format   = VK_FORMAT_R32G32_SFLOAT;
      vtx_attr_descs[2].offset   = offsetof(R_Geo3D_Vertex, tex);

      vtx_attr_descs[3].binding  = 0;
      vtx_attr_descs[3].location = 3;
      vtx_attr_descs[3].format   = VK_FORMAT_R32G32B32_SFLOAT;
      vtx_attr_descs[3].offset   = offsetof(R_Geo3D_Vertex, tan);

      vtx_attr_descs[4].binding  = 0;
      vtx_attr_descs[4].location = 4;
      vtx_attr_descs[4].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[4].offset   = offsetof(R_Geo3D_Vertex, col);

      vtx_attr_descs[5].binding  = 0;
      vtx_attr_descs[5].location = 5;
      vtx_attr_descs[5].format   = VK_FORMAT_R32G32B32A32_UINT;
      vtx_attr_descs[5].offset   = offsetof(R_Geo3D_Vertex, joints);

      vtx_attr_descs[6].binding  = 0;
      vtx_attr_descs[6].location = 6;
      vtx_attr_descs[6].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[6].offset   = offsetof(R_Geo3D_Vertex, weights);

      // Instance binding xform (vec4 x4)
      vtx_binding_desc[1].binding   = 1;
      vtx_binding_desc[1].stride    = sizeof(R_Mesh2DInst);
      vtx_binding_desc[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

      // xform
      vtx_attr_descs[7].binding  = 1;
      vtx_attr_descs[7].location = 7;
      vtx_attr_descs[7].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[7].offset   = offsetof(R_Mesh2DInst, xform) + sizeof(Vec4F32) * 0;

      vtx_attr_descs[8].binding  = 1;
      vtx_attr_descs[8].location = 8;
      vtx_attr_descs[8].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[8].offset   = offsetof(R_Mesh2DInst, xform) + sizeof(Vec4F32) * 1;

      vtx_attr_descs[9].binding  = 1;
      vtx_attr_descs[9].location = 9;
      vtx_attr_descs[9].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[9].offset   = offsetof(R_Mesh2DInst, xform) + sizeof(Vec4F32) * 2;

      vtx_attr_descs[10].binding  = 1;
      vtx_attr_descs[10].location = 10;
      vtx_attr_descs[10].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[10].offset   = offsetof(R_Mesh2DInst, xform) + sizeof(Vec4F32) * 3;

      // xform_inv
      vtx_attr_descs[11].binding  = 1;
      vtx_attr_descs[11].location = 11;
      vtx_attr_descs[11].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[11].offset   = offsetof(R_Mesh2DInst, xform_inv) + sizeof(Vec4F32) * 0;

      vtx_attr_descs[12].binding  = 1;
      vtx_attr_descs[12].location = 12;
      vtx_attr_descs[12].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[12].offset   = offsetof(R_Mesh2DInst, xform_inv) + sizeof(Vec4F32) * 1;

      vtx_attr_descs[13].binding  = 1;
      vtx_attr_descs[13].location = 13;
      vtx_attr_descs[13].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[13].offset   = offsetof(R_Mesh2DInst, xform_inv) + sizeof(Vec4F32) * 2;

      vtx_attr_descs[14].binding  = 1;
      vtx_attr_descs[14].location = 14;
      vtx_attr_descs[14].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[14].offset   = offsetof(R_Mesh2DInst, xform_inv) + sizeof(Vec4F32) * 3;

      // key
      vtx_attr_descs[15].binding  = 1;
      vtx_attr_descs[15].location = 15;
      vtx_attr_descs[15].format   = VK_FORMAT_R32G32_SFLOAT;
      vtx_attr_descs[15].offset   = offsetof(R_Mesh2DInst, key);

      // has_texture
      vtx_attr_descs[16].binding  = 1;
      vtx_attr_descs[16].location = 16;
      vtx_attr_descs[16].format   = VK_FORMAT_R32_UINT;
      vtx_attr_descs[16].offset   = offsetof(R_Mesh2DInst, has_texture);

      // color
      vtx_attr_descs[17].binding  = 1;
      vtx_attr_descs[17].location = 17;
      vtx_attr_descs[17].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[17].offset   = offsetof(R_Mesh2DInst, color);

      // has color
      vtx_attr_descs[18].binding  = 1;
      vtx_attr_descs[18].location = 18;
      vtx_attr_descs[18].format   = VK_FORMAT_R32_UINT;
      vtx_attr_descs[18].offset   = offsetof(R_Mesh2DInst, has_color);

      // draw edge
      vtx_attr_descs[19].binding  = 1;
      vtx_attr_descs[19].location = 19;
      vtx_attr_descs[19].format   = VK_FORMAT_R32_UINT;
      vtx_attr_descs[19].offset   = offsetof(R_Mesh2DInst, draw_edge);
    }break;
    case R_Vulkan_PipelineKind_GFX_Geo3D_ZPre:
    case R_Vulkan_PipelineKind_GFX_Geo3D_Forward:
    {
      vtx_binding_desc_count = 2;
      vtx_attr_desc_cnt = 22;

      // All of our per-vertex data is packed together in one array, so we'are only going to have one binding for now
      // This specifies the index of the binding in the array of bindings
      vtx_binding_desc[0].binding   = 0;
      // This specifies the number of bytes from one entry to the next
      vtx_binding_desc[0].stride    = sizeof(R_Geo3D_Vertex);
      // The inputRate parameter can have one of the following values
      // 1. VK_VERTEX_INPUT_RATE_VERTEX: move to the next data entry after each vertex
      // 2. VK_VERTEX_INPUT_RATE_INSTANCE: move to the next data entary after each instance
      vtx_binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

      // The VkVertexInputAttributeDescription describes how to handle vertex input
      // An attribute description struct describes how to extract a vertex attribtue from a chunk of vertex data originating from a binding description
      // We have two attribtues, position and color, so we need two attribute description structs
      // The binding parameter tells Vulkan from which binding the per-vertex data comes
      vtx_attr_descs[0].binding = 0;
      // The location parameter references the localtion directive of the input in the vertex shader
      // The input in the vertex shader with location 0 is the position, which has two 32-bit float components
      vtx_attr_descs[0].location = 0;
      // The format parameter describes the type of data for the attribtue
      // A bit confusingly, the formats are specified using the same enumeration as color formats
      // The following shader types and formats are commonly used together
      // 1. float: VK_FORMAT_R32_SFLOAT
      // 2.  vec2: VK_FORMAT_R32G32_SFLOAT
      // 3.  vec3: VK_FORMAT_R32G32B32_SFLOAT
      // 4.  vec4: VK_FORMAT_R32G32B32A32_SFLOAT
      // As you can see, you should use the format where the amount of color channels matches the number of components in the shader data type
      // It is allowed to use more channels than the number of components in the shader, but they will be silently discarded
      // If the number of channels is lower than the number of components, then the BGA components will use default values (0, 0, 1)
      // The color type (SFLOAT, UINT, SINT) and bit width should also match the type of the shader input. See the following examples:
      // 1.  ivec2: VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
      // 2.  uvec4: VK_FORMAT_R32G32B32A32_UINT, a 4-component vector of 32-bit unsigned integers
      // 3. double: VK_FORMAT_R64_SFLOAT, a double-precision(64-bit) float
      // Also the format parameter implicitly defines the byte size of attribtue data
      vtx_attr_descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
      // The offset parameter specifies the number of bytes since the start of the per-vertex data to read from
      vtx_attr_descs[0].offset = offsetof(R_Geo3D_Vertex, pos);

      vtx_attr_descs[1].binding  = 0;
      vtx_attr_descs[1].location = 1;
      vtx_attr_descs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
      vtx_attr_descs[1].offset   = offsetof(R_Geo3D_Vertex, nor);

      vtx_attr_descs[2].binding  = 0;
      vtx_attr_descs[2].location = 2;
      vtx_attr_descs[2].format   = VK_FORMAT_R32G32_SFLOAT;
      vtx_attr_descs[2].offset   = offsetof(R_Geo3D_Vertex, tex);

      vtx_attr_descs[3].binding  = 0;
      vtx_attr_descs[3].location = 3;
      vtx_attr_descs[3].format   = VK_FORMAT_R32G32B32_SFLOAT;
      vtx_attr_descs[3].offset   = offsetof(R_Geo3D_Vertex, tan);

      vtx_attr_descs[4].binding  = 0;
      vtx_attr_descs[4].location = 4;
      vtx_attr_descs[4].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[4].offset   = offsetof(R_Geo3D_Vertex, col);

      vtx_attr_descs[5].binding  = 0;
      vtx_attr_descs[5].location = 5;
      vtx_attr_descs[5].format   = VK_FORMAT_R32G32B32A32_UINT;
      vtx_attr_descs[5].offset   = offsetof(R_Geo3D_Vertex, joints);

      vtx_attr_descs[6].binding  = 0;
      vtx_attr_descs[6].location = 6;
      vtx_attr_descs[6].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[6].offset   = offsetof(R_Geo3D_Vertex, weights);

      // Instance binding xform (vec4 x4)
      vtx_binding_desc[1].binding   = 1;
      vtx_binding_desc[1].stride    = sizeof(R_Mesh3DInst);
      vtx_binding_desc[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

      // xform
      vtx_attr_descs[7].binding  = 1;
      vtx_attr_descs[7].location = 7;
      vtx_attr_descs[7].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[7].offset   = offsetof(R_Mesh3DInst, xform) + sizeof(Vec4F32) * 0;

      vtx_attr_descs[8].binding  = 1;
      vtx_attr_descs[8].location = 8;
      vtx_attr_descs[8].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[8].offset   = offsetof(R_Mesh3DInst, xform) + sizeof(Vec4F32) * 1;

      vtx_attr_descs[9].binding  = 1;
      vtx_attr_descs[9].location = 9;
      vtx_attr_descs[9].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[9].offset   = offsetof(R_Mesh3DInst, xform) + sizeof(Vec4F32) * 2;

      vtx_attr_descs[10].binding  = 1;
      vtx_attr_descs[10].location = 10;
      vtx_attr_descs[10].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[10].offset   = offsetof(R_Mesh3DInst, xform) + sizeof(Vec4F32) * 3;

      // xform_inv
      vtx_attr_descs[11].binding  = 1;
      vtx_attr_descs[11].location = 11;
      vtx_attr_descs[11].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[11].offset   = offsetof(R_Mesh3DInst, xform_inv) + sizeof(Vec4F32) * 0;

      vtx_attr_descs[12].binding  = 1;
      vtx_attr_descs[12].location = 12;
      vtx_attr_descs[12].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[12].offset   = offsetof(R_Mesh3DInst, xform_inv) + sizeof(Vec4F32) * 1;

      vtx_attr_descs[13].binding  = 1;
      vtx_attr_descs[13].location = 13;
      vtx_attr_descs[13].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[13].offset   = offsetof(R_Mesh3DInst, xform_inv) + sizeof(Vec4F32) * 2;

      vtx_attr_descs[14].binding  = 1;
      vtx_attr_descs[14].location = 14;
      vtx_attr_descs[14].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[14].offset   = offsetof(R_Mesh3DInst, xform_inv) + sizeof(Vec4F32) * 3;

      // key
      vtx_attr_descs[15].binding  = 1;
      vtx_attr_descs[15].location = 15;
      vtx_attr_descs[15].format   = VK_FORMAT_R32G32_SFLOAT;
      vtx_attr_descs[15].offset   = offsetof(R_Mesh3DInst, key);

      // material_idx
      vtx_attr_descs[16].binding  = 1;
      vtx_attr_descs[16].location = 16;
      vtx_attr_descs[16].format   = VK_FORMAT_R32_UINT;
      vtx_attr_descs[16].offset   = offsetof(R_Mesh3DInst, material_idx);

      // draw_edge
      vtx_attr_descs[17].binding  = 1;
      vtx_attr_descs[17].location = 17;
      vtx_attr_descs[17].format   = VK_FORMAT_R32_UINT;
      vtx_attr_descs[17].offset   = offsetof(R_Mesh3DInst, draw_edge);

      // joint_count
      vtx_attr_descs[18].binding  = 1;
      vtx_attr_descs[18].location = 18;
      vtx_attr_descs[18].format   = VK_FORMAT_R32_UINT;
      vtx_attr_descs[18].offset   = offsetof(R_Mesh3DInst, joint_count);

      // first joint
      vtx_attr_descs[19].binding  = 1;
      vtx_attr_descs[19].location = 19;
      vtx_attr_descs[19].format   = VK_FORMAT_R32_UINT;
      vtx_attr_descs[19].offset   = offsetof(R_Mesh3DInst, first_joint);

      // depth test
      vtx_attr_descs[20].binding  = 1;
      vtx_attr_descs[20].location = 20;
      vtx_attr_descs[20].format   = VK_FORMAT_R32_UINT;
      vtx_attr_descs[20].offset   = offsetof(R_Mesh3DInst, depth_test);

      // omit light
      vtx_attr_descs[21].binding  = 1;
      vtx_attr_descs[21].location = 21;
      vtx_attr_descs[21].format   = VK_FORMAT_R32_UINT;
      vtx_attr_descs[21].offset   = offsetof(R_Mesh3DInst, omit_light);
    }break;
    case(R_Vulkan_PipelineKind_GFX_Rect):
    {
      vtx_binding_desc_count = 1;
      vtx_attr_desc_cnt = 10;

      vtx_binding_desc[0].binding   = 0;
      vtx_binding_desc[0].stride    = sizeof(R_Rect2DInst);
      vtx_binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

      // dst
      vtx_attr_descs[0].binding  = 0;
      vtx_attr_descs[0].location = 0;
      vtx_attr_descs[0].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[0].offset   = offsetof(R_Rect2DInst, dst);

      // src
      vtx_attr_descs[1].binding  = 0;
      vtx_attr_descs[1].location = 1;
      vtx_attr_descs[1].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[1].offset   = offsetof(R_Rect2DInst, src);

      // color 1
      vtx_attr_descs[2].binding  = 0;
      vtx_attr_descs[2].location = 2;
      vtx_attr_descs[2].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[2].offset   = offsetof(R_Rect2DInst, colors[0]);

      // color 2
      vtx_attr_descs[3].binding  = 0;
      vtx_attr_descs[3].location = 3;
      vtx_attr_descs[3].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[3].offset   = offsetof(R_Rect2DInst, colors[1]);

      // color 3
      vtx_attr_descs[4].binding  = 0;
      vtx_attr_descs[4].location = 4;
      vtx_attr_descs[4].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[4].offset   = offsetof(R_Rect2DInst, colors[2]);

      // color 4
      vtx_attr_descs[5].binding  = 0;
      vtx_attr_descs[5].location = 5;
      vtx_attr_descs[5].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[5].offset   = offsetof(R_Rect2DInst, colors[3]);

      // corner_radii
      vtx_attr_descs[6].binding  = 0;
      vtx_attr_descs[6].location = 6;
      vtx_attr_descs[6].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[6].offset   = offsetof(R_Rect2DInst, corner_radii);

      // sty_1
      vtx_attr_descs[7].binding  = 0;
      vtx_attr_descs[7].location = 7;
      vtx_attr_descs[7].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[7].offset   = offsetof(R_Rect2DInst, border_thickness);

      // sty_2
      vtx_attr_descs[8].binding  = 0;
      vtx_attr_descs[8].location = 8;
      vtx_attr_descs[8].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[8].offset   = offsetof(R_Rect2DInst, omit_texture);

      // line
      vtx_attr_descs[9].binding  = 0;
      vtx_attr_descs[9].location = 9;
      vtx_attr_descs[9].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
      vtx_attr_descs[9].offset   = offsetof(R_Rect2DInst, line);
    }break;
    // case R_Vulkan_PipelineKind_GFX_Blur:
    case R_Vulkan_PipelineKind_GFX_Noise:
    case R_Vulkan_PipelineKind_GFX_Edge:
    case R_Vulkan_PipelineKind_GFX_Crt:
    case R_Vulkan_PipelineKind_GFX_Geo2D_Composite:
    case(R_Vulkan_PipelineKind_GFX_Geo3D_Composite):
    case(R_Vulkan_PipelineKind_GFX_Finalize):
    {
      vtx_binding_desc_count = 0;
      vtx_attr_desc_cnt = 0;
    }break;
    default: {InvalidPath;}break;

  }
  vtx_input_state_create_info.vertexBindingDescriptionCount   = vtx_binding_desc_count;
  vtx_input_state_create_info.pVertexBindingDescriptions      = vtx_binding_desc;
  vtx_input_state_create_info.vertexAttributeDescriptionCount = vtx_attr_desc_cnt;
  vtx_input_state_create_info.pVertexAttributeDescriptions    = vtx_attr_descs;

  // Input assembly stage
  // The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be drawn from the vertices and if primitive restart should be enabled
  // The former is specified in the topology member and can have values like:
  // VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
  // VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 vertices without reuse
  // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex for the next line
  // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 verties without reuse
  // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle are used as first two vertices of the next triangle
  // Normally, the vertices are loaded from the vertex buffer by index in sequential order, but with an element buffer you can specify the indices to use yourself.
  // This allows you to perform optimizations like reusing vertices
  // If you set the primitiveRestartEnable member to VK_TRUE, then it's possible to break up lines and triangles in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF
  // Now we intend to draw a simple triangle, so ...
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
  VkPrimitiveTopology vk_topology;

  switch(topology)
  {
    case R_GeoTopologyKind_Lines:         {vk_topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;}break;
    case R_GeoTopologyKind_LineStrip:     {vk_topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;}break;
    case R_GeoTopologyKind_Triangles:     {vk_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;}break;
    case R_GeoTopologyKind_TriangleStrip: {vk_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;}break;
    default:                              {InvalidPath;}break;
  }
  input_assembly_state_create_info.topology = vk_topology;
  input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

  // Resterizer stage
  // The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader
  // It also performs depth testing, face culling and the scissor test, and it can be configured to output fragments that fill entire polygons or just the edges (wireframe rendering)
  // All this is configured using the VkPipelineRasterizationStateCreateInfo structure

  VkPolygonMode vk_polygon;
  switch(polygon)
  {
    case R_GeoPolygonKind_Fill:  {vk_polygon = VK_POLYGON_MODE_FILL;}break;
    case R_GeoPolygonKind_Line:  {vk_polygon = VK_POLYGON_MODE_LINE;}break;
    case R_GeoPolygonKind_Point: {vk_polygon = VK_POLYGON_MODE_POINT;}break;
    default:                     {InvalidPath;}break;
  }

  // VkPipelineRasterizationStateRasterizationOrderAMD order = 
  // {
  //     .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD,
  //     .rasterizationOrder = VK_RASTERIZATION_ORDER_STRICT_AMD,
  // };
  VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    // If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them as opposed to discarding them
    // This is useful in some special cases like shadow maps. Using this requires enabling a GPU feature
    .depthClampEnable        = VK_FALSE,
    // If rasterizerDiscardEnable set to VK_TRUE, then geometry never passes through the rasterizer stage
    // This basically disables any output to the framebuffer
    .rasterizerDiscardEnable = VK_FALSE,
    // The polygonMode determines how fragments are generated for geometry. The following modes are available:
    // 1. VK_POLYGON_MODE_FILL:  fill the area of the polygon with fragments
    // 2. VK_POLYGON_MODE_LINE:  polygon edges are drawn as lines
    // 3. VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
    // Using any mode other than fill requires enabling a GPU feature
    .polygonMode             = vk_polygon,
    // The lineWidth member is strightforward, it describes the thickness of lines in terms of number of fragments
    // The maximum line width that is supported depends on the hardware and any line thicker than 1.0f requires you to enable wideLines GPU feature
    .lineWidth               = 1.0f,
    // The cullMode variable determines the type of face culling to use. You can disable culling, cull the front faces, cull the back faces or both
    .cullMode                = VK_CULL_MODE_BACK_BIT,
    // The frontFace variable specifies the vertex order for faces to be considered front-facing and can be clockwise or counterclockwise
    .frontFace               = VK_FRONT_FACE_CLOCKWISE,
    // TODO(k): not sure what are these settings for
    // The rasterizer can alter the depth values by adding a constant value or biasign them based on a fragment's slope
    // This si sometimes used for shadow mapping, but we won't be using it
    .depthBiasEnable         = VK_FALSE,
    .depthBiasConstantFactor = 0.0f, // Optional
    .depthBiasClamp          = 0.0f, // Optional
    .depthBiasSlopeFactor    = 0.0f, // Optional
  };

  switch(kind)
  {
    case R_Vulkan_PipelineKind_GFX_Geo2D_Forward:
    case R_Vulkan_PipelineKind_GFX_Geo3D_ZPre:
    case R_Vulkan_PipelineKind_GFX_Geo3D_Forward:
    {
      // rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_LINE;
      rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }break;
    case R_Vulkan_PipelineKind_GFX_Geo3D_Debug:
    case R_Vulkan_PipelineKind_GFX_Rect:
    // case R_Vulkan_PipelineKind_GFX_Blur:
    case R_Vulkan_PipelineKind_GFX_Noise:
    case R_Vulkan_PipelineKind_GFX_Edge:
    case R_Vulkan_PipelineKind_GFX_Crt:
    case R_Vulkan_PipelineKind_GFX_Geo2D_Composite:
    case R_Vulkan_PipelineKind_GFX_Geo3D_Composite:
    case R_Vulkan_PipelineKind_GFX_Finalize:
    {
      // noop
    }break;
    default: {InvalidPath;}break;
  }

  // Multisampling
  // The VkPipelineMultisamplesStateCreateInfo struct configures multisampling, which is one of the ways to perform anti-aliasing
  // It works by combining the fragment shader resutls of multiple polygons that rasterize to the same pixel
  // This mainly occurs along edges, which is also where the most noticable aliasing artifacts occur
  // Because it doesn't need to run the fragment shader multiple times if only one polygon maps to a pixel, it is significantly less expensive than simply rendering
  // to a higher resolution and then downscaling (aka SSA). Enabling it requires enabling a GPU feature
  // TODO(k): this doesn't make too much sense to me
  VkPipelineMultisampleStateCreateInfo multi_sampling_state_create_info = {
    .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable   = VK_FALSE,
    .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading      = 1.0f, // Optional
    .pSampleMask           = NULL, // Optional
    .alphaToCoverageEnable = VK_FALSE, // Optional
    .alphaToOneEnable      = VK_FALSE, // Optional
  };

  // Depth and stencil testing stage
  // If you are using a depth and/or strencil buffer, then you also need to configure the depth and stencil tests using 
  //      VkPipelineDepthStencilStateCreateInfo. 
  // We don't have one right now, so we can simply pass a NULL instead of a pointer to such a struct
  VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  switch(kind)
  {
    case R_Vulkan_PipelineKind_GFX_Geo3D_Debug:
    case R_Vulkan_PipelineKind_GFX_Geo3D_ZPre:
    case R_Vulkan_PipelineKind_GFX_Geo3D_Forward:
    {
      // The depthTestEnable field specifies if the depth of new fragments should be compared to the depth buffer to see if they should be discarded
      // The depthWriteEnable field specifies if the new depth of fragments that pass the depth test should actually be written to the depth buffer
      depth_stencil_state_create_info.depthTestEnable       = VK_TRUE;
      depth_stencil_state_create_info.depthWriteEnable      = VK_TRUE;
      // The depthCompareOp field specifies the comparison that is performed to keep or discard fragments
      // We're sticking to the convention of lower depth = closer, so the depth of new fragments should be less
      depth_stencil_state_create_info.depthCompareOp        = VK_COMPARE_OP_LESS;
      // These three fields are used for the optional depth bound test
      // Basically this allows you to only keep fragments that fall within the specified depth range
      depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
      depth_stencil_state_create_info.minDepthBounds        = 0.0f; // Optional
      depth_stencil_state_create_info.maxDepthBounds        = 1.0f; // Optional
                                                                    // The last three fields configure stencil buffer operations, which we also won't be using for now
                                                                    // If you want to use these operations, then you will have to make sure that the format of the depth/stencil image contains a stencil component
      depth_stencil_state_create_info.stencilTestEnable     = VK_FALSE;
      depth_stencil_state_create_info.front                 = (VkStencilOpState){0}; // Optional
      depth_stencil_state_create_info.back                  = (VkStencilOpState){0}; // Optional
    }break;
    case R_Vulkan_PipelineKind_GFX_Rect:
    // case R_Vulkan_PipelineKind_GFX_Blur:
    case R_Vulkan_PipelineKind_GFX_Noise:
    case R_Vulkan_PipelineKind_GFX_Edge:
    case R_Vulkan_PipelineKind_GFX_Crt:
    case R_Vulkan_PipelineKind_GFX_Geo2D_Forward:
    case R_Vulkan_PipelineKind_GFX_Geo2D_Composite:
    case R_Vulkan_PipelineKind_GFX_Geo3D_Composite:
    case R_Vulkan_PipelineKind_GFX_Finalize:
    {
      depth_stencil_state_create_info.depthTestEnable       = VK_FALSE;
      depth_stencil_state_create_info.depthWriteEnable      = VK_FALSE;
      depth_stencil_state_create_info.depthCompareOp        = VK_COMPARE_OP_LESS;
      depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
      depth_stencil_state_create_info.minDepthBounds        = 0.0f; // Optional
      depth_stencil_state_create_info.maxDepthBounds        = 1.0f; // Optional
      depth_stencil_state_create_info.stencilTestEnable     = VK_FALSE;
      depth_stencil_state_create_info.front                 = (VkStencilOpState){0}; // Optional
      depth_stencil_state_create_info.back                  = (VkStencilOpState){0}; // Optional
    }break;
    default: {InvalidPath;}break;
  }

  // Color blending stage
  // After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer
  // This transformation is known as color blending and there are two ways to do it
  // 1. Mix the old and new value to produce a final color
  // 2. Combine the old and new value using a bitwise operation
  // There are two types of structs to configure color blending
  // 1. VkPipelineColorBlendAttachmentState contains the configuration per attached framebuffer
  // 2. VkPipelineColorBlendStateCreateInfo contains the global color blending settings

#if 0
  // No Blend
  ///////////////////////////////////////////////////////////////////////////////////////

  VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
    .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable         = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
    .colorBlendOp        = VK_BLEND_OP_ADD, // Optional
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
    .alphaBlendOp        = VK_BLEND_OP_ADD, // Optional
  };
#elif 1
  // This per-framebuffer struct allows you to configure the first way of color blending. The operations that will be performed are best demonstrated using the following pseudocode:
  // if(blendEnable) {
  //     finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
  //     finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
  // } else {
  //     finalColor = newColor;
  // }
  // 
  // finalColor = finalColor & colorWriteMask;

  // The most common way to use color blending is to implement alpha blending, where we want the new color to blended with the old color 
  // based on its opacity. The finalColor should then computed as follows 
  // finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
  // finalColor.a = newAlpha;
  // This can be accomplished with following parameters
  // colorBlendAttachment.blendEnable = VK_TRUE;
  // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  // NOTE(k): mesh pass have two color attachment
  VkPipelineColorBlendAttachmentState color_blend_attachment_state[3] =
  {
    // main color attachment
    {
      .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable         = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, // Optional
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // Optional
      .colorBlendOp        = VK_BLEND_OP_ADD, // Optional
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
      .alphaBlendOp        = VK_BLEND_OP_ADD, // Optional
    },
    {
      .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable         = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp        = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp        = VK_BLEND_OP_ADD,
    },
    {
      .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable         = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp        = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp        = VK_BLEND_OP_ADD,
    },
  };
  switch(kind)
  {
    default:{InvalidPath;}break;
    case R_Vulkan_PipelineKind_GFX_Rect:
    {
      color_blend_attachment_state[1].blendEnable = VK_TRUE;
    }break;
    case R_Vulkan_PipelineKind_GFX_Geo2D_Forward:
    case R_Vulkan_PipelineKind_GFX_Geo3D_Debug:
    case R_Vulkan_PipelineKind_GFX_Geo3D_Forward:
    { 
      // noop
    }break;
    case R_Vulkan_PipelineKind_GFX_Geo2D_Composite:
    {
      color_blend_attachment_state[0].blendEnable = VK_FALSE;
    }break;
    case R_Vulkan_PipelineKind_GFX_Geo3D_Composite:
    {
      // TODO(k): what is this, why we ever want to do this, it don't make sense, but I am too afraid to delete it
      // color_blend_attachment_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
      // color_blend_attachment_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
      // color_blend_attachment_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      // color_blend_attachment_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    }break;
    // case R_Vulkan_PipelineKind_GFX_Blur:
    case R_Vulkan_PipelineKind_GFX_Noise:
    case R_Vulkan_PipelineKind_GFX_Edge:
    case R_Vulkan_PipelineKind_GFX_Crt:
    case R_Vulkan_PipelineKind_GFX_Geo3D_ZPre: 
    case R_Vulkan_PipelineKind_GFX_Finalize: 
    {
      color_blend_attachment_state[0].blendEnable = VK_FALSE;
    }break;
  }
#endif

  // The second structure references the array of structures for all of the framebuffers and allows you to set blend constants that you can use
  // as blend factors in the aforementioned calculations
  // If you want to use the second method of blending (bitwise combination), then you should set logicOpEnable to VK_TRUE
  // The bitwise operation can then specified in the logicOp field
  // Note that this will automatically disable the first method, as if you had set blendEnable to VK_FALSE for every attached framebuffer!!!
  // The colorWriteMask will also be used in this mode to determine which channels in the framebuffer will actually be affected
  // It is also possbile to disable both modes, as we've done here, in which case the fragment colors will be written to the framebuffer unmodified
  VkPipelineColorBlendStateCreateInfo color_blend_state_create_info =
  {
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable     = VK_FALSE,
    .logicOp           = VK_LOGIC_OP_COPY, // Optional
    .attachmentCount   = 1,
    .pAttachments      = color_blend_attachment_state, 
    .blendConstants[0] = 0.0f, // Optionsal
    .blendConstants[1] = 0.0f, // Optionsal
    .blendConstants[2] = 0.0f, // Optionsal
    .blendConstants[3] = 0.0f, // Optionsal
  };
  switch(kind)
  {
    default:{InvalidPath;}break;
    case R_Vulkan_PipelineKind_GFX_Geo2D_Forward:
    {
      color_blend_state_create_info.attachmentCount = 3;
    }break;
    case R_Vulkan_PipelineKind_GFX_Rect:
    {
      color_blend_state_create_info.attachmentCount = 2;
    }break;
    // case R_Vulkan_PipelineKind_GFX_Blur:
    case R_Vulkan_PipelineKind_GFX_Noise:
    case R_Vulkan_PipelineKind_GFX_Edge:
    case R_Vulkan_PipelineKind_GFX_Crt:
    case R_Vulkan_PipelineKind_GFX_Geo2D_Composite:
    case R_Vulkan_PipelineKind_GFX_Geo3D_Composite:
    case R_Vulkan_PipelineKind_GFX_Finalize:
    {
      color_blend_state_create_info.attachmentCount = 1;
    }break;
    case R_Vulkan_PipelineKind_GFX_Geo3D_ZPre:
    {
      color_blend_state_create_info.attachmentCount = 1;
    }break;
    case R_Vulkan_PipelineKind_GFX_Geo3D_Debug:
    case R_Vulkan_PipelineKind_GFX_Geo3D_Forward:
    {
      color_blend_state_create_info.attachmentCount = 3;
    }break;
  }

  // Pipeline layout
  // You can use *uniform* values in shaders, which are globals similar to dynamic state variables that can be changed at drawing time to alfter the 
  // behavior of your shaders without having to recreate them
  // They are commonly used to pass the transformation matrix to the vertex shader, or to create texture samplers in the fragment shader
  // These uniform values need to be specified during pipeline creation by creating a VkPipelineLayout object
  // Even though we won't be using them until a future chapter, we are still required to create an empty pipeline layout
  {
    VkPipelineLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

    U64 set_layout_count = 0;
#define __MAX_PIPELINE_SET_LAYOUTS 9
    VkDescriptorSetLayout set_layouts[__MAX_PIPELINE_SET_LAYOUTS];

#define __MAX_PIPELINE_PUSH_CONSTANT_RANGES 1
    // push constants
    U64 push_constant_range_count = 0;
    VkPushConstantRange push_constant_ranges[__MAX_PIPELINE_PUSH_CONSTANT_RANGES];

    switch(kind)
    {
      default: {InvalidPath;}break;
      case R_Vulkan_PipelineKind_GFX_Rect:
      {
        set_layout_count = 2;
        set_layouts[0] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Rect].h;
        set_layouts[1] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;
      }break;
      case R_Vulkan_PipelineKind_GFX_Geo2D_Forward:
      {
        set_layout_count = 2;
        set_layouts[0] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Geo2D].h;
        set_layouts[1] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;
      }break;
      case R_Vulkan_PipelineKind_GFX_Geo3D_ZPre:
      {
        set_layout_count = 2;
        set_layouts[0] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Geo3D].h;
        set_layouts[1] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_Joints].h;
      }break;
      case R_Vulkan_PipelineKind_GFX_Geo3D_Debug:
      {
        set_layout_count = 1;
        set_layouts[0] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Geo3D].h;
      }break;
      case R_Vulkan_PipelineKind_GFX_Geo3D_Forward:
      {
        set_layout_count = 7;
        set_layouts[0] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Geo3D].h;
        set_layouts[1] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_Joints].h;
        set_layouts[2] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;
        set_layouts[3] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_Lights].h;
        set_layouts[4] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_LightIndices].h;
        set_layouts[5] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_TileLights].h;
        set_layouts[6] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_Materials].h;

        push_constant_range_count = 1;
        push_constant_ranges[0] =
          (VkPushConstantRange){
            .offset = 0,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .size = sizeof(R_Vulkan_PUSH_Geo3D_Forward),
          };
      }break;
      // NOTE(k): normal_depth + geo3d_image
      case R_Vulkan_PipelineKind_GFX_Geo3D_Composite:
      {
        set_layout_count = 2;
        set_layouts[0] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;
        set_layouts[1] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;
      }break;
      case R_Vulkan_PipelineKind_GFX_Geo2D_Composite:
      case R_Vulkan_PipelineKind_GFX_Noise:
      {
        set_layout_count = 1;
        set_layouts[0] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;

        push_constant_range_count = 1;
        push_constant_ranges[0] = (VkPushConstantRange){
          .offset = 0,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .size = sizeof(R_Vulkan_PUSH_Noise),
        };
      }break;
      case R_Vulkan_PipelineKind_GFX_Edge:
      {
        set_layout_count = 2;
        set_layouts[0] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;
        set_layouts[1] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;

        push_constant_range_count = 1;
        push_constant_ranges[0] = (VkPushConstantRange){
          .offset = 0,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .size = sizeof(R_Vulkan_PUSH_Edge),
        };
      }break;
      case R_Vulkan_PipelineKind_GFX_Crt:
      {
        set_layout_count = 1;
        set_layouts[0] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;

        push_constant_range_count = 1;
        push_constant_ranges[0] = (VkPushConstantRange){
          .offset = 0,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .size = sizeof(R_Vulkan_PUSH_Crt),
        };
      }break;
      case R_Vulkan_PipelineKind_GFX_Finalize:
      {
        set_layout_count = 1;
        set_layouts[0] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;
      }break;
    }

    create_info.setLayoutCount         = set_layout_count; // Optional
    create_info.pSetLayouts            = set_layouts;      // Optional
    // The structure also specifies push constants, which are another way of passing dynamic values to shaders that we may get into in future
    create_info.pushConstantRangeCount = push_constant_range_count; // Optional
    create_info.pPushConstantRanges    = push_constant_ranges; // Optional

    // The pipeline layout will be referenced throughout the program's lifetime, so we should destroy it at the end
    VK_Assert(vkCreatePipelineLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &pipeline.layout));
  }

  // Rendering info
  VkPipelineRenderingCreateInfo rendering_info = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
  {
    U32 color_attachment_count = 0;
    VkFormat *color_attachment_formats = 0;
    VkFormat depth_attachment_format = {0};
    VkFormat stencil_attachment_format = {0};

    switch(kind)
    {
      case R_Vulkan_PipelineKind_GFX_Rect:
      {
        color_attachment_count = 2;
        color_attachment_formats = push_array(scratch.arena, VkFormat, color_attachment_count);
        color_attachment_formats[0] = swapchain_format;
        color_attachment_formats[1] = VK_FORMAT_R32G32B32A32_SFLOAT;
      }break;
      case R_Vulkan_PipelineKind_GFX_Noise:
      {
        color_attachment_count = 1;
        color_attachment_formats = push_array(scratch.arena, VkFormat, color_attachment_count);
        color_attachment_formats[0] = swapchain_format;
      }break;
      case R_Vulkan_PipelineKind_GFX_Edge:
      {
        color_attachment_count = 1;
        color_attachment_formats = push_array(scratch.arena, VkFormat, color_attachment_count);
        color_attachment_formats[0] = swapchain_format;
      }break;
      case R_Vulkan_PipelineKind_GFX_Crt:
      {
        color_attachment_count = 1;
        color_attachment_formats = push_array(scratch.arena, VkFormat, color_attachment_count);
        color_attachment_formats[0] = swapchain_format;
      }break;
      case R_Vulkan_PipelineKind_GFX_Geo2D_Forward:
      {
        color_attachment_count = 3;
        color_attachment_formats = push_array(scratch.arena, VkFormat, color_attachment_count);
        color_attachment_formats[0] = swapchain_format;
        color_attachment_formats[1] = VK_FORMAT_R32G32B32A32_SFLOAT;
        color_attachment_formats[2] = VK_FORMAT_R32G32B32A32_SFLOAT;
      }break;
      case R_Vulkan_PipelineKind_GFX_Geo2D_Composite:
      {
        color_attachment_count = 1;
        color_attachment_formats = push_array(scratch.arena, VkFormat, color_attachment_count);
        color_attachment_formats[0] = swapchain_format;
      }break;
      case R_Vulkan_PipelineKind_GFX_Geo3D_ZPre:
      {
        color_attachment_count = 0;
        depth_attachment_format = r_vulkan_pdevice()->depth_image_format;
      }break;
      case R_Vulkan_PipelineKind_GFX_Geo3D_Debug:
      case R_Vulkan_PipelineKind_GFX_Geo3D_Forward:
      {
        color_attachment_count = 3;
        color_attachment_formats = push_array(scratch.arena, VkFormat, color_attachment_count);
        color_attachment_formats[0] = swapchain_format;
        color_attachment_formats[1] = VK_FORMAT_R32G32B32A32_SFLOAT;
        color_attachment_formats[2] = VK_FORMAT_R32G32B32A32_SFLOAT;
        
        depth_attachment_format = r_vulkan_pdevice()->depth_image_format;
      }break;
      case R_Vulkan_PipelineKind_GFX_Geo3D_Composite:
      {
        color_attachment_count = 1;
        color_attachment_formats = push_array(scratch.arena, VkFormat, color_attachment_count);
        color_attachment_formats[0] = swapchain_format;
      }break;
      case R_Vulkan_PipelineKind_GFX_Finalize:
      {
        color_attachment_count = 1;
        color_attachment_formats = push_array(scratch.arena, VkFormat, color_attachment_count);
        color_attachment_formats[0] = swapchain_format;
      }break;
      default:{InvalidPath;}break;
    }

    rendering_info.colorAttachmentCount    = color_attachment_count;
    rendering_info.pColorAttachmentFormats = color_attachment_formats;
    rendering_info.depthAttachmentFormat   = depth_attachment_format;
    rendering_info.stencilAttachmentFormat = stencil_attachment_format;
  }

  // Pipeline
  {
    VkGraphicsPipelineCreateInfo create_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    create_info.pNext               = &rendering_info;
    create_info.stageCount          = 2; // vertex and fragment shader
    create_info.pStages             = shad_stages;
    create_info.pVertexInputState   = &vtx_input_state_create_info;
    create_info.pInputAssemblyState = &input_assembly_state_create_info;
    create_info.pViewportState      = &viewport_state_create_info;
    create_info.pRasterizationState = &rasterization_state_create_info;
    create_info.pMultisampleState   = &multi_sampling_state_create_info;
    create_info.pDepthStencilState  = &depth_stencil_state_create_info;
    create_info.pColorBlendState    = &color_blend_state_create_info;
    create_info.pDynamicState       = &dynamic_state_create_info;
    create_info.layout              = pipeline.layout;
    // NOTE(k): we are using dynamic rendering now, no longer need a render pass
    // And finally we have the reference to the render pass and the index of the sub pass where the graphics pipeline will be used
    // It is also possible to use other render passes with this pipeline instead of this specific instance, but they have to be compatible with renderPass 
    create_info.renderPass          = 0;
    create_info.subpass             = 0;
    // Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline
    // The idea of pipeline derivatives is that it is less expensive to set up pipelines when they have much functionality in common with an existing pipeline and 
    // switching between pipelines from the same parent can also be done quicker
    // You can either specify the handle of an existing pipeline with basePipelineHandle or reference another pipeline that is about to be created by index with basePipelineIndex
    // These values are only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is also specified in the flags field of VkGraphicsPipelineCreateInfo
    create_info.basePipelineHandle  = VK_NULL_HANDLE; // Optional
    create_info.basePipelineIndex   = -1; // Optional
    // TODO(k): not sure if we should do it
    create_info.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    if(old != NULL)
    {
      create_info.basePipelineHandle = old->h;
      create_info.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    }

    // The vkCreateGraphicsPipelines function actually has more parameters than the usual object creation functions in Vulkan
    // It is designed to take multiple VkGraphicsPipelineCreateInfo objects and create multiple Vkpipeline objects in the single call
    // The second parameter, for which we've passed the VK_NULL_HANDLE argument, references an optional VkPipelineCache object
    // A pipeline cache can be used to store and reuse data relevant to pipeline creation across multiple calls to vkCreateGraphcisPipelines and even across program executions if the 
    // cache is stored to a file
    // This makes it possbile to significantly speed up pipeline creation at a later time
    VK_Assert(vkCreateGraphicsPipelines(r_vulkan_state->logical_device.h, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline.h));
  }
  scratch_end(scratch);
  return pipeline;
}

internal R_Vulkan_Pipeline
r_vulkan_cmp_pipeline(R_Vulkan_PipelineKind kind)
{
  R_Vulkan_Pipeline ret;
  ret.kind = kind;

  Temp scratch = scratch_begin(0,0);

  ///////////////////////////////////////////////////////////////////////////////////////
  // specialization constants

  VkSpecializationInfo spec_info = {0};

  ///////////////////////////////////////////////////////////////////////////////////////
  // shader stage

  VkShaderModule compute_shader_mo;
  switch(kind)
  {
    case(R_Vulkan_PipelineKind_CMP_Geo3D_TileFrustum):
    {
      compute_shader_mo = r_vulkan_state->cshad_modules[R_Vulkan_CShadKind_Geo3D_TileFrustum];
      // spec constants
      Vec2U32 *tile_size = push_array(scratch.arena, Vec2U32, 1);
      tile_size->x = R_VULKAN_TILE_SIZE;
      tile_size->y = R_VULKAN_TILE_SIZE;

      VkSpecializationMapEntry *entries = push_array(scratch.arena, VkSpecializationMapEntry, 2);
      entries[0].constantID = 0;
      entries[0].offset = OffsetOf(Vec2U32, x);
      entries[0].size = sizeof(U32);
      entries[1].constantID = 1;
      entries[1].offset = OffsetOf(Vec2U32, y);
      entries[1].size = sizeof(U32);

      spec_info.pData = tile_size;
      spec_info.dataSize = sizeof(Vec2U32);
      spec_info.pMapEntries = entries;
      spec_info.mapEntryCount = 2;
    }break;
    case(R_Vulkan_PipelineKind_CMP_Geo3D_LightCulling):
    {
      compute_shader_mo = r_vulkan_state->cshad_modules[R_Vulkan_CShadKind_Geo3D_LightCulling];

      // spec constants
      Vec2U32 *tile_size = push_array(scratch.arena, Vec2U32, 1);
      tile_size->x = R_VULKAN_TILE_SIZE;
      tile_size->y = R_VULKAN_TILE_SIZE;

      VkSpecializationMapEntry *entries = push_array(scratch.arena, VkSpecializationMapEntry, 2);
      entries[0].constantID = 0;
      entries[0].offset = OffsetOf(Vec2U32, x);
      entries[0].size = sizeof(U32);
      entries[1].constantID = 1;
      entries[1].offset = OffsetOf(Vec2U32, y);
      entries[1].size = sizeof(U32);

      spec_info.pData = tile_size;
      spec_info.dataSize = sizeof(Vec2U32);
      spec_info.pMapEntries = entries;
      spec_info.mapEntryCount = 2;
    }break;
    default: {InvalidPath;}break;
  }

  VkPipelineShaderStageCreateInfo shad_stage = {0};
  shad_stage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shad_stage.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
  shad_stage.module              = compute_shader_mo;
  shad_stage.pName               = "main";
  shad_stage.pSpecializationInfo = &spec_info;

  ///////////////////////////////////////////////////////////////////////////////////////
  // pipeline layout

  VkPipelineLayoutCreateInfo layout_info = {0};
  VkDescriptorSetLayout *set_layouts;
  U64 set_layout_count = 0;
  switch(kind)
  {
    case(R_Vulkan_PipelineKind_CMP_Geo3D_TileFrustum):
    {
      set_layout_count = 2;
      set_layouts = push_array(scratch.arena, VkDescriptorSetLayout, set_layout_count);
      set_layouts[0] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Geo3D_TileFrustum].h;
      set_layouts[1] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_Tiles].h;
    }break;
    case(R_Vulkan_PipelineKind_CMP_Geo3D_LightCulling):
    {
      set_layout_count = 6;
      set_layouts = push_array(scratch.arena, VkDescriptorSetLayout, set_layout_count);
      set_layouts[0] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Geo3D_LightCulling].h;
      set_layouts[1] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_Lights].h;
      set_layouts[2] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;
      set_layouts[3] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_Tiles].h;
      set_layouts[4] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_LightIndices].h;
      set_layouts[5] = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_TileLights].h;
    }break;
    default: {InvalidPath;}break;
  }

  layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout_info.setLayoutCount = set_layout_count;
  layout_info.pSetLayouts = set_layouts;
  layout_info.pushConstantRangeCount = 0;
  layout_info.pPushConstantRanges = NULL;

  VkPipelineLayout layout;
  VK_Assert(vkCreatePipelineLayout(r_vulkan_state->logical_device.h, &layout_info, NULL, &layout));

  ///////////////////////////////////////////////////////////////////////////////////////
  // create pipeline

  VkComputePipelineCreateInfo pipeline_info = {0};
  pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipeline_info.layout = layout;
  pipeline_info.stage = shad_stage;

  VK_Assert(vkCreateComputePipelines(r_vulkan_state->logical_device.h, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &ret.h));
  ret.layout = layout;
  scratch_end(scratch);
  return ret;
}

//- sampler

internal VkSampler
r_vulkan_sampler2d(R_Tex2DSampleKind kind)
{
  // It is possible for shaders to read texels directly from iamges, but that is not very common when they are used a textures
  // Textures are usually accessed through samplers, which will aplly filtering and transformation to compute the final color that is retrieved
  // These filters are helpful to deal with problems like oversampling (geometry with more fragments than texels, like pixel game or Minecraft)
  // In this case if you combined the 4 closed texels through linear interpolation, then you would get a smoother result
  // The linear interpolation in oversampling is preferred in conventional graphics applications
  // A sampler object automatically applies the filtering for you when reading a color from the texture
  //
  // Undersampling is the opposite problem, where you have more texels than fragments
  // This will lead to artifacts when sampling high frequency patterns like a checkerboard texture at a sharp angle
  // The solution to this is *anisotropic filtering*, which can also be applied automatically by sampler
  //
  // Aside from these filters, a sampler can also take care of transformation
  // It determines what happens when you try to read texels outside the image through its addressing mode
  // * Repeat
  // * Mirrored repeat
  // * Clamp to edge
  // * Clamp to border
  VkSampler sampler;

  VkSamplerCreateInfo create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  // The magFilter and minFilter fields specify how to interpolate texels that are magnified or minified
  // Magnification concerns the oversampling proble describes above
  // Minification concerns undersampling
  // The choices are VK_FILTER_NEAREST and VK_FILTER_LINEAR render_vulkan
  switch(kind)
  {
    case R_Tex2DSampleKind_Nearest: {create_info.magFilter = VK_FILTER_NEAREST; create_info.minFilter = VK_FILTER_NEAREST;}break;
    case R_Tex2DSampleKind_Linear:  {create_info.magFilter = VK_FILTER_LINEAR; create_info.minFilter = VK_FILTER_LINEAR;}break;
    default:                        {InvalidPath;}break;
  }

  // Note that the axes are called U, V and W instead of X, Y and Z
  // VK_SAMPLER_ADDRESS_MODE_REPEAT:               repeat the texture when going beyond the image dimension
  // VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:      like repeat mode, but inverts the coordinates to mirror the image when going beyond the dimension
  // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:        take the color of the edge cloesest to the coordinate beyond the image dimensions
  // VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: like clamp to edge, but instead uses the edge opposite the closest edge
  // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:      return a solid color when sampling beyond the dimensions of the image
  // It doesn't really matter which addressing mode we use here, because we're not going to sample outside of the image in this tutorial
  // However, the repeat mode is probably the most common mode, because it can be used to tile textures like floors an walls
  create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  // These two fields specify if anisotropic filtering should be used
  // There is no reason not to use this unless performance is concern
  // The maxAnisotropy field limits the amount of texel samples that can be used to calculate the final color
  // A lower value results in better performance, but lower quality results
  // To figure out which value we can use, we need to retrieve the properties of the physical device
  create_info.anisotropyEnable = VK_TRUE;
  create_info.maxAnisotropy    = r_vulkan_pdevice()->properties.limits.maxSamplerAnisotropy;
  // The borderColor field specifies which color is returned when sampling beyond the image with clamp to border addressing mode
  // It is possible to return black, white or transparent in either float or int formats
  // You cannot specify an arbitrary color
  create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  // The unnormalizedCoordinates fields specifies which coordinate system you want to use to address texels in an image
  // If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth] and [0, textHeight] range
  // If this field is VK_FALSE, then the texels are addressed using [0, 1] range on all axes
  // Real world applications almost always use normalized coordinates, because then it's possible to use textures of varying resolutions with the exact same coordinates
  create_info.unnormalizedCoordinates = VK_FALSE;
  // If a comparsion function is enabled, then texels will first be compared to a value, and the result of that comparison is used in filtering operations
  // TODO(k): This si mainly used for percentage-closer filtering "https://developer.nvidia.com/gpugems/GPUGems/gpugems_ch11.html" on shadow maps
  // We are not using any of that
  create_info.compareEnable = VK_FALSE;
  create_info.compareOp     = VK_COMPARE_OP_ALWAYS;
  // NOTE(k): These 4 fields apply to mipmapping
  // We will look at mipmapping in a later chapter, but basically it's another type of filter that can be applied
  create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  create_info.mipLodBias = 0.0f;
  create_info.minLod     = 0.0f;
  create_info.maxLod     = 0.0f;

  VK_Assert(vkCreateSampler(r_vulkan_state->logical_device.h, &create_info, NULL, &sampler));
  // Note the sampler does not reference a VkImage anywhere
  // The sampler is a distinct object that provides an interface to extrat colors from a texture
  // It can be applied to any image you want, whether it is 1D, 2D or 3D
  // This is different from many older APIS, which combined texture images and filtering into a single state

  return sampler;
}

//- helpers

internal S32
r_vulkan_memory_index_from_type_filer(U32 type_bits, VkMemoryPropertyFlags properties)
{
  // The VkMemoryRequirements struct has three fields
  // 1.           size: the size of the required amount of memory in bytes, may differ from vertex_buffer.size, e.g. (60 requested vs 64 got, alignment is 16)
  // 2.      alignment: the offset in bytes where the buffer begins in the allocated region of memory, depends on vertex_buffer.usage and vertex_buffer.flags
  // 3. memoryTypeBits: bit field of the memory types that are suitable for the buffer
  // Graphics cards can offer different tyeps of memory to allcoate from
  // Each type of memory varies in terms of allowed operations and performance characteristics
  // We need to combine the requirements of the buffer and our own application requirements to find the right type of memory to use
  // First we need to query info about the available types of memory using vkGetPhysicalDeviceMemoryProperties
  // The VkPhyscicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps
  // Memory heaps are distinct memory resources like didicated VRAM and swap space in RAM for when VRAM runs out
  // The different types of memory exist within these heaps
  // TODO(k): Right now we'll only concern ourselves with the type of memory and not the heap it comes from, but you can imagine that this can affect performance
  S64 ret = -1;
  VkMemoryType *mem_types = r_vulkan_pdevice()->mem_properties.memoryTypes;
  U64 mem_type_count = r_vulkan_pdevice()->mem_properties.memoryTypeCount;
  for(U64 i = 0; i < mem_type_count; i++)
  {
    // However, we're not just interested in a memory type that is suitable for the vertex buffer
    // We also need to be able to write our vertex data to that memory
    // The memoryTypes array consist of VkMemoryType structs that specify the heap and properties of each type of memory
    // The properties define special features of the memory, like being able to map it so we can write to it from the CPU
    // This property is indicated with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, but we also need to the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property
    if((type_bits & (1<<i)) && ((mem_types[i].propertyFlags & properties) == properties))
    {
      ret = i;
      break;
    }
  }
  AssertAlways(ret != -1);
  return ret;
}

////////////////////////////////
//- Helper Functions

//- debug

/*
 * The first parameter specifies the severity of the message, which is one of the following flags
 * 1. VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: diagnostic message
 * 2. VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT   : information message like the creation of a resource
 * 3. VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: message about behavior that is not necessarily an error, but very likely a bug in your application
 * 4. VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT  : message about behavior that is invalid and may cause crashes
 *
 * The message_type parameter can have the following values
 * 1. VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    : some event has happened that is unrelated to the specification or performance
 * 2. VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT : something has happeded that violates the specification or indicates a possbile mistake
 * 3. VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: potential non-optimal use of vulkan
 *
 * The p_callback_data parameter refers to a VkDebugUtilsMessengerCallbackDataEXT struct containing the details of the message itself, with the most important members being
 * 1. pMessage   : the debug message as a null-terminated string
 * 2. pObjects   : array of vulkan object handles related to the message
 * 3. objectCount: number of objects in array
 *
 * Finally, the p_userdata parameter contains a pointer that was specified during the setup of the callback and allows you to pass your own data to it
 */
internal VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
               VkDebugUtilsMessageTypeFlagsEXT message_type,
               const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
               void *p_userdata)
{
  if(message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    fprintf(stderr, "validation layer: %s\n", p_callback_data->pMessage);
  }

  // The callback returns a boolean that indicates if the Vulkan call that triggered the validation layer message should be aborted.
  // If the callback returns true, then the call is aborted with the VK_ERROR_VALIDATION_FAILED_EXT error.
  // This is normally only used to test the validation layers themselved, so you should always return VK_FALSE
  return VK_FALSE;
}

//- command buffer scope

internal void
r_vulkan_cmd_begin(VkCommandBuffer cmd_buf)
{
  VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  // The flags parameter specifies how we're going to use the command buffer
  // The following values are available
  // 1. VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: this command buffer will be re-recoreded right after executing it once
  // 2. VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: this is a secondary command buffer that will be entirely within a single render pass
  // 3. VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: the command buffer can be resubmitted while it is also already pending execution
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  // The pInheritanceInfo parameter is only relevant for secondary command buffers
  // It specifies which state to inherit from the calling primary command buffers
  // Put it simple, they are necessary parameters that the second command buffers requires
  begin_info.pInheritanceInfo = NULL; // Optional

  // If the command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicity reset it
  // It's not possbile to append commands to a buffer at a later time
  VK_Assert(vkBeginCommandBuffer(cmd_buf, &begin_info));
}

internal void
r_vulkan_cmd_end(VkCommandBuffer cmd_buf)
{
  VkSubmitInfo submit_info = 
  {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, 
    .commandBufferCount = 1,
    .pCommandBuffers = &cmd_buf,
  };
  VK_Assert(vkEndCommandBuffer(cmd_buf));
  VK_Assert(vkQueueSubmit(r_vulkan_state->logical_device.gfx_queue, 1, &submit_info, VK_NULL_HANDLE));
}

////////////////////////////////
//~ rjf: Backend Hooks

//- rjf: top-level layer initialization

r_hook void
r_init(OS_Handle window, B32 debug)
{
  Arena *arena = arena_alloc();
  r_vulkan_state = push_array(arena, R_Vulkan_State, 1);
  r_vulkan_state->arena = arena;
  r_vulkan_state->frame_arena = arena_alloc();
  r_vulkan_state->debug = debug;
  r_vulkan_state->device_rw_mutex = rw_mutex_alloc();
  Temp scratch = scratch_begin(0,0);

  ////////////////////////////////
  //~ Api version

  U32 inst_version = 0;
  VK_Assert(vkEnumerateInstanceVersion(&inst_version));
  U32 major = VK_API_VERSION_MAJOR(inst_version);
  U32 minor = VK_API_VERSION_MINOR(inst_version);
  U32 patch = VK_API_VERSION_PATCH(inst_version);

  r_vulkan_state->instance_version_major = major;
  r_vulkan_state->instance_version_minor = minor;
  r_vulkan_state->instance_version_patch = patch;

  // Dynamic rendering requires at least 1.3
  if(major < 1 || minor < 3)
  {
    // VK_API_VERSION_1_3
    Temp scratch = scratch_begin(0, 0);
    String8 message = push_str8f(scratch.arena, "Unsupported Vulkan version (%i.%i, need at least 1.3)", major, minor);
    os_graphical_message(1, str8_lit("Fatal Error"), message);
    os_abort(1);
    scratch_end(scratch);
  }

  ////////////////////////////////
  //~ Application info

  VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
  {
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.pEngineName = "";
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.apiVersion = VK_MAKE_API_VERSION(0, major, minor, 0);
    app_info.pNext = NULL; // point to extension information
  }

  ////////////////////////////////
  //~ Instance extensions info

  U64 enabled_ext_count;
  char **enabled_ext_names;
  {
    // NOTE(k): instance extensions is not the same as the physical device extensions
    U32 ext_count;
    vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);
    VkExtensionProperties *extensions = push_array(scratch.arena, VkExtensionProperties, ext_count);
    vkEnumerateInstanceExtensionProperties(NULL, &ext_count, extensions); /* query the extension details */

    printf("%d instance extensions supported\n", ext_count);
    for(U64 i = 0; i < ext_count; i++)
    {
      printf("[%3lu]: %s [%d] is supported\n", (unsigned long)i, extensions[i].extensionName, extensions[i].specVersion);
    }

    // Required Extensions
    // Glfw required instance extensions
    U64 required_inst_ext_count = 2;
    char **required_inst_exts = push_array(scratch.arena, char*, required_inst_ext_count);
    required_inst_exts[0] = "VK_KHR_surface";
    required_inst_exts[1] = os_vulkan_surface_ext();

    // Assert every required extension by glfw is in the supported extensions list
    U64 found = 0;
    for(U64 i = 0; i < required_inst_ext_count; i++)
    {
      const char *ext = required_inst_exts[i];
      for(U64 j = 0; j < ext_count; j++)
      {
        if(strcmp(ext, extensions[j].extensionName))
        {
          found++;
          break;
        }
      }
    }
    AssertAlways(found == required_inst_ext_count);

    enabled_ext_count = required_inst_ext_count;
    // NOTE(k): add one for optional debug extension
    enabled_ext_names = push_array(scratch.arena, char *, required_inst_ext_count + 1);

    for(U64 i = 0; i < required_inst_ext_count; i++)
    {
      enabled_ext_names[i] = (char *)required_inst_exts[i];
    }

    if(r_vulkan_state->debug)
    {
      enabled_ext_names[enabled_ext_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }
  }

  ////////////////////////////////
  //~ Validation Layer

  // All of the useful standard validation layer is bundle into a layer included in the SDK that is known as VK_LAYER_KHRONOS_validation 
  U64 enabled_layer_count = 0;
  const char *enabled_layers[] = { "VK_LAYER_KHRONOS_validation" };

  if(r_vulkan_state->debug)
  {
    enabled_layer_count = 1;
  }

  // get instance layer info
  U32 instance_layer_count;
  VK_Assert(vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL));
  VkLayerProperties *instance_layers = push_array(scratch.arena, VkLayerProperties, instance_layer_count);
  VK_Assert(vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers));

  // check if all enabled layers are presented
  for(U64 i = 0; i < enabled_layer_count; i++)
  {
    B32 found = 0;
    for(U64 j = 0; j < instance_layer_count; j++)
    {
      if(strcmp(instance_layers[j].layerName, enabled_layers[i]) == 0)
      {
        found = 1;
        break;
      }
    }

    if(!found)
    {
      fprintf(stderr, "layer '%s' was not found for this instance\n", enabled_layers[i]);
      Trap();
    }
  }

  ////////////////////////////////
  //~ Create vulkan instance (create debug_messenger if needed)

  // It tells the Vulkan driver which global extension and validation layers we want ot use.
  // Global here means that they apply to the entire program and not a specific device
  {
    VkInstanceCreateInfo create_info = {
      .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo        = &app_info,
      .enabledExtensionCount   = enabled_ext_count, /* the last two members of the struct determine the global validation layers to enable */
      .ppEnabledExtensionNames = (const char **)enabled_ext_names,
      .enabledLayerCount       = 0,
      .ppEnabledLayerNames     = NULL,
      .pNext                   = NULL,
    };
    // Although we've now added debugging with validation layers to the
    //      program we're not covering everything quite yet.
    // The vkCreateDebugUtilsMessengerEXT call requires a valid instance
    //      to have been created and vkDestroyDebugUtilsMessengerEXT must be called before the instance is destroyed.
    // This currently leaves us unable to debug any issues in the vkCreateInstance 
    //      and vkDestroyInstance calls.
    // However, if you closely read the extension documentation, you'll see that
    //      there is a way to create a separate debug utils messenger specifically for those two function calls.
    // It requires you to simply pass a pointer to a VkDebugUtilsMessengerCreateInfoEXT     
    //      struct in the pNext extension field of VkInstanceCreateInfo.
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info =
    {
      .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debug_callback,
      .pUserData       = NULL, // Optional 
    };

    create_info.enabledLayerCount   = enabled_layer_count;
    create_info.ppEnabledLayerNames = enabled_layers;

    if(r_vulkan_state->debug)
    {
      create_info.pNext = &debug_messenger_create_info;
    }

    // create instance
    VK_Assert(vkCreateInstance(&create_info, NULL, &r_vulkan_state->instance));

    if(r_vulkan_state->debug)
    {
      // This struct should be passed to the vkCreateDebugUtilsMessengerEXT function to create the VkDebugUtilsMessengerEXT object
      // Unfortunately, because this function is an extension function, it is not automatically loaded, we have to load it ourself
      PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(r_vulkan_state->instance, "vkCreateDebugUtilsMessengerEXT");
      AssertAlways(vkCreateDebugUtilsMessengerEXT != NULL);

      r_vulkan_state->vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(r_vulkan_state->instance, "vkDestroyDebugUtilsMessengerEXT");
      AssertAlways(r_vulkan_state->vkDestroyDebugUtilsMessengerEXT != NULL);

      /* VK_ERROR_EXTENSION_NOT_PRESENT */
      VK_Assert(vkCreateDebugUtilsMessengerEXT(r_vulkan_state->instance, &debug_messenger_create_info, NULL, &r_vulkan_state->debug_messenger));
    }
  }

  ////////////////////////////////
  //~ Create window surface

  // Since Vulkan is a platform agnostic API, it can not interface directly with the window system on its own
  // To establish the connection between Vulkan and the window system to present results to the screen, we need to use the WSI (Window System Integration) extensions
  // The first one is VK_KHR_surface. It exposes a VkSurfaceKHR object that represents
  //      an abstract type of surface to present rendered images to.
  //      The surface in our program will be backed by the window that we've already opened with GLFW
  // The VK_KHR_surface extension is an instance level extension and we've actually already enabled it
  //      because it's included in the list returned by glfwGetRequiredInstanceExtensions.
  //      The list also includes some other WSI related extensions
  //
  // The window surface needs to be created right after the instance creation, because it can actually influence the physical device selection
  // It should also be noted that window surfaces are entirely optional component in Vulkan, if you just need off-screen rendering.
  // Vulkan allows you to do that without hacks like creating an invisible window (necessary for OpenGL)
  R_Vulkan_Surface surface = {0};
  surface.h = os_vulkan_surface_from_window(window, r_vulkan_state->instance);

  ////////////////////////////////
  //~ Pick the physical device

  // NOTE(k)This object will be implicitly destroyed when the VkInstance is destroyed
  //      so we won't need to do anything new in the "Cleanup" section

  const char *required_pdevice_ext_names[] = {
    // It should be noted that the availablility of a presentation queue, implies that the swap chain extension must be supported, and vice vesa
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
  };
  {
    U32 pdevice_src_count = 0;
    VK_Assert(vkEnumeratePhysicalDevices(r_vulkan_state->instance, &pdevice_src_count, NULL));
    VkPhysicalDevice *pdevices_src = push_array(scratch.arena, VkPhysicalDevice, pdevice_src_count);
    VK_Assert(vkEnumeratePhysicalDevices(r_vulkan_state->instance, &pdevice_src_count, pdevices_src));

    U64 pdevice_dst_count = 0;
    R_Vulkan_PhysicalDevice *pdevices_dst = push_array(arena, R_Vulkan_PhysicalDevice, pdevice_src_count);

    for(U64 i = 0; i < pdevice_src_count; i++)
    {
      VkPhysicalDeviceProperties properties;
      VkPhysicalDeviceFeatures features;
      vkGetPhysicalDeviceProperties(pdevices_src[i], &properties);
      vkGetPhysicalDeviceFeatures(pdevices_src[i], &features);

      // query physical device supported extensions
      U32 ext_count;
      VK_Assert(vkEnumerateDeviceExtensionProperties(pdevices_src[i], NULL, &ext_count, NULL));
      VkExtensionProperties *extensions = push_array(scratch.arena, VkExtensionProperties, ext_count);
      VK_Assert(vkEnumerateDeviceExtensionProperties(pdevices_src[i], NULL, &ext_count, extensions));

      // Check if current physical device supports all the required extensions
      U64 found = 0;
      for(U64 j = 0; j < ArrayCount(required_pdevice_ext_names); j++)
      {
        for(U64 k = 0; k < ext_count; k++)
        {
          if(strcmp(extensions[k].extensionName, required_pdevice_ext_names[j]))
          {
            found++;
            break;
          } 
        }
      }
      if(found != ArrayCount(required_pdevice_ext_names)) continue;

      // If application can't function without geometry shaders, we don't need it for now
      // if(features.geometryShader == VK_FALSE) continue;
      // We want Anisotropy
      if(features.samplerAnisotropy == VK_FALSE) continue;
      if(features.independentBlend == VK_FALSE)  continue;
      if(features.fillModeNonSolid == VK_FALSE)  continue;
      if(features.wideLines == VK_FALSE)         continue;

      R_Vulkan_PhysicalDevice *dst = &pdevices_dst[pdevice_dst_count++];
      dst->h = pdevices_src[i];
      dst->properties = properties;
      dst->features = features;
      dst->extensions = push_array(arena, VkExtensionProperties, ext_count);
      MemoryCopy(dst->extensions, extensions, sizeof(VkExtensionProperties)*ext_count);
      dst->extension_count = ext_count;
    }

    // pick the most performant physical device
    S32 pdevice_idx = -1;
    U64 pdevice_score = 0;
    for(U64 i = 0; i < pdevice_dst_count; i++)
    {
      R_Vulkan_PhysicalDevice *pdevice = &pdevices_dst[i];
      U64 score = 0;

      if(pdevice->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {score += 1;}
      // score += properties.limits.maxImageDimension2D;

      // Querying details of swap chain support
      // Just checking if a swap chain is avaiable is not sufficient, because it may not actually be compatible with our window surface
      // Creating a swap chain also involves a lot more settings than instance and device creation
      //   so we need to query for some more details before we're able to proceed
      //
      // There are basically three kinds of properties we need to check:
      // 1. Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
      // 2. Surface formats (pixel format, color space)
      // 3. Available presentation modes

      VkSurfaceCapabilitiesKHR surface_caps;
      // This function takes the specified **VkPhysicalDevice** and **VkSurfaceKHR window surface** into account when determining the supported capabilities.
      // All of the support querying functions have these two as first parameters, because they are the **core components** of the swap chain
      VK_Assert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevices_src[i], surface.h, &surface_caps));

      U32 surface_format_count;
      VK_Assert(vkGetPhysicalDeviceSurfaceFormatsKHR(pdevices_src[i], surface.h, &surface_format_count, NULL));
      VkSurfaceFormatKHR *surface_formats = push_array(scratch.arena, VkSurfaceFormatKHR, surface_format_count);
      VK_Assert(vkGetPhysicalDeviceSurfaceFormatsKHR(pdevices_src[i], surface.h, &surface_format_count, surface_formats));

      U32 surface_present_mode_count;
      VK_Assert(vkGetPhysicalDeviceSurfacePresentModesKHR(pdevices_src[i], surface.h, &surface_present_mode_count, NULL));
      VkPresentModeKHR *surface_present_modes = push_array(scratch.arena, VkPresentModeKHR, surface_present_mode_count);
      VK_Assert(vkGetPhysicalDeviceSurfacePresentModesKHR(pdevices_src[i], surface.h, &surface_present_mode_count, surface_present_modes));

      // For now, swap chain support is sufficient if there is at least one supported image format and one supported presentation mode given the window surface we have
      if(surface_format_count == 0 || surface_present_mode_count == 0) continue;
      score += 1;
      if(score > pdevice_score)
      {
        pdevice_idx = i;
        pdevice_score = score;

        surface.caps = surface_caps;
        surface.format_count = ClampBot(surface_format_count, ArrayCount(surface.formats));
        MemoryCopy(surface.formats, surface_formats, sizeof(VkSurfaceFormatKHR) * surface.format_count);

        surface.prest_mode_count = ClampBot(surface_present_mode_count, ArrayCount(surface.prest_modes));
        MemoryCopy(surface.prest_modes, surface_present_modes, sizeof(VkPresentModeKHR) * surface.prest_mode_count);
      }
    }
    AssertAlways(pdevice_idx >= 0 && "No suitable physical device was founed");

    R_Vulkan_PhysicalDevice *pdevice = &pdevices_dst[pdevice_idx];
    vkGetPhysicalDeviceMemoryProperties(pdevice->h, &pdevice->mem_properties);
    pdevice->depth_image_format = r_vulkan_optimal_depth_format_from_pdevice(pdevice->h);

    r_vulkan_state->physical_devices      = pdevices_dst;
    r_vulkan_state->physical_device_count = pdevice_dst_count;
    r_vulkan_state->physical_device_idx   = pdevice_idx;
  }

  ////////////////////////////////
  //~ Queue families information

  // Almost every operation in Vulkan, anything from drawing to uploading textures
  //      requires commands to be submmited to a queue
  // There are different types of queues that originate from different queue families 
  //      and each family of queues allows only a subset of commands
  // For example, there could be a queue family that only allows processing of compute 
  //      commands or one that only allows memory transfer related commands
  // We need to check which queue families are supported by the physical device and
  //      which one of these supports the commands that we want to use
  // Right now we are only going to look a queue that supports graphics commands
  // Also we may prefer devices with a dedicated transfer queue family, but not require it

  // The buffer copy command requires a queue family that supports transfer operations,
  //      which is indicated using VK_QUEUE_TRANSFER_BIT
  // The good news is that any queue family with VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT 
  //      capabilities already implicitly support VK_QUEUE_TRANSFER_BIT operations
  // The implementation is not required to explicitly list it in queueFlags in those cases
  // If you like a challenge, then you can still try to use a different queue family specifically for transfer operations
  // It will require you to make the following modifications to your program
  // 1. Find the queue family with VK_QUEUE_TRANSFER_BIT bit, but not the VK_QUEUE_GRAPHICS_BIT
  // 2. Request a handle to the transfer queue
  // 3. Create a second command pool for command buffers that are submitted on the transfer queue family
  // 4. Change the sharingMode of resources to be VK_SHARING_MODE_CONCURRENT and specify 
  //      both the graphics and transfer queue families, since we would copy resources from transfer queue to graphics queue
  // 5. Submit any transfer commands like vkCmdCopyBuffer to the transfer queue instead of the graphcis queue

  {
    // Since the presentation is a queue-specific feature, we would need to find a queue
    //      family that supports presenting to the surface we created
    // It's actually possible that the queue families supporting drawing commands 
    //      (aka graphic queue family) and the ones supporting presentation do not overlap
    // Therefore we have to take into account that there could be a distinct presentation queue
    B32 has_graphic_and_compute_queue_family = 0;
    B32 has_present_queue_family = 0;

    U32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(r_vulkan_pdevice()->h, &queue_family_count, NULL);
    VkQueueFamilyProperties *queue_family_properties = push_array(scratch.arena, VkQueueFamilyProperties, queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(r_vulkan_pdevice()->h, &queue_family_count, queue_family_properties);

    for(U64 i = 0; i < queue_family_count; i++)
    {
      VkQueueFamilyProperties prop = queue_family_properties[i];
      VkQueueFlags flags = prop.queueFlags;

      if((flags&VK_QUEUE_GRAPHICS_BIT) && (flags&VK_QUEUE_COMPUTE_BIT))
      {
        r_vulkan_pdevice()->gfx_queue_family_index = i;
        r_vulkan_pdevice()->cmp_queue_family_index = i;
        has_graphic_and_compute_queue_family = 1;
      }

      // if((flags&VK_QUEUE_TRANSFER_BIT) && !(flags&VK_QUEUE_GRAPHICS_BIT) && !(flags&VK_QUEUE_COMPUTE_BIT))
      // {
      //   has_dedicated_transfer_queue_family = 1;
      //   r_vulkan_pdevice()->xfer_queue_family_index = i;
      // }

      VkBool32 present_supported = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(r_vulkan_pdevice()->h, i, surface.h, &present_supported);
      if(present_supported == VK_TRUE)
      {
        r_vulkan_pdevice()->prest_queue_family_index = i;
        has_present_queue_family = 1;
      }

      // Note that it's very likely that these end up being the same queue family after all
      //      but throughout the program we will treat them as if they were seprate queues for a uniform approach.
      // TODO(k): Nevertheless, we would prefer a physical device that supports drawing and presentation in the same queue for improved performance.
      if(has_graphic_and_compute_queue_family && has_present_queue_family) break;

    }
    AssertAlways(has_graphic_and_compute_queue_family);
    AssertAlways(has_present_queue_family);
  }

  ////////////////////////////////
  //~ Logical device and queues

  // After selecting a physical device to use we need to set up a logical device to interface with it.
  // The logical device creation process is similar to the instance creation process and describes the features we want to use
  // We also need to specify which queues to create nwo that we've queried which queue families are available
  // You can even create multiple logical devices from the same physical device if you have varying requirements
  // Create one graphic queue
  // The currently available drivers will only allow you to create a small number of queues for each queue family
  // And you don't really need more than one. That's because you can create all of the commands buffers on multiple threads and then submit them all at once on the main thread with a single low-overhead call
  {
    const F32 gfx_queue_priority = 1.0f;
    const F32 prest_queue_priority = 1.0f;

    // If a queue family both support drawing and presentation, could we just create one queue
    U32 num_queues_to_create = r_vulkan_pdevice()->gfx_queue_family_index == r_vulkan_pdevice()->prest_queue_family_index ? 1 : 2;
    VkDeviceQueueCreateInfo queue_create_infos[3] = {
      // graphic queue
      {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = r_vulkan_pdevice()->gfx_queue_family_index,
        .queueCount       = 1,
        .pQueuePriorities = &gfx_queue_priority,
      },
      // present queue, this second create info will be ignored if num_queues_to_create is 1
      {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = r_vulkan_pdevice()->prest_queue_family_index,
        .queueCount       = 1,
        .pQueuePriorities = &prest_queue_priority,
      },
    };

    // Specifying used device features, don't need anything special for now, leave everything to VK_FALSE
    VkPhysicalDeviceFeatures device_features = { VK_FALSE };
    // required
    device_features.samplerAnisotropy = VK_TRUE;
    device_features.independentBlend  = VK_TRUE;
    device_features.fillModeNonSolid  = VK_TRUE;
    // optional
    if(r_vulkan_pdevice()->features.wideLines == VK_TRUE)
    {
      device_features.wideLines = VK_TRUE;
    }
    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR};
    dynamic_rendering_feature.dynamicRendering = VK_TRUE;

    // Create the logical device
    VkDeviceCreateInfo create_info = {
      .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext                   = &dynamic_rendering_feature,
      .pQueueCreateInfos       = queue_create_infos,
      .queueCreateInfoCount    = num_queues_to_create,
      .pEnabledFeatures        = &device_features,
      .enabledLayerCount       = 0,
      .enabledExtensionCount   = ArrayCount(required_pdevice_ext_names),
      .ppEnabledExtensionNames = required_pdevice_ext_names,
    };

    // Device specific extension VK_KHR_swapchain, which allows you to present rendered images from that device to windows 
    // It's possible that there are Vulkan devices in the system that lack this ability, for example because they only support compute operations
    // Previous implementations of Vulkan made a distinction between instance and device specific validation layers, but this is no longer the case
    // That means that the enabledLayerCount and ppEnabledLayerNames fields of VkDeviceCreateInfo are ignored by up-to-date implementations. 
    // However, it is still a good idea to set them anyway to be compatible with older implementations
    create_info.enabledLayerCount = enabled_layer_count;
    create_info.ppEnabledLayerNames = enabled_layers;
    VK_Assert(vkCreateDevice(r_vulkan_pdevice()->h, &create_info, NULL, &r_vulkan_state->logical_device.h));

    // Retrieving queue handles
    // The queues are automatically created along with the logical device, but we don't have a handle to itnerface with them yet
    // Also, device queues are implicitly cleaned up when the device is destroyed, so we don't need to do anything in cleanup
    // VkQueue graphics_queue;
    // The third parameter is queu index, since we're only creating a single queue from this family, we'll simply use index 0
    vkGetDeviceQueue(r_vulkan_state->logical_device.h, r_vulkan_pdevice()->gfx_queue_family_index, 0, &r_vulkan_state->logical_device.gfx_queue);
    // VkQueue present_queue;
    // If the queue families are the same, then those two queue handler will be the same
    vkGetDeviceQueue(r_vulkan_state->logical_device.h, r_vulkan_pdevice()->prest_queue_family_index, 0, &r_vulkan_state->logical_device.prest_queue);
  }

  // Create a command pool for gfx queue, can be used to record draw call, image layout transition and copy staging buffer
  // Create a one-shot command buffer for later use
  {
    // Commands in Vulkan, like drawing operations and memory transfers, are not executed directly using function calls
    // You have to record all of the operations you want to perform in command buffer objects
    // The advantage of this is that when we are ready to tell the Vulkan what we want to do, all of the commands are submitted together and Vulkan 
    // can more efficiently process the commands since all of them are available together
    // In addtion, this allows command recording to happen in multiple threads if so desired
    // Command pool manage the memory that is used to store the command buffers and command buffers are allocated from them
    VkCommandPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      // There are two possbile flags for command pools
      // 1. VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: hint that command buffers are re-recorded with new commands very often (may change memory allocation behavior)
      // 2. VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: allow command buffers to be re-recorded individually, without this flag they all have to be reset together
      // TODO(k): this part don't make too much sense to me
      // We will be recording a command buffer every frame, so we want to be able to reset and re-record over it
      // Thus, we need to set the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag bit for our command pool
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = r_vulkan_pdevice()->gfx_queue_family_index,
    };
    // Command buffers are executed by submitting them on one of the device queues, like the graphcis and presentation queues we retrieved
    // Each command pool can only allocate command buffers that are submitted on a single type of queue
    // We're going to record commands for drawing, which is why we've chosen the graphcis queue family
    VK_Assert(vkCreateCommandPool(r_vulkan_state->logical_device.h, &create_info, NULL, &r_vulkan_state->cmd_pool));

    VkCommandBufferAllocateInfo alloc_info = {
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandPool        = r_vulkan_state->cmd_pool,
      .commandBufferCount = 1,
    };

    VK_Assert(vkAllocateCommandBuffers(r_vulkan_state->logical_device.h, &alloc_info, &r_vulkan_state->oneshot_cmd_buf));
  }

  // Create texture samplers
  r_vulkan_state->samplers[R_Tex2DSampleKind_Nearest] = r_vulkan_sampler2d(R_Tex2DSampleKind_Nearest);
  r_vulkan_state->samplers[R_Tex2DSampleKind_Linear]  = r_vulkan_sampler2d(R_Tex2DSampleKind_Linear);

  ////////////////////////////////
  //~ Load shader modules

  // Shader modules are just a thin wrapper around the shader bytecode that we've previously loaded from a file and the functions defined in it
  // The compilation and linking of the SPIR-V bytecode to machine code fro execution by the GPU doesn't happen until the graphics pipeline is created
  // That means that we're allowed to destroy the shader modules again as soon as pipeline creation is finished
  // The one catch here is that the size of the bytecode is specified in bytes, but the bytecode pointer is uint32_t pointer rather than a char pointer
  // You also need to ensure that the data satisfies the alignment requirements of uin32_t 
  for(U64 kind = 0; kind < R_Vulkan_VShadKind_COUNT; kind++)
  {
    VkShaderModule *shad_mo = &r_vulkan_state->vshad_modules[kind];
    String8 shad_code;
    switch(kind)
    {
      case R_Vulkan_VShadKind_Rect:            {shad_code = str8(rect_vert_spv, rect_vert_spv_len);}break;
      case R_Vulkan_VShadKind_Noise:           {shad_code = str8(noise_vert_spv, noise_vert_spv_len);}break;
      case R_Vulkan_VShadKind_Edge:            {shad_code = str8(edge_vert_spv, edge_vert_spv_len);}break;
      case R_Vulkan_VShadKind_Crt:             {shad_code = str8(crt_vert_spv, crt_vert_spv_len);}break;
      case R_Vulkan_VShadKind_Geo2D_Forward:   {shad_code = str8(geo2d_forward_vert_spv, geo2d_forward_vert_spv_len);}break;
      case R_Vulkan_VShadKind_Geo2D_Composite: {shad_code = str8(geo2d_composite_vert_spv, geo2d_composite_vert_spv_len);}break;
      case R_Vulkan_VShadKind_Geo3D_ZPre:      {shad_code = str8(geo3d_zpre_vert_spv, geo3d_zpre_vert_spv_len);}break;
      case R_Vulkan_VShadKind_Geo3D_Debug:     {shad_code = str8(geo3d_debug_vert_spv, geo3d_debug_vert_spv_len);}break;
      case R_Vulkan_VShadKind_Geo3D_Forward:   {shad_code = str8(geo3d_forward_vert_spv, geo3d_forward_vert_spv_len);}break;
      case R_Vulkan_VShadKind_Geo3D_Composite: {shad_code = str8(geo3d_composite_vert_spv, geo3d_composite_vert_spv_len);}break;
      case R_Vulkan_VShadKind_Finalize:        {shad_code = str8(finalize_vert_spv, finalize_vert_spv_len);}break;
      default:                                 {InvalidPath;}break;
    }

    VkShaderModuleCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = shad_code.size,
      .pCode = (U32 *)shad_code.str,
    };
    VK_Assert(vkCreateShaderModule(r_vulkan_state->logical_device.h, &create_info, NULL, shad_mo));
  }

  for(U64 kind = 0; kind < R_Vulkan_FShadKind_COUNT; kind++)
  {
    VkShaderModule *shad_mo = &r_vulkan_state->fshad_modules[kind];
    String8 shad_code;
    switch(kind)
    {
      case R_Vulkan_FShadKind_Rect:            {shad_code = str8(rect_frag_spv, rect_frag_spv_len);}break;
      case R_Vulkan_FShadKind_Noise:           {shad_code = str8(noise_frag_spv, noise_frag_spv_len);}break;
      case R_Vulkan_FShadKind_Edge:            {shad_code = str8(edge_frag_spv, edge_frag_spv_len);}break;
      case R_Vulkan_FShadKind_Crt:             {shad_code = str8(crt_frag_spv, crt_frag_spv_len);}break;
      case R_Vulkan_FShadKind_Geo2D_Forward:   {shad_code = str8(geo2d_forward_frag_spv, geo2d_forward_frag_spv_len);}break;
      case R_Vulkan_FShadKind_Geo2D_Composite: {shad_code = str8(geo2d_composite_frag_spv, geo2d_composite_frag_spv_len);}break;
      case R_Vulkan_FShadKind_Geo3D_ZPre:      {shad_code = str8(geo3d_zpre_frag_spv, geo3d_zpre_frag_spv_len);}break;
      case R_Vulkan_FShadKind_Geo3D_Debug:     {shad_code = str8(geo3d_debug_frag_spv, geo3d_debug_frag_spv_len);}break;
      case R_Vulkan_FShadKind_Geo3D_Forward:   {shad_code = str8(geo3d_forward_frag_spv, geo3d_forward_frag_spv_len);}break;
      case R_Vulkan_FShadKind_Geo3D_Composite: {shad_code = str8(geo3d_composite_frag_spv, geo3d_composite_frag_spv_len);}break;
      case R_Vulkan_FShadKind_Finalize:        {shad_code = str8(finalize_frag_spv, finalize_frag_spv_len);}break;
      default:                                 {InvalidPath;}break;
    }
    VkShaderModuleCreateInfo create_info = {
      .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = shad_code.size,
      .pCode    = (U32 *)shad_code.str,
    };
    VK_Assert(vkCreateShaderModule(r_vulkan_state->logical_device.h, &create_info, NULL, shad_mo));
  }

  for(U64 kind = 0; kind < R_Vulkan_CShadKind_COUNT; kind++)
  {
    VkShaderModule *shad_mo = &r_vulkan_state->cshad_modules[kind];
    String8 shad_code;
    switch(kind)
    {
      case R_Vulkan_CShadKind_Geo3D_TileFrustum:  {shad_code = str8(geo3d_tile_frustum_comp_spv, geo3d_tile_frustum_comp_spv_len);}break;
      case R_Vulkan_CShadKind_Geo3D_LightCulling: {shad_code = str8(geo3d_light_culling_comp_spv, geo3d_light_culling_comp_spv_len);}break;
      default:                                    {InvalidPath;}break;
    }
    VkShaderModuleCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = shad_code.size,
      .pCode = (U32 *)shad_code.str,
    };
    VK_Assert(vkCreateShaderModule(r_vulkan_state->logical_device.h, &create_info, NULL, shad_mo));
  }

  ////////////////////////////////
  //~ Create set layouts

  // R_Vulkan_DescriptorSetKind_UBO_Rect
  {
    R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Rect];
    VkDescriptorSetLayoutBinding *bindings   = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
    set_layout->bindings      = bindings;
    set_layout->binding_count = 1;

    // The first two fields specify the binding used in the shader and the type of descriptor, which is a uniform buffer object
    // It is possible for the shader variable to represent an array of uniform buffer objects, and descriptorCount specifies the number of values in the array
    // This could be used to specify a transformation for each of the bones in a skeleton for skeletal animation
    // Our MVP transformation is in a single uniform buffer object, so we're using a descriptorCount of 1
    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[0].descriptorCount    = 1;
    // We also need to specify in which shader stages the descriptor is going to be referenced
    // The stageFlags field can be a combination of VkShaderStageFlasBits values or the value VK_SHADER_STAGE_ALL_GRAPHICS
    bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
    // NOTE(k): The pImmutableSampers field is only relevant for image sampling related descriptors
    bindings[0].pImmutableSamplers = NULL;

    // All of the descriptor bindings are combined into a single VkDescriptorSetLayout object
    VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    create_info.bindingCount = 1;
    create_info.pBindings    = bindings;
    VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &set_layout->h));
  }
  // R_Vulkan_DescriptorSetKind_UBO_Geo2D
  {
    R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Geo2D];
    VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
    set_layout->bindings = bindings;
    set_layout->binding_count = 1;

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    create_info.bindingCount = 1;
    create_info.pBindings    = bindings;
    VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &set_layout->h));
  }
  // R_Vulkan_DescriptorSetKind_UBO_Geo3D
  {
    R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Geo3D];
    VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
    set_layout->bindings = bindings;
    set_layout->binding_count = 1;

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    create_info.bindingCount = 1;
    create_info.pBindings    = bindings;
    VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &set_layout->h));
  }
  // R_Vulkan_DescriptorSetKind_SBO_Geo3D_Joints
  {
    R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_Joints];
    VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
    set_layout->bindings = bindings;
    set_layout->binding_count = 1;

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    create_info.bindingCount = 1;
    create_info.pBindings    = bindings;
    VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &set_layout->h));
  }
  // R_Vulkan_DescriptorSetKind_SBO_Geo3D_Materials
  {
    R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_Materials];
    VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
    set_layout->bindings = bindings;
    set_layout->binding_count = 1;

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    create_info.bindingCount = 1;
    create_info.pBindings    = bindings;
    VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &set_layout->h));
  }
  // R_Vulkan_DescriptorSetKind_Tex2D
  {
    R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D];
    VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
    set_layout->bindings = bindings;
    set_layout->binding_count = 1;

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    create_info.bindingCount = 1;
    create_info.pBindings    = bindings;
    VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &set_layout->h));
  }
  // R_Vulkan_DescriptorSetKind_UBO_Geo3D_TileFrustum
  {
    R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Geo3D_TileFrustum];
    VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
    set_layout->bindings = bindings;
    set_layout->binding_count = 1;

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    create_info.bindingCount = 1;
    create_info.pBindings    = bindings;
    VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &set_layout->h));
  }
  // R_Vulkan_DescriptorSetKind_UBO_Geo3D_LightCulling
  {
    R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Geo3D_LightCulling];
    VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
    set_layout->bindings = bindings;
    set_layout->binding_count = 1;

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    create_info.bindingCount = 1;
    create_info.pBindings    = bindings;
    VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &set_layout->h));
  }
  // R_Vulkan_DescriptorSetKind_SBO_Geo3D_Tiles
  {
    R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_Tiles];
    VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
    set_layout->bindings = bindings;
    set_layout->binding_count = 1;

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo create_info = {0};
    create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 1;
    create_info.pBindings    = bindings;
    VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &set_layout->h));
  }
  // R_Vulkan_DescriptorSetKind_SBO_Geo3D_Lights
  {
    R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_Lights];
    VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
    set_layout->bindings = bindings;
    set_layout->binding_count = 1;

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo create_info = {0};
    create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 1;
    create_info.pBindings    = bindings;
    VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &set_layout->h));
  }
  // R_Vulkan_DescriptorSetKind_SBO_Geo3D_LightIndices
  {
    R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_LightIndices];
    VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
    set_layout->bindings = bindings;
    set_layout->binding_count = 1;

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo create_info = {0};
    create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 1;
    create_info.pBindings    = bindings;
    VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &set_layout->h));
  }
  // R_Vulkan_DescriptorSetKind_SBO_Geo3D_TileLights
  {
    R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_SBO_Geo3D_TileLights];
    VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
    set_layout->bindings = bindings;
    set_layout->binding_count = 1;

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT|VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo create_info = {0};
    create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 1;
    create_info.pBindings    = bindings;
    VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->logical_device.h, &create_info, NULL, &set_layout->h));
  }

  // init stage state
  r_vulkan_stage_init();

  // create backup/default texture
  U32 backup_texture_data[] = {
    0xFFFF00FF, 0xFF330033,
    0xFF330033, 0xFFFF00FF,
  };
  r_vulkan_state->backup_texture = r_tex2d_alloc(R_ResourceKind_Static, R_Tex2DSampleKind_Nearest, v2s32(2,2), R_Tex2DFormat_RGBA8, backup_texture_data);
  R_Vulkan_InitStacks(r_vulkan_state);
  R_Vulkan_InitStackNils(r_vulkan_state);
  scratch_end(scratch);
}

//- rjf: window setup/teardown

r_hook R_Handle
r_window_equip(OS_Handle os_wnd)
{
  ProfBeginFunction();
  R_Vulkan_Window *ret = r_vulkan_state->first_free_window;

  if(ret == 0)
  {
    ret = push_array(r_vulkan_state->arena, R_Vulkan_Window, 1);
  }
  else
  {
    U64 gen = ret->generation;
    SLLStackPop(r_vulkan_state->first_free_window);
    MemoryZeroStruct(ret);
    ret->generation = gen;
  }

  // create surface
  R_Vulkan_Surface surface = {0};
  surface.h = os_vulkan_surface_from_window(os_wnd, r_vulkan_state->instance);
  r_vulkan_surface_update(&surface);

  // create render targets
  R_Vulkan_RenderTargets *render_targets = r_vulkan_render_targets_alloc(os_wnd, &surface, 0);

  // create frames
  {
    // Command buffers will be automatically freed when their command pool is destroyed, so we don't need explicit cleanup
    // Command buffers are allocated with the vkAllocateCommandBuffers function which takes a VkCommandBufferAllocateInfo struct as parameter that specifies the command pool and number of buffers to allcoate
    VkCommandBuffer command_buffers[R_VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    alloc_info.commandPool = r_vulkan_state->cmd_pool;
    // The level parameter specifies if the allcoated command buffers are primary or secondary command buffers
    // 1. VK_COMMAND_BUFFER_LEVEL_PRIMARY:   can be submitted to a queue for execution, but cannot be called from other command buffers
    // 2. VK_COMMAND_BUFFER_LEVEL_SECONDARY: cannot be submitted directly, but can be called from primary command buffers, useful to reuse common operations
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = R_VULKAN_MAX_FRAMES_IN_FLIGHT;
    VK_Assert(vkAllocateCommandBuffers(r_vulkan_state->logical_device.h, &alloc_info, command_buffers));

    for(U64 i = 0; i < R_VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
      ret->frames[i].cmd_buf = command_buffers[i];

      // Create the synchronization objects for frames

      // One semaphore to signal that an image has been acquired from the swapchain and is ready for rendering
      // Another one to signal that rendering has finished and presentation can happen
      // One fence to make sure only one frame is rendering at a time
      ret->frames[i].img_acq_sem = r_vulkan_semaphore(r_vulkan_state->logical_device.h);
      ret->frames[i].inflt_fence = r_vulkan_fence();

      // Create all uniform buffers
      for(U64 kind = 0; kind < R_Vulkan_UBOTypeKind_COUNT; kind++)
      {
        U64 unit_count;
        switch(kind)
        {
          default:                                      {InvalidPath;}break;
          case R_Vulkan_UBOTypeKind_Rect:               {unit_count = R_MAX_RECT_PASS*R_MAX_RECT_GROUPS;}break;
          case R_Vulkan_UBOTypeKind_Geo2D:              {unit_count = R_MAX_GEO2D_PASS;}break;
          case R_Vulkan_UBOTypeKind_Geo3D:              {unit_count = R_MAX_GEO3D_PASS;}break;
          case R_Vulkan_UBOTypeKind_Geo3D_TileFrustum:  {unit_count = R_MAX_GEO3D_PASS;}break;
          case R_Vulkan_UBOTypeKind_Geo3D_LightCulling: {unit_count = R_MAX_GEO3D_PASS;}break;
        }
        ret->frames[i].ubo_buffers[kind] = r_vulkan_ubo_buffer_alloc(kind, unit_count);
      }

      // Create all storage buffers
      for(U64 kind = 0; kind < R_Vulkan_SBOTypeKind_COUNT; kind++)
      {
        U64 unit_count;
        switch(kind)
        {
          default:                                      {InvalidPath;}break;
          case R_Vulkan_SBOTypeKind_Geo3D_Joints:       {unit_count = R_MAX_GEO3D_PASS;}break;
          case R_Vulkan_SBOTypeKind_Geo3D_Materials:    {unit_count = R_MAX_GEO3D_PASS;}break;
          case R_Vulkan_SBOTypeKind_Geo3D_Lights:       {unit_count = R_MAX_GEO3D_PASS;}break;
          case R_Vulkan_SBOTypeKind_Geo3D_Tiles:        {unit_count = R_MAX_GEO3D_PASS;}break;
          case R_Vulkan_SBOTypeKind_Geo3D_LightIndices: {unit_count = R_MAX_GEO3D_PASS;}break;
          case R_Vulkan_SBOTypeKind_Geo3D_TileLights:   {unit_count = R_MAX_GEO3D_PASS;}break;
        }
        ret->frames[i].sbo_buffers[kind] = r_vulkan_sbo_buffer_alloc(kind, unit_count);
      }

      ////////////////////////////////
      //~ Create instance buffers for rect and geo3d

      // TODO(k): just remember to free this
      // create inst buffer for rect
      for(U64 j = 0; j < R_MAX_RECT_PASS; j++)
      {
        R_Vulkan_Buffer *buffer = &ret->frames[i].inst_buffer_rect[j];
        VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        create_info.size = sizeof(R_Rect2DInst)*R_MAX_RECT_INSTANCES;
        create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_Assert(vkCreateBuffer(r_vulkan_state->logical_device.h, &create_info, NULL, &buffer->h));

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->logical_device.h, buffer->h, &mem_requirements);

        buffer->kind = R_ResourceKind_Static;
        buffer->size = mem_requirements.size;
        buffer->cap  = mem_requirements.size;

        VkMemoryAllocateInfo alloc_info = {0};
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &buffer->memory));
        VK_Assert(vkBindBufferMemory(r_vulkan_state->logical_device.h, buffer->h, buffer->memory, 0));
        VK_Assert(vkMapMemory(r_vulkan_state->logical_device.h, buffer->memory, 0, buffer->size, 0, &buffer->mapped));
      }

      // TODO(k): just remember to free this
      for(U64 j = 0; j < R_MAX_GEO2D_PASS; j++)
      {
        R_Vulkan_Buffer *buffer = &ret->frames[i].inst_buffer_mesh2d[j];
        VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        create_info.size = sizeof(R_Mesh2DInst)*R_MAX_MESH2D_INSTANCES;
        create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_Assert(vkCreateBuffer(r_vulkan_state->logical_device.h, &create_info, NULL, &buffer->h));

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->logical_device.h, buffer->h, &mem_requirements);

        buffer->kind = R_ResourceKind_Static;
        buffer->size = mem_requirements.size;
        buffer->cap  = mem_requirements.size;

        VkMemoryAllocateInfo alloc_info = {0};
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &buffer->memory));
        VK_Assert(vkBindBufferMemory(r_vulkan_state->logical_device.h, buffer->h, buffer->memory, 0));
        VK_Assert(vkMapMemory(r_vulkan_state->logical_device.h, buffer->memory, 0, buffer->size, 0, &buffer->mapped));
      }

      // TODO(k): just remember to free this
      // create inst buffer for rect
      for(U64 j = 0; j < R_MAX_GEO3D_PASS; j++)
      {
        R_Vulkan_Buffer *buffer = &ret->frames[i].inst_buffer_mesh3d[j];
        VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        create_info.size = sizeof(R_Mesh3DInst)*R_MAX_MESH3D_INSTANCES;
        create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_Assert(vkCreateBuffer(r_vulkan_state->logical_device.h, &create_info, NULL, &buffer->h));

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->logical_device.h, buffer->h, &mem_requirements);

        buffer->kind = R_ResourceKind_Static;
        buffer->size = mem_requirements.size;
        buffer->cap  = mem_requirements.size;

        VkMemoryAllocateInfo alloc_info = {0};
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &buffer->memory));
        VK_Assert(vkBindBufferMemory(r_vulkan_state->logical_device.h, buffer->h, buffer->memory, 0));
        VK_Assert(vkMapMemory(r_vulkan_state->logical_device.h, buffer->memory, 0, buffer->size, 0, &buffer->mapped));
      }
    }
  }

  ////////////////////////////////
  // ~ Pipeline creation

  {
    // rect
    ret->pipelines.rect = r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind_GFX_Rect, R_GeoTopologyKind_TriangleStrip, R_GeoPolygonKind_Fill, render_targets->swapchain.format, 0);

    // blur

    // noise
    ret->pipelines.noise = r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind_GFX_Noise, R_GeoTopologyKind_TriangleStrip, R_GeoPolygonKind_Fill, render_targets->swapchain.format, 0);

    // edge
    ret->pipelines.edge = r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind_GFX_Edge, R_GeoTopologyKind_TriangleStrip, R_GeoPolygonKind_Fill, render_targets->swapchain.format, 0);

    // crt
    ret->pipelines.crt = r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind_GFX_Crt, R_GeoTopologyKind_TriangleStrip, R_GeoPolygonKind_Fill, render_targets->swapchain.format, 0);

    // geo2d
    for(U64 i = 0; i < R_GeoTopologyKind_COUNT; i++)
    {
      for(U64 j = 0; j < R_GeoPolygonKind_COUNT; j++)
      {
        ret->pipelines.geo2d.forward[i*R_GeoPolygonKind_COUNT + j] = r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind_GFX_Geo2D_Forward, i, j, render_targets->swapchain.format, 0);
      }
    }
    ret->pipelines.geo2d.composite = r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind_GFX_Geo2D_Composite, R_GeoTopologyKind_TriangleStrip, R_GeoPolygonKind_Fill, render_targets->swapchain.format, 0);

    // geo3d
    ret->pipelines.geo3d.tile_frustum = r_vulkan_cmp_pipeline(R_Vulkan_PipelineKind_CMP_Geo3D_TileFrustum);
    ret->pipelines.geo3d.light_culling = r_vulkan_cmp_pipeline(R_Vulkan_PipelineKind_CMP_Geo3D_LightCulling);
    for(U64 i = 0; i < R_GeoTopologyKind_COUNT; i++)
    {
      for(U64 j = 0; j < R_GeoPolygonKind_COUNT; j++)
      {
        ret->pipelines.geo3d.z_pre[i*R_GeoPolygonKind_COUNT + j] = r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind_GFX_Geo3D_ZPre, i, j, render_targets->swapchain.format, 0);
        ret->pipelines.geo3d.debug[i*R_GeoPolygonKind_COUNT + j] = r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind_GFX_Geo3D_Debug, i, j, render_targets->swapchain.format, 0);
        ret->pipelines.geo3d.forward[i*R_GeoPolygonKind_COUNT + j] = r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind_GFX_Geo3D_Forward, i, j, render_targets->swapchain.format, 0);
      }
    }
    ret->pipelines.geo3d.composite = r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind_GFX_Geo3D_Composite, R_GeoTopologyKind_TriangleStrip, R_GeoPolygonKind_Fill, render_targets->swapchain.format, 0);

    // finalize
    ret->pipelines.finalize = r_vulkan_gfx_pipeline(R_Vulkan_PipelineKind_GFX_Finalize, R_GeoTopologyKind_TriangleStrip, R_GeoPolygonKind_Fill, render_targets->swapchain.format, 0);
  }

  // fill & return
  ret->os_wnd = os_wnd;
  ret->render_targets = render_targets;
  ret->surface = surface;
  ProfEnd();
  return r_vulkan_handle_from_window(ret);
}

r_hook void
r_window_unequip(OS_Handle window, R_Handle window_equip)
{
  NotImplemented;
}

//- rjf: textures

r_hook R_Handle
r_tex2d_alloc(R_ResourceKind kind, R_Tex2DSampleKind sample_kind, Vec2S32 size, R_Tex2DFormat format, void *data)
{
  ProfBeginFunction();

  // alloc
  R_Vulkan_Tex2D *texture = 0;
  {
    texture = r_vulkan_state->first_free_tex2d;
    if(texture == 0)
    {
      texture = push_array(r_vulkan_state->arena, R_Vulkan_Tex2D, 1);
    }
    else
    {
      U64 gen = texture->generation;
      SLLStackPop(r_vulkan_state->first_free_tex2d);
      MemoryZeroStruct(texture);
      texture->generation = gen;
    }
  }
  texture->image.extent.width  = size.x;
  texture->image.extent.height = size.y;
  texture->format = format;

  // decide image bytes size
  VkDeviceSize vk_image_size = size.x * size.y;
  VkFormat vk_image_format = 0;
  switch(format)
  {
    case R_Tex2DFormat_R8:    {vk_image_format = VK_FORMAT_R8_UNORM;}break;
    case R_Tex2DFormat_RGBA8: {vk_image_format = VK_FORMAT_R8G8B8A8_SRGB; vk_image_size *= 4;}break;
    default:                  {InvalidPath;}break;
  }
  texture->image.format = vk_image_format;
  texture->image.gpu_layout = VK_IMAGE_LAYOUT_UNDEFINED;

  ////////////////////////////////
  //~ Create the gpu device local buffer

  ProfScope("create gpu local buffer")
  {

    // Although we could set up the shader to access the pixel values in the buffer, it's better to use image objects in Vulkan for this purpose
    // Image objects will make it easier and faster to retrieve colors by allowing us to use 2D coordinates, for one
    // Pixels within an image object are known as texels and we'll use that name from this point on
    VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    // Tells Vulkan with what kind of coordinate system the texels in the image are going to be addressed
    // It's possbiel to create 1D, 2D and 3D images
    // One dimensional images can be used to store an array of data or gradient
    // Two dimensional images are mainly used for textures, and three dimensional images can be used to store voxel volumes, for example
    // The extent field specifies the dimensions of the image, basically how many texels there are on each axis
    // That's why depth must be 1 instead of 0
    create_info.imageType     = VK_IMAGE_TYPE_2D;
    create_info.extent.width  = size.x;
    create_info.extent.height = size.y;
    create_info.extent.depth  = 1;
    create_info.mipLevels     = 1;
    create_info.arrayLayers   = 1;
    // Vulkan supports many possible image formats, but we should use the same format for the texels as the pixels in the buffer, otherwise the copy operations will fail
    // It is possible that the VK_FORMAT_R8G8B8A8_SRGB is not supported by the graphics hardware
    // You should have a list of acceptable alternatives and go with the best one that is supported
    // However, support for this particular for this particular format is so widespread that we'll skip this step
    // Using different formats would also require annoying conversions
    // .format = VK_FORMAT_R8G8B8A8_SRGB,
    create_info.format = vk_image_format;
    // The tiling field can have one of two values:
    // 1.  VK_IMAGE_TILING_LINEAR: texels are laid out in row-major order like our pixels array
    // 2. VK_IMAGE_TILING_OPTIMAL: texels are laid out in an implementation defined order for optimal access 
    // Unlike the layout of an image, the tiling mode cannot be changed at a later time 
    // If you want to be able to directly access texels in the memory of the image, then you must use VK_IMAGE_TILING_LINEAR layout
    // We will be using a staging buffer instead of staging image, so this won't be necessary, we will be using VK_IMAGE_TILING_OPTIMAL for efficient access from the shader
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    // There are only two possbile values for the initialLayout of an image
    // 1.      VK_IMAGE_LAYOUT_UNDEFINED: not usable by the GPU and the very first transition will discard the texels
    // 2. VK_IMAGE_LAYOUT_PREINITIALIZED: not usable by the GPU, but the first transition will preserve the texels
    // There are few situations where it is necessary for the texels to be preserved during the first transition
    // One example, however, would be if you wanted to use an image as staging image in combination with VK_IMAGE_TILING_LINEAR layout
    // In that case, you'd want to upload the texel data to it and then transition the image to be transfer source without losing the data
    // In out case, however, we're first going to transition the image to be transfer destination and then copy texel data to it from a buffer object
    // So we don't need this property and can safely use VK_IMAGE_LAYOUT_UNDEFINED
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // The image is going to be used as destination for the buffer copy
    // We also want to be able to access the image from the shader to color our mesh
    create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    // The image will only be used by one queue family: the one that supports graphics (and therefor also) transfer operations
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // The samples flag is related to multisampling
    // This is only relevant for images that will be used as attachments, so stick to one sample
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    // There are some optional flags for images that are related to sparse images
    // Sparse images are images where only certain regions are actually backed by memory
    // If you were using a 3D texture for a voxel terrain, for example, then you could use this avoid allocating memory to storage large volumes of "air" values
    create_info.flags = 0;
    VK_Assert(vkCreateImage(r_vulkan_state->logical_device.h, &create_info, NULL, &texture->image.h));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(r_vulkan_state->logical_device.h, texture->image.h, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

    VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &texture->image.memory));
    VK_Assert(vkBindImageMemory(r_vulkan_state->logical_device.h, texture->image.h, texture->image.memory, 0));
  }

  ////////////////////////////////
  //~ Create image view

  VkImageViewCreateInfo ivci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  ivci.image                           = texture->image.h;
  ivci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  ivci.format                          = vk_image_format;
  ivci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  ivci.subresourceRange.baseMipLevel   = 0;
  ivci.subresourceRange.levelCount     = 1;
  ivci.subresourceRange.baseArrayLayer = 0;
  ivci.subresourceRange.layerCount     = 1;
  VK_Assert(vkCreateImageView(r_vulkan_state->logical_device.h, &ivci, NULL, &texture->image.view));

  ////////////////////////////////
  //~ Has initial data? -> copy

  if(data != 0)
  {
    Vec3S32 offset = {0,0,0};
    Vec3S32 extent = {size.x, size.y, 1};
    r_vulkan_stage_copy_image(data, vk_image_size, &texture->image, offset, extent);
  }

  // TODO(k): All of the helper functions that submit commands so far have been set up to execute synchronously by waiting for the queue to become idle
  // For practical applications it is recommended to combine these operations in a single command buffer and execute them asynchronously for higher throughput
  //     especially the transitions and copy in the create_texture_image function
  // We could experiment with this by creating a setup_command_buffer that the helper functions record commands into, and add a finish_setup_commands to execute the 
  //     the commands that have been recorded so far

  ////////////////////////////////
  //~ Create sampler

  // TODO(k): we could create two descriptor set for two types of sampler
  VkSampler *sampler = &r_vulkan_state->samplers[sample_kind];
  r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind_Tex2D, 1, 64, NULL, &texture->image.view, sampler, &texture->desc_set);

  R_Handle ret = r_vulkan_handle_from_tex2d(texture);
  ProfEnd();
  return ret;
}

r_hook void
r_tex2d_release(R_Handle handle)
{
  R_Vulkan_Tex2D *tex2d = r_vulkan_tex2d_from_handle(handle);

  // view->image->ds->memory
  vkDestroyImageView(r_vulkan_state->logical_device.h, tex2d->image.view, NULL);
  vkDestroyImage(r_vulkan_state->logical_device.h, tex2d->image.h, NULL);
  r_vulkan_descriptor_set_destroy(&tex2d->desc_set);
  vkFreeMemory(r_vulkan_state->logical_device.h, tex2d->image.memory, NULL);

  SLLStackPush(r_vulkan_state->first_free_tex2d, tex2d);
  tex2d->generation++;
}

r_hook R_ResourceKind
r_kind_from_tex2d(R_Handle texture)
{
  NotImplemented;
}

r_hook Vec2S32
r_size_from_tex2d(R_Handle texture)
{
  NotImplemented;
}

r_hook R_Tex2DFormat
r_format_from_tex2d(R_Handle texture)
{
  NotImplemented;
}

r_hook void
r_fill_tex2d_region(R_Handle handle, Rng2S32 subrect, void *data)
{
  R_Vulkan_Tex2D *texture = r_vulkan_tex2d_from_handle(handle);

  // calc bytes size
  Vec2S32 dim = {subrect.x1-subrect.x0, subrect.y1-subrect.y0};
  U64 size = dim.x*dim.y;
  switch(texture->format)
  {
    default:break;
    case R_Tex2DFormat_RGBA8:{size*=4;}break;
  }

  Vec3S32 offset = {subrect.x0, subrect.y0, 0};
  Vec3S32 extent = {dim.x, dim.y, 1};
  r_vulkan_stage_copy_image(data, size, &texture->image, offset, extent);
}

//- rjf: buffers

r_hook R_Handle
r_buffer_alloc(R_ResourceKind kind, U64 size, void *data, U64 data_size)
{
  R_Vulkan_Buffer *ret = 0;
  ret = r_vulkan_state->first_free_buffer;
  if(ret == 0)
  {
    ret = push_array(r_vulkan_state->arena, R_Vulkan_Buffer, 1);
  }
  else
  {
    U64 gen = ret->generation;
    SLLStackPop(r_vulkan_state->first_free_buffer);
    MemoryZeroStruct(ret);
    ret->generation = gen;
  }

  // Fill basics
  ret->kind = kind;
  ret->size = data_size;
  ret->cap = size;

  // It should be noted that in a real world application, you're not supposed to actually call vkAllocateMemory for every individual buffer
  // The maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount physical device limit
  // which may be as low as 4096 even on high end hardward like an NVIDIA GTX1080
  // The right way to allocate memory for a large number of objects at the same time is to create a custom allocator that splits
  //      up a single allocation among many different objects by using the offset parameters that we've seen in many functions
  // We can either implement such an allocator ourself, or use the VulkanMemoryAllocator library provided by the GPUOpen initiative

  switch(kind)
  {
    case R_ResourceKind_Static:
    {
      // Create staging buffer
      VkBuffer staging_buffer;
      VkDeviceMemory staging_memory;
      {
        VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create_info.size        = size;
        create_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_Assert(vkCreateBuffer(r_vulkan_state->logical_device.h, &create_info, NULL, &staging_buffer));

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->logical_device.h, staging_buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &staging_memory));
        VK_Assert(vkBindBufferMemory(r_vulkan_state->logical_device.h, staging_buffer, staging_memory, 0));

        AssertAlways(data != 0 && data_size > 0);
        void *mapped;
        VK_Assert(vkMapMemory(r_vulkan_state->logical_device.h, staging_memory, 0, mem_requirements.size, 0, &mapped));
        MemoryCopy(mapped, data, data_size);
        vkUnmapMemory(r_vulkan_state->logical_device.h, staging_memory);
      }

      VkBuffer buffer;
      VkDeviceMemory buffer_memory;
      // Create buffer <GPU Device Local> 
      {
        VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create_info.size        = size;
        create_info.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_Assert(vkCreateBuffer(r_vulkan_state->logical_device.h, &create_info, NULL, &buffer));

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->logical_device.h, buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &buffer_memory));
        VK_Assert(vkBindBufferMemory(r_vulkan_state->logical_device.h, buffer, buffer_memory, 0));
      }

      // Copy buffer
      VkCommandBuffer cmd = r_vulkan_state->oneshot_cmd_buf;
      CmdScope(cmd)
      {
        VkBufferCopy copy_region = {
          .srcOffset = 0, // Optional
          .dstOffset = 0, // Optional
          .size      = data_size,
        };

        // It is not possible to specify VK_WHOLE_SIZE here unlike the vkMapMemory command
        vkCmdCopyBuffer(cmd, staging_buffer, buffer, 1, &copy_region);

        // TODO(k): we should bind a barrier here
      }

      // TODO(k) it's not effecient to use vkQueueWaitIdle, maybe we could add a to_free queue, and use some sync primitive to do this
      // index Semaphore could be a good idea
      VK_Assert(vkQueueWaitIdle(r_vulkan_state->logical_device.gfx_queue));

      // Free staging buffer and memory
      vkDestroyBuffer(r_vulkan_state->logical_device.h, staging_buffer, NULL);
      vkFreeMemory(r_vulkan_state->logical_device.h, staging_memory, NULL);

      // fill result
      ret->h = buffer; 
      ret->memory = buffer_memory;
    }break;
    case R_ResourceKind_Dynamic:
    {
      // double buffer setup (host_visiable/staging+device_local)
      VkBuffer staging_buffer;
      VkDeviceMemory staging_memory;
      void *mapped;
      {
        VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create_info.size        = size;
        create_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_Assert(vkCreateBuffer(r_vulkan_state->logical_device.h, &create_info, NULL, &staging_buffer));

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->logical_device.h, staging_buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &staging_memory));
        VK_Assert(vkBindBufferMemory(r_vulkan_state->logical_device.h, staging_buffer, staging_memory, 0));

        VK_Assert(vkMapMemory(r_vulkan_state->logical_device.h, staging_memory, 0, mem_requirements.size, 0, &mapped));
        if(data != 0 && data_size >0)
        {
          MemoryCopy(mapped, data, size);
        }
      }

      // create buffer
      VkBuffer buffer;
      VkDeviceMemory buffer_memory;
      // Create buffer <GPU Device Local> 
      {
        VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create_info.size        = size;
        create_info.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_Assert(vkCreateBuffer(r_vulkan_state->logical_device.h, &create_info, NULL, &buffer));

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->logical_device.h, buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &buffer_memory));
        VK_Assert(vkBindBufferMemory(r_vulkan_state->logical_device.h, buffer, buffer_memory, 0));

        // Copy buffer if data is provided
        if(data != 0 && data_size > 0)
        {
          VkCommandBuffer cmd = r_vulkan_state->oneshot_cmd_buf;
          CmdScope(cmd)
          {
            VkBufferCopy copy_region = {
              .srcOffset = 0, // Optional
              .dstOffset = 0, // Optional
              .size      = data_size,
            };

            // It is not possible to specify VK_WHOLE_SIZE here unlike the vkMapMemory command
            vkCmdCopyBuffer(cmd, staging_buffer, buffer, 1, &copy_region);

            // create a buffer barrier
            VkBufferMemoryBarrier barrier = 
            {
              .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
              // If you are using the barrier to transfer queue family ownership, then these two fields should be the indices of the queue families
              // They must be set to VK_QUEUE_FAMILY_IGNORED if you don't want to do this (not the default value)
              .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
              .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
              .srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
              .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
              .buffer = buffer,
              .offset = 0,
              .size = data_size,
            };
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,0, NULL, 1, &barrier, 0, 0);
          }
        }
      }

      // fill result
      ret->h = buffer; 
      ret->memory = buffer_memory;
      ret->staging = staging_buffer;
      ret->staging_memory = staging_memory;
      ret->mapped = mapped;
    }break;
    case R_ResourceKind_Stream:
    {
      // NOTE(k): don't make too much difference in here after profling double buffer and single host visiable buffer
#if 1
      VkBuffer buffer;
      VkDeviceMemory buffer_memory;
      void *mapped;
      {
        VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create_info.size        = size;
        create_info.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_Assert(vkCreateBuffer(r_vulkan_state->logical_device.h, &create_info, NULL, &buffer));

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->logical_device.h, buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        // TODO(k): some device provide 256MB host_visiable and device local memory, find out if we can make use of that conditionally
        // VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->logical_device.h, &alloc_info, NULL, &buffer_memory));
        VK_Assert(vkBindBufferMemory(r_vulkan_state->logical_device.h, buffer, buffer_memory, 0));
        VK_Assert(vkMapMemory(r_vulkan_state->logical_device.h, buffer_memory, 0, size, 0, &mapped));
      }

      if(data != 0 && data_size > 0)
      {
        MemoryCopy(mapped, data, data_size);
      }

      // fill result
      ret->h = buffer;
      ret->memory = buffer_memory;
      ret->mapped = mapped;
#else
      // double buffer setup (host_visiable/staging+device_local)
      VkBuffer staging_buffer;
      VkDeviceMemory staging_memory;
      void *mapped;
      {
        VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create_info.size        = size;
        create_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_Assert(vkCreateBuffer(r_vulkan_state->device.h, &create_info, NULL, &staging_buffer));

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->device.h, staging_buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &staging_memory));
        VK_Assert(vkBindBufferMemory(r_vulkan_state->device.h, staging_buffer, staging_memory, 0));

        VK_Assert(vkMapMemory(r_vulkan_state->device.h, staging_memory, 0, mem_requirements.size, 0, &mapped));
        if(data != 0 && data_size >0)
        {
          MemoryCopy(mapped, data, size);
        }
      }

      // create buffer
      VkBuffer buffer;
      VkDeviceMemory buffer_memory;
      // Create buffer <GPU Device Local> 
      {
        VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create_info.size        = size;
        create_info.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_Assert(vkCreateBuffer(r_vulkan_state->device.h, &create_info, NULL, &buffer));

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->device.h, buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &buffer_memory));
        VK_Assert(vkBindBufferMemory(r_vulkan_state->device.h, buffer, buffer_memory, 0));

        // Copy buffer if data is provided
        if(data != 0 && data_size > 0)
        {
          VkCommandBuffer cmd = r_vulkan_state->oneshot_cmd_buf;
          CmdScope(cmd)
          {
            VkBufferCopy copy_region = {
              .srcOffset = 0, // Optional
              .dstOffset = 0, // Optional
              .size      = data_size,
            };

            // It is not possible to specify VK_WHOLE_SIZE here unlike the vkMapMemory command
            vkCmdCopyBuffer(cmd, staging_buffer, buffer, 1, &copy_region);

            // create a buffer barrier
            VkBufferMemoryBarrier barrier = 
            {
              .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
              // If you are using the barrier to transfer queue family ownership, then these two fields should be the indices of the queue families
              // They must be set to VK_QUEUE_FAMILY_IGNORED if you don't want to do this (not the default value)
              .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
              .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
              .srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
              .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
              .buffer = buffer,
              .offset = 0,
              .size = data_size,
            };
            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                0,0, NULL, 1, &barrier, 0, 0);
          }
        }
      }

      // fill result
      ret->h = buffer; 
      ret->memory = buffer_memory;
      ret->staging = staging_buffer;
      ret->stating_memory = staging_memory;
      ret->mapped = mapped;
#endif
    }break;
    default: {InvalidPath;}break;
  }

  return r_vulkan_handle_from_buffer(ret);
}

// NOTE(k): this function can only be called within a frame boundary
r_hook void
r_buffer_copy(R_Handle handle, void *data, U64 size)
{
  R_Vulkan_Buffer *buffer = r_vulkan_buffer_from_handle(handle);
  buffer->size = size;

  VkCommandBuffer cmd = r_vulkan_top_cmd();
  switch(buffer->kind)
  {
    case R_ResourceKind_Dynamic:
    // case R_ResourceKind_Stream:
    {
      MemoryCopy(buffer->mapped, data, size);

      // copy staging buffer to device local buffer
      // NOTE(k): we can't use oneshot_cmd_buf, since we didn't wait for the oneshot cmd buffer, maybe we should use frame cmd buffer
      // VkCommandBuffer cmd = r_vulkan_state->oneshot_cmd_buf;
      VkBufferCopy copy_region = {
        .srcOffset = 0, // Optional
        .dstOffset = 0, // Optional
        .size      = size,
      };

      // It is not possible to specify VK_WHOLE_SIZE here unlike the vkMapMemory command
      vkCmdCopyBuffer(cmd, buffer->staging, buffer->h, 1, &copy_region);

      // create a buffer barrier
      VkBufferMemoryBarrier barrier = 
      {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        // If you are using the barrier to transfer queue family ownership, then these two fields should be the indices of the queue families
        // They must be set to VK_QUEUE_FAMILY_IGNORED if you don't want to do this (not the default value)
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
        .buffer = buffer->h,
        .offset = 0,
        .size = size,
      };
      vkCmdPipelineBarrier(cmd,
          VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
          0,0, NULL, 1, &barrier, 0, 0);
    }break;
    case R_ResourceKind_Stream:
    {
      MemoryCopy(buffer->mapped, data, size);
    }break;
    default: {InvalidPath;}break;
  }
}

r_hook void
r_buffer_release(R_Handle handle)
{
  R_Vulkan_Buffer *buffer = r_vulkan_buffer_from_handle(handle);

  vkDestroyBuffer(r_vulkan_state->logical_device.h, buffer->h, NULL);
  vkFreeMemory(r_vulkan_state->logical_device.h, buffer->memory, NULL);

  switch(buffer->kind)
  {
    case R_ResourceKind_Dynamic:
    {
      vkUnmapMemory(r_vulkan_state->logical_device.h, buffer->memory);
      vkDestroyBuffer(r_vulkan_state->logical_device.h, buffer->staging, NULL);
      vkFreeMemory(r_vulkan_state->logical_device.h, buffer->staging_memory, NULL);
    }break;
    case R_ResourceKind_Static:
    case R_ResourceKind_Stream:
    {
      // noop
    }break;
    default:{InvalidPath;}break;
  }

  SLLStackPush(r_vulkan_state->first_free_buffer, buffer);
  // NOTE(k): we should increase generation in release instead of alloc
  buffer->generation++;
}

//- rjf: frame markers

r_hook void
r_begin_frame(void)
{
  // submit any stage works to gfx queue
  if(r_vulkan_state->stage.last_touch_frame_index == r_vulkan_state->frame_index)
  {
    r_vulkan_stage_end();
  }
}

r_hook void
r_end_frame(void)
{
  r_vulkan_state->frame_index++;
  arena_clear(r_vulkan_state->frame_arena);
}

r_hook void
r_window_begin_frame(OS_Handle os_wnd, R_Handle window_equip)
{
  ProfBeginFunction();
  R_Vulkan_Window *wnd = r_vulkan_window_from_handle(window_equip);
  R_Vulkan_Frame *frame = &wnd->frames[wnd->curr_frame_idx];
  R_Vulkan_LogicalDevice *device = &r_vulkan_state->logical_device;

  // Wait until the previous frame has finished
  // This function takes an array of fences and waits on the host for either any or all of the fences to be signaled before returning
  // The VK_TRUE we pass here indicates that we want to wait for all fences
  // This function also has a timeout parameter that we set to the maxium value of 64 bit unsigned integer, which effectively disables the timeout
  VK_Assert(vkWaitForFences(r_vulkan_state->logical_device.h, 1, &frame->inflt_fence, VK_TRUE, UINT64_MAX));

  // Vulkan will usually just tell us that the swapchain is no longer adequate during presentation
  // The vkAcquireNextImageKHR and vkQueuePresentKHR functions can return the following special values to indicate this
  // 1. VK_ERROR_OUT_OF_DATE_KHR: the swap chain has become incompatible with the surface and can no longer be used for rendering
  //    Usually happens after a window resize
  // 2. VK_SUBOPTIMAL_KHR: the swap chain can still be used to successfully present to the surface, but the surface properties are no longer matched exactly
  VkResult ret;
  while(1)
  {
    // Acquire image from swapchain
    ret = vkAcquireNextImageKHR(device->h, wnd->render_targets->swapchain.h, UINT64_MAX, frame->img_acq_sem, VK_NULL_HANDLE, &frame->img_idx);
    // TODO(XXX): this one don't trigger somehow, find out why
    if(ret == VK_ERROR_OUT_OF_DATE_KHR || ret == VK_SUBOPTIMAL_KHR)
    {
      r_vulkan_window_resize(wnd);
      continue;
    }
    AssertAlways(ret == VK_SUCCESS);
    break;
  }

  // TODO(k): there is one issue though, we are only keep the bag and rendpass_grp references, and it's not good enough
  // if window is resizing, bag and rendpass_grp between frames won't be synced, the count of to_free_bag will keeping increasing, and the gpu memory will run out
  // we could use some kind of generation or index to check if a bag or rendpass_grp could be cleared

  R_Vulkan_RenderTargets *targets = frame->render_targets_ref;
  if(targets != 0)
  {
    targets->rc--;
    Assert(targets->rc >= 0);
  }

  ////////////////////////////////
  //~ Destroy deprecated render targets

  for(R_Vulkan_RenderTargets *t = r_vulkan_state->first_to_free_render_targets; t != 0;)
  {
    R_Vulkan_RenderTargets *next = t->next;
    if(t->rc == 0 && (r_vulkan_state->frame_index-t->deprecated_at_frame) > R_VULKAN_MAX_FRAMES_IN_FLIGHT)
    {
      SLLQueuePop(r_vulkan_state->first_to_free_render_targets, r_vulkan_state->last_to_free_render_targets);
      r_vulkan_render_targets_destroy(t);
      SLLStackPush(r_vulkan_state->first_free_render_targets, t);
    }
    else
    {
      break;
    }
    t = next;
  }

  // reset frame fence & command buffer
  VK_Assert(vkResetFences(device->h, 1, &frame->inflt_fence));
  VK_Assert(vkResetCommandBuffer(frame->cmd_buf, 0));

  // start command recrod
  r_vulkan_cmd_begin(frame->cmd_buf);

  ////////////////////////////////
  //~ Clear framebuffers (stage color & stage id)

  // NOTE(k): we can't clear swapchain image using this, since swap image don't guarantee to have usage of VK_IMAGE_USAGE_TRANSFER_DST
  // https://github.com/GameTechDev/IntroductionToVulkan/issues/4
  VkImage stage_color_image = wnd->render_targets->stage_color_image.h;
  {
    r_vulkan_image_transition(frame->cmd_buf, stage_color_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    VkClearColorValue clear_clr = {0,0,0,0};
    VkImageSubresourceRange subrange;
    subrange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subrange.baseMipLevel   = 0;
    subrange.levelCount     = 1;
    subrange.baseArrayLayer = 0;
    subrange.layerCount     = 1;
    vkCmdClearColorImage(frame->cmd_buf, stage_color_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_clr, 1, &subrange);
    r_vulkan_image_transition(frame->cmd_buf, stage_color_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
  }
  VkImage id_color_image = wnd->render_targets->stage_id_image.h;
  {
    r_vulkan_image_transition(frame->cmd_buf, id_color_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_PIPELINE_STAGE_HOST_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    VkClearColorValue clear_clr = {0,0,0,0};
    VkImageSubresourceRange subrange;
    subrange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subrange.baseMipLevel   = 0;
    subrange.levelCount     = 1;
    subrange.baseArrayLayer = 0;
    subrange.layerCount     = 1;
    vkCmdClearColorImage(frame->cmd_buf, id_color_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_clr, 1, &subrange);
    r_vulkan_image_transition(frame->cmd_buf, id_color_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
  }

  // reset per-frame index
  wnd->ui_group_index = 0;
  wnd->ui_pass_index = 0;
  wnd->geo2d_pass_index = 0;
  wnd->geo3d_pass_index = 0;

  r_vulkan_push_cmd(frame->cmd_buf);
  ProfEnd();
}

r_hook Vec2F32
r_window_end_frame(OS_Handle window, R_Handle window_equip, Vec2F32 mouse_ptr)
{
  ProfBeginFunction();

  // unpack params
  R_Vulkan_Window *wnd = r_vulkan_window_from_handle(window_equip);
  R_Vulkan_Frame *frame = &wnd->frames[wnd->curr_frame_idx];
  VkCommandBuffer cmd_buf = frame->cmd_buf;
  R_Vulkan_Swapchain *swapchain = &wnd->render_targets->swapchain;

  // NOTE(k): stage_id_cpu buffer could be lagged by several frames
  // get point entity id
  void *ids = wnd->render_targets->stage_id_cpu.mapped;
  // U64 id = ((U64 *)ids)[(U64)(ptr.y*w+ptr.x)];
  Vec4F32 id = ((Vec4F32 *)ids)[0];

  // increase render_targets ref counter
  frame->render_targets_ref = wnd->render_targets;
  frame->render_targets_ref->rc++;

  ////////////////////////////////
  //~ Copy stage id image to stage id buffer (cpu)

  {
    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    // NOTE(k): weird here, it's alreay in src optimal, but we need to wait for the writing
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // Not changing queue families
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = wnd->render_targets->stage_id_image.h;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Prior access
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT; // Next access

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    vkCmdPipelineBarrier(cmd_buf,
                         srcStage, dstStage, // Source and destination pipeline stages
                         0,                  // No dependency flags
                         0, NULL,            // Memory barriers
                         0, NULL,            // Buffer memory barriers
                         1, &barrier);       // Image memory barriers

    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    Vec2F32 range = {mouse_ptr.x, mouse_ptr.y};
    range.x = Clamp(0,mouse_ptr.x,wnd->render_targets->stage_id_image.extent.width-1);
    range.y = Clamp(0,mouse_ptr.y,wnd->render_targets->stage_id_image.extent.height-1);
    region.imageOffset = (VkOffset3D){range.x,range.y,0};
    // region.imageExtent = (VkExtent3D){wnd->bag->geo3d_id_image.extent.width, wnd->bag->geo3d_id_image.extent.height, 1};
    region.imageExtent = (VkExtent3D){1, 1, 1};
    vkCmdCopyImageToBuffer(cmd_buf, wnd->render_targets->stage_id_image.h, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, wnd->render_targets->stage_id_cpu.h, 1, &region);
  }

  ////////////////////////////////
  //~ Copy stage buffer to swapchain image for present

  {
    VkClearValue clear_color = {0};
    clear_color.color = (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 0.0f }}; /* black with 100% opacity */
    VkRenderingAttachmentInfo color_attachment_info = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    color_attachment_info.imageView = swapchain->image_views[frame->img_idx];
    color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_info.clearValue = clear_color;

    VkRenderingInfo render_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
    render_info.renderArea.offset = (VkOffset2D){0, 0};
    render_info.renderArea.extent = wnd->render_targets->stage_color_image.extent;
    render_info.layerCount = 1;
    render_info.colorAttachmentCount = 1;
    render_info.pColorAttachments = &color_attachment_info;

    // transition stage color image layout to shader read optimal
    r_vulkan_image_transition(cmd_buf, wnd->render_targets->stage_color_image.h,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    r_vulkan_image_transition(frame->cmd_buf, swapchain->images[frame->img_idx], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    // begin
    vkCmdBeginRendering(cmd_buf, &render_info);

    // bind pipeline
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.finalize.h);

    // viewport and scissor
    VkViewport viewport = {0};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = swapchain->extent.width;
    viewport.height   = swapchain->extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    // setup scissor rect
    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent = swapchain->extent;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    // bind stage color descriptor
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.finalize.layout, 0, 1, &wnd->render_targets->stage_color_ds.h, 0, NULL);

    // draw the quad
    vkCmdDraw(cmd_buf, 4, 1, 0, 0);

    vkCmdEndRendering(cmd_buf);
  }
  r_vulkan_image_transition(frame->cmd_buf, swapchain->images[frame->img_idx], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

  ////////////////////////////////
  //~ Submit gfx command buffer

  VK_Assert(vkEndCommandBuffer(cmd_buf));
  VkSemaphore wait_sems[1] = { frame->img_acq_sem };
  VkPipelineStageFlags wait_stages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  VkSemaphore signal_sems[1] = { swapchain->submit_semaphores[frame->img_idx] };

  // submit
  VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
  submit_info.waitSemaphoreCount   = 1;
  submit_info.pWaitSemaphores      = wait_sems;
  submit_info.pWaitDstStageMask    = wait_stages;
  submit_info.commandBufferCount   = 1;
  submit_info.pCommandBuffers      = &cmd_buf;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores    = signal_sems;
  ProfScope("queue submit")
  {
    VK_Assert(vkQueueSubmit(r_vulkan_state->logical_device.gfx_queue, 1, &submit_info, frame->inflt_fence));
  }

  ////////////////////////////////
  //~ Present

  VkSwapchainKHR target_swapchains[1] = { swapchain->h };
  VkPresentInfoKHR prest_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
  prest_info.waitSemaphoreCount = 1;
  prest_info.pWaitSemaphores = signal_sems;
  prest_info.swapchainCount = 1;
  prest_info.pSwapchains = target_swapchains;
  prest_info.pImageIndices = &frame->img_idx;
  // It allows you to specify an array of VkResult values to check for every individual swapchain if presentation was successful
  // It's not necessary if you're only using a single swapchain, because you can simply use the return value of the present function
  prest_info.pResults = NULL; // Optional

  VkResult prest_ret = 0;
  ProfScope("queue present")
  {
    prest_ret = vkQueuePresentKHR(r_vulkan_state->logical_device.prest_queue, &prest_info);
  }

  // NOTE(k): It is important to only check window_resized here to ensure that the job-done semaphores are in a consistent state
  //          otherwise a signaled semaphore may never be properly waited upon
  if(prest_ret == VK_ERROR_OUT_OF_DATE_KHR || prest_ret == VK_SUBOPTIMAL_KHR)
  {
    r_vulkan_window_resize(wnd);
  } 
  else { AssertAlways(prest_ret == VK_SUCCESS); }

  // bump window frame index
  wnd->curr_frame_idx = (wnd->curr_frame_idx + 1) % R_VULKAN_MAX_FRAMES_IN_FLIGHT;
  r_vulkan_pop_cmd();
  ProfEnd();
  return v2f32(id.x, id.y);
}

//- rjf: render pass submission

r_hook void
r_window_submit(OS_Handle window, R_Handle window_equip, R_PassList *passes)
{
  ProfBeginFunction();

  R_Vulkan_Window *wnd = r_vulkan_window_from_handle(window_equip);
  R_Vulkan_Frame *frame = &wnd->frames[wnd->curr_frame_idx];
  R_Vulkan_RenderTargets *render_targets = wnd->render_targets;
  VkCommandBuffer cmd_buf = frame->cmd_buf;

  // TODO(k): Build command buffers in parallel and evenly across several threads/cores
  //          to multiple command lists
  //          Recording commands is a CPU intensive operation and no driver threads come to reuse


  // TODO(XXX): remove this local_persist
  local_persist B32 first_submit = 1;
  if(BUILD_DEBUG && first_submit)
  {
    printf("rt stage_color: %p\n", wnd->render_targets->stage_color_image.h);
    printf("rt stage_id: %p\n", wnd->render_targets->stage_id_image.h);
    printf("rt scratch_color: %p\n", wnd->render_targets->scratch_color_image.h);
    printf("rt edge_image: %p\n", wnd->render_targets->edge_image.h);
    printf("rt geo2d_color_image: %p\n", wnd->render_targets->geo2d_color_image.h);

    printf("rt geo3d_color_image: %p\n", wnd->render_targets->geo3d_color_image.h);
    printf("rt geo3d_normal_depth_image: %p\n", wnd->render_targets->geo3d_normal_depth_image.h);
    printf("rt geo3d_depth_image: %p\n", wnd->render_targets->geo3d_depth_image.h);
    printf("rt geo3d_pre_depth_image: %p\n", wnd->render_targets->geo3d_pre_depth_image.h);
    first_submit = 0;
  }

  U64 ui_group_index = wnd->ui_group_index;
  U64 ui_pass_index = wnd->ui_pass_index;
  U64 geo2d_pass_index = wnd->geo2d_pass_index;
  U64 geo3d_pass_index = wnd->geo3d_pass_index;

  // Do passing
  for(R_PassNode *pass_n = passes->first; pass_n != 0; pass_n = pass_n->next)
  {
    R_Pass *pass = &pass_n->v;
    switch(pass->kind)
    {
      case R_PassKind_Rect:
      {
        ProfBegin("ui_pass");
        Assert(ui_pass_index < R_MAX_RECT_PASS);
        VkRenderingAttachmentInfo color_attachment_infos[2] = {0};

        color_attachment_infos[0].sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_infos[0].imageView   = render_targets->stage_color_image.view;
        color_attachment_infos[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        color_attachment_infos[0].loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachment_infos[0].storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

        color_attachment_infos[1].sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_infos[1].imageView   = render_targets->stage_id_image.view;
        color_attachment_infos[1].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        color_attachment_infos[1].loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachment_infos[1].storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo render_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        // The render area defiens where shader loads and stores will take place
        // The pixels outside this region will have undefined values
        render_info.renderArea.offset = (VkOffset2D){0, 0};
        render_info.renderArea.extent = wnd->render_targets->stage_color_image.extent;
        render_info.layerCount = 1;
        render_info.colorAttachmentCount = ArrayCount(color_attachment_infos);
        render_info.pColorAttachments = color_attachment_infos;

        // begin drawing
        vkCmdBeginRendering(cmd_buf, &render_info);

        // Unpack params 
        R_PassParams_Rect *params = pass->params_rect;
        R_BatchGroupRectList *rect_batch_groups = &params->rects;

        // Unpack uniform buffer
        R_Vulkan_UBOBuffer *uniform_buffer = &frame->ubo_buffers[R_Vulkan_UBOTypeKind_Rect];
        // TODO(k): dynamic allocate uniform buffer if needed
        AssertAlways((ui_group_index+rect_batch_groups->count) < R_MAX_RECT_GROUPS);

        // Bind pipeline
        // The second parameter specifies if the pipeline object is a graphics or compute pipelinVK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BITe
        // We've now told Vulkan which operations to execute in the graphcis pipeline and which attachment to use in the fragment shader
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.rect.h);

        // unpack inst buffer
        R_Vulkan_Buffer *inst_buffer = &frame->inst_buffer_rect[ui_pass_index];

        // Draw each group
        // Rects in the same group share the globals
        U64 inst_idx = 0;
        for(R_BatchGroupRectNode *group_n = rect_batch_groups->first; group_n != 0; group_n = group_n->next)
        {
          R_BatchList *batches = &group_n->batches;
          R_BatchGroupRectParams *group_params = &group_n->params;

          // Get & fill instance bufer
          U64 inst_buffer_off = inst_idx*sizeof(R_Rect2DInst);
          for(R_BatchNode *batch = batches->first; batch != 0; batch = batch->next)
          {
            U8 *dst_ptr = (U8*)inst_buffer->mapped + inst_idx*sizeof(R_Rect2DInst);
            MemoryCopy(dst_ptr, batch->v.v, batch->v.byte_count);
            inst_idx += batch->v.byte_count / sizeof(R_Rect2DInst);
          }

          if(inst_idx > R_MAX_RECT_INSTANCES)
          {
            Temp scratch = scratch_begin(0, 0);
            String8 message = push_str8f(scratch.arena, "Too many rects for one frame, %I64u > %i", inst_idx, R_MAX_RECT_INSTANCES);
            os_graphical_message(1, str8_lit("Fatal Error"), message);
            os_abort(1);
            scratch_end(scratch);
          }

          // Bind instance buffer
          vkCmdBindVertexBuffers(cmd_buf, 0, 1, &inst_buffer->h, &(VkDeviceSize){inst_buffer_off});

          // Get texture
          R_Handle tex_handle = group_params->tex;
          if(r_handle_match(tex_handle, r_handle_zero()))
          {
            tex_handle = r_vulkan_state->backup_texture;
          }
          R_Vulkan_Tex2D *texture = r_vulkan_tex2d_from_handle(tex_handle);
          vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.rect.layout, 1, 1, &texture->desc_set.h, 0, NULL);

          // Set up texture sample map matrix based on texture format
          // Vulkan use col-major
          Vec4F32 texture_sample_channel_map[] = {
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1},
          };

          switch(texture->format)
          {
            default: break;
            case R_Tex2DFormat_R8: 
            {
              MemoryZeroStruct(&texture_sample_channel_map);
              // TODO(k): why, shouldn't vulkan use col-major order?
              texture_sample_channel_map[0] = v4f32(1, 1, 1, 1);
              // texture_sample_channel_map[0].x = 1;
              // texture_sample_channel_map[1].x = 1;
              // texture_sample_channel_map[2].x = 1;
              // texture_sample_channel_map[3].x = 1;
            }break;
          }

          ///////////////////////////////////////////////////////////////////////////////
          //~ Bind uniform buffer

          // Upload uniforms
          R_Vulkan_UBO_Rect uniforms = {0};
          uniforms.viewport_size = group_params->viewport;
          uniforms.opacity = 1-group_params->transparency;
          MemoryCopyArray(uniforms.texture_sample_channel_map, &texture_sample_channel_map);
          // TODO(k): don't know if we need it or not
          uniforms.translate;
          uniforms.texture_t2d_size = v2f32(texture->image.extent.width, texture->image.extent.height);
          uniforms.xform[0] = v4f32(group_params->xform.v[0][0], group_params->xform.v[1][0], group_params->xform.v[2][0], 0);
          uniforms.xform[1] = v4f32(group_params->xform.v[0][1], group_params->xform.v[1][1], group_params->xform.v[2][1], 0);
          uniforms.xform[2] = v4f32(group_params->xform.v[0][2], group_params->xform.v[1][2], group_params->xform.v[2][2], 0);
          Vec2F32 xform_2x2_row0 = v2f32(uniforms.xform[0].x, uniforms.xform[0].y);
          Vec2F32 xform_2x2_row1 = v2f32(uniforms.xform[1].x, uniforms.xform[1].y);
          uniforms.xform_scale.x = length_2f32(xform_2x2_row0);
          uniforms.xform_scale.y = length_2f32(xform_2x2_row1);

          U32 uniform_buffer_offset = ui_group_index * uniform_buffer->stride;
          MemoryCopy((U8 *)uniform_buffer->buffer.mapped + uniform_buffer_offset, &uniforms, sizeof(R_Vulkan_UBO_Rect));
          vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.rect.layout, 0, 1, &uniform_buffer->set.h, 1, &uniform_buffer_offset);

          VkViewport viewport = {0};
          viewport.x        = 0.0f;
          viewport.y        = 0.0f;
          viewport.width    = render_targets->stage_color_image.extent.width;
          viewport.height   = render_targets->stage_color_image.extent.height;
          viewport.minDepth = 0.0f;
          viewport.maxDepth = 1.0f;
          vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

          VkRect2D scissor = {0};
          if(group_params->clip.x0 == 0 && group_params->clip.x1 == 0 && group_params->clip.y0 == 0 && group_params->clip.y1 == 0)
          {
            scissor.offset = (VkOffset2D){0,0};
            scissor.extent = render_targets->stage_color_image.extent;
          }
          else if(group_params->clip.x0 > group_params->clip.x1 || group_params->clip.y0 > group_params->clip.y1)
          {
            scissor.offset = (VkOffset2D){0,0};
            scissor.extent = (VkExtent2D){0,0};
          }
          else
          {
            scissor.offset = (VkOffset2D){(U32)group_params->clip.p0.x, (U32)group_params->clip.p0.y};
            scissor.offset.x = Max(scissor.offset.x, 0);
            scissor.offset.y = Max(scissor.offset.y, 0);
            Vec2F32 clip_dim = dim_2f32(group_params->clip);
            scissor.extent = (VkExtent2D){(U32)clip_dim.x, (U32)clip_dim.y};
            Assert(!(clip_dim.x == 0 && clip_dim.y == 0));
          }
          vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

          U64 inst_count = batches->byte_count / batches->bytes_per_inst;
          vkCmdDraw(cmd_buf, 4, inst_count, 0, 0);

          ui_group_index++;
        }
        vkCmdEndRendering(cmd_buf);
        ui_pass_index++;
        ProfEnd();
      }break;
      case R_PassKind_Blur: {NotImplemented;}break;
      case R_PassKind_Noise:
      {
        ProfBegin("noise pass");
        // unpack params
        R_PassParams_Noise *params = pass->params_noise;

        /////////////////////////////////////////////////////////////////////////////////
        // viewport and scissor

        VkViewport viewport = {0};
        Vec2F32 viewport_dim = dim_2f32(params->rect);
        if(viewport_dim.x == 0 || viewport_dim.y == 0)
        {
          viewport.width  = render_targets->scratch_color_image.extent.width;
          viewport.height = render_targets->scratch_color_image.extent.height;
        }
        else
        {
          viewport.x      = params->rect.p0.x;
          viewport.y      = params->rect.p0.y;
          viewport.width  = viewport_dim.x;
          viewport.height = viewport_dim.y;
        }
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

        VkRect2D scissor = {0};
        scissor.offset = (VkOffset2D){0, 0};
        scissor.extent = render_targets->stage_color_image.extent;
        vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

        /////////////////////////////////////////////////////////////////////////////////
        // draw

        r_vulkan_image_transition(frame->cmd_buf, render_targets->stage_color_image.h, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        r_vulkan_image_transition(frame->cmd_buf, render_targets->scratch_color_image.h, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        // TODO(XXX): we are not using params here (clip and rect e.g.)

        VkClearValue clear_colors[1] = {0};
        clear_colors[0].color = (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 0.0f }}; /* black with 100% opacity */
        VkRenderingAttachmentInfo color_attachment_infos[1] = {0};
        color_attachment_infos[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_infos[0].imageView = render_targets->scratch_color_image.view;
        color_attachment_infos[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        color_attachment_infos[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment_infos[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_infos[0].clearValue = clear_colors[0];

        VkRenderingInfo render_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        render_info.renderArea.offset = (VkOffset2D){0, 0};
        render_info.renderArea.extent = wnd->render_targets->scratch_color_image.extent;
        render_info.layerCount = 1;
        render_info.colorAttachmentCount = ArrayCount(color_attachment_infos);
        render_info.pColorAttachments = color_attachment_infos;

        // push constant
        R_Vulkan_PUSH_Noise push = {0};
        push.mouse = v2f32(0,0); /* TODO(XXX): not used */
        push.resolution.x = render_targets->scratch_color_image.extent.width;
        push.resolution.y = render_targets->scratch_color_image.extent.height;
        push.time = params->elapsed_secs*0.5;

        vkCmdPushConstants(cmd_buf, wnd->pipelines.noise.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(R_Vulkan_PUSH_Noise), &push);

        // bind pipeline
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.noise.h);

        // begin drawing
        vkCmdBeginRendering(cmd_buf, &render_info);

        // bind stage color descriptor
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.noise.layout, 0, 1, &render_targets->stage_color_ds.h, 0, NULL);

        vkCmdDraw(cmd_buf, 4, 1, 0, 0);

        // end drawing
        vkCmdEndRendering(cmd_buf);

        r_vulkan_image_transition(cmd_buf, render_targets->scratch_color_image.h, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        r_vulkan_image_transition(cmd_buf, render_targets->stage_color_image.h, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        // copy scratch image to stage image
        {
          VkImage src = render_targets->scratch_color_image.h;
          VkImage dst = render_targets->stage_color_image.h;
          VkImageCopy copy_region = {0};
          copy_region.srcSubresource = (VkImageSubresourceLayers){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
          };
          copy_region.srcOffset = (VkOffset3D){0,0,0};
          copy_region.dstSubresource = (VkImageSubresourceLayers){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
          };
          copy_region.dstOffset = (VkOffset3D){0,0,0};
          copy_region.extent = (VkExtent3D){render_targets->scratch_color_image.extent.width, render_targets->scratch_color_image.extent.height, 1};
          vkCmdCopyImage(cmd_buf, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
          r_vulkan_image_transition(cmd_buf, render_targets->stage_color_image.h,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                    VK_IMAGE_ASPECT_COLOR_BIT);
        }
        ProfEnd();
      }break;
      case R_PassKind_Edge:
      {
        ProfBegin("edge pass");
        // unpack params
        R_PassParams_Edge *params = pass->params_edge;

        /////////////////////////////////////////////////////////////////////////////////
        // viewport and scissor

        VkViewport viewport = {0};
        viewport.width    = render_targets->scratch_color_image.extent.width;
        viewport.height   = render_targets->scratch_color_image.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

        VkRect2D scissor = {0};
        scissor.offset = (VkOffset2D){0, 0};
        scissor.extent = render_targets->stage_color_image.extent;
        vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

        /////////////////////////////////////////////////////////////////////////////////
        // draw

        r_vulkan_image_transition(frame->cmd_buf, render_targets->stage_color_image.h, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        r_vulkan_image_transition(frame->cmd_buf, render_targets->edge_image.h, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        r_vulkan_image_transition(frame->cmd_buf, render_targets->scratch_color_image.h, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        // TODO(XXX): we are not using params here (clip and rect e.g.)

        VkClearValue clear_colors[1] = {0};
        clear_colors[0].color = (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 0.0f }}; /* black with 100% opacity */

        VkRenderingAttachmentInfo color_attachment_infos[1] = {0};
        color_attachment_infos[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_infos[0].imageView = render_targets->scratch_color_image.view;
        color_attachment_infos[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        color_attachment_infos[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment_infos[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_infos[0].clearValue = clear_colors[0];

        VkRenderingInfo render_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        render_info.renderArea.offset = (VkOffset2D){0, 0};
        render_info.renderArea.extent = wnd->render_targets->scratch_color_image.extent;
        render_info.layerCount = 1;
        render_info.colorAttachmentCount = ArrayCount(color_attachment_infos);
        render_info.pColorAttachments = color_attachment_infos;

        // push constant
        R_Vulkan_PUSH_Edge push = {0};
        push.time = params->elapsed_secs;

        vkCmdPushConstants(cmd_buf, wnd->pipelines.edge.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(R_Vulkan_PUSH_Edge), &push);

        // bind pipeline
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.edge.h);

        // begin drawing
        vkCmdBeginRendering(cmd_buf, &render_info);

        // bind stage color descriptor
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.edge.layout, 0, 1, &render_targets->stage_color_ds.h, 0, NULL);
        // bind edge color descriptor
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.edge.layout, 1, 1, &render_targets->edge_ds.h, 0, NULL);

        vkCmdDraw(cmd_buf, 4, 1, 0, 0);

        // end drawing
        vkCmdEndRendering(cmd_buf);

        r_vulkan_image_transition(cmd_buf, render_targets->scratch_color_image.h, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        r_vulkan_image_transition(cmd_buf, render_targets->stage_color_image.h, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        // copy scratch image to stage image
        {
          VkImage src = render_targets->scratch_color_image.h;
          VkImage dst = render_targets->stage_color_image.h;
          VkImageCopy copy_region = {0};
          copy_region.srcSubresource = (VkImageSubresourceLayers){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
          };
          copy_region.srcOffset = (VkOffset3D){0,0,0};
          copy_region.dstSubresource = (VkImageSubresourceLayers){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
          };
          copy_region.dstOffset = (VkOffset3D){0,0,0};
          copy_region.extent = (VkExtent3D){render_targets->scratch_color_image.extent.width, render_targets->scratch_color_image.extent.height, 1};
          vkCmdCopyImage(cmd_buf, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
          r_vulkan_image_transition(cmd_buf, render_targets->stage_color_image.h,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                    VK_IMAGE_ASPECT_COLOR_BIT);
        }
        ProfEnd();
      }break;
      case R_PassKind_Crt:
      {
        ProfBegin("crt pass");
        // unpack params
        R_PassParams_Crt *params = pass->params_crt;

        /////////////////////////////////////////////////////////////////////////////////
        // viewport and scissor

        VkViewport viewport = {0};
        viewport.width    = render_targets->scratch_color_image.extent.width;
        viewport.height   = render_targets->scratch_color_image.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

        VkRect2D scissor = {0};
        scissor.offset = (VkOffset2D){0, 0};
        scissor.extent = render_targets->stage_color_image.extent;
        vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

        /////////////////////////////////////////////////////////////////////////////////
        // draw

        r_vulkan_image_transition(frame->cmd_buf, render_targets->stage_color_image.h, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        r_vulkan_image_transition(frame->cmd_buf, render_targets->scratch_color_image.h, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        // TODO(XXX): we are not using params here (clip and rect e.g.)

        VkClearValue clear_colors[1] = {0};
        clear_colors[0].color = (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 0.0f }}; /* black with 100% opacity */

        VkRenderingAttachmentInfo color_attachment_infos[1] = {0};
        color_attachment_infos[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_infos[0].imageView = render_targets->scratch_color_image.view;
        color_attachment_infos[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        color_attachment_infos[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment_infos[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_infos[0].clearValue = clear_colors[0];

        VkRenderingInfo render_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        render_info.renderArea.offset = (VkOffset2D){0, 0};
        render_info.renderArea.extent = wnd->render_targets->scratch_color_image.extent;
        render_info.layerCount = 1;
        render_info.colorAttachmentCount = ArrayCount(color_attachment_infos);
        render_info.pColorAttachments = color_attachment_infos;

        // push constant
        R_Vulkan_PUSH_Crt push = {0};
        push.resolution.x = render_targets->scratch_color_image.extent.width;
        push.resolution.y = render_targets->scratch_color_image.extent.height;
        push.time = params->elapsed_secs;
        push.scan = params->scan;
        push.warp = params->warp;

        vkCmdPushConstants(cmd_buf, wnd->pipelines.crt.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(R_Vulkan_PUSH_Crt), &push);

        // bind pipeline
        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.crt.h);

        // begin drawing
        vkCmdBeginRendering(cmd_buf, &render_info);

        // bind stage color descriptor
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.crt.layout, 0, 1, &render_targets->stage_color_ds.h, 0, NULL);

        vkCmdDraw(cmd_buf, 4, 1, 0, 0);

        // end drawing
        vkCmdEndRendering(cmd_buf);

        r_vulkan_image_transition(cmd_buf, render_targets->scratch_color_image.h, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        r_vulkan_image_transition(cmd_buf, render_targets->stage_color_image.h, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        // copy scratch image to stage image
        {
          VkImage src = render_targets->scratch_color_image.h;
          VkImage dst = render_targets->stage_color_image.h;
          VkImageCopy copy_region = {0};
          copy_region.srcSubresource = (VkImageSubresourceLayers){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
          };
          copy_region.srcOffset = (VkOffset3D){0,0,0};
          copy_region.dstSubresource = (VkImageSubresourceLayers){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
          };
          copy_region.dstOffset = (VkOffset3D){0,0,0};
          copy_region.extent = (VkExtent3D){render_targets->scratch_color_image.extent.width, render_targets->scratch_color_image.extent.height, 1};
          vkCmdCopyImage(cmd_buf, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
          r_vulkan_image_transition(cmd_buf, render_targets->stage_color_image.h,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                    VK_IMAGE_ASPECT_COLOR_BIT);
        }
        ProfEnd();
      }break;
      case R_PassKind_Geo2D:
      {
        ProfBegin("geo2d pass");
        Assert(geo2d_pass_index < R_MAX_GEO2D_PASS);

        // unpack params
        R_PassParams_Geo2D *params = pass->params_geo2d;
        R_BatchGroup2DMap *group_map = &params->batches;

        // pre-compute some inverses
        Mat4x4F32 proj_inv = inverse_4x4f32(params->projection);
        Mat4x4F32 view_inv = inverse_4x4f32(params->view);

        /////////////////////////////////////////////////////////////////////////////////
        // unpack & upload buffer and its offset

        // inst buffer
        R_Vulkan_Buffer *inst_buffer = &frame->inst_buffer_mesh2d[geo2d_pass_index];
        // ubo
        R_Vulkan_UBOBuffer *ubo_buffer = &frame->ubo_buffers[R_Vulkan_UBOTypeKind_Geo2D];
        U32 ubo_buffer_offset = geo2d_pass_index * ubo_buffer->stride;
        U8 *ubo_dst = (U8*)ubo_buffer->buffer.mapped + ubo_buffer_offset;

        // upload ubo buffer to gpu
        R_Vulkan_UBO_Geo2D ubo = {0};
        ubo.proj = params->projection;
        ubo.proj_inv = proj_inv;
        ubo.view = params->view;
        ubo.view_inv = view_inv;
        MemoryCopy(ubo_dst, &ubo, sizeof(R_Vulkan_UBO_Geo2D));

        /////////////////////////////////////////////////////////////////////////////////
        // viewport and scissor

        VkViewport viewport = {0};
        Vec2F32 viewport_dim = dim_2f32(params->viewport);
        if(viewport_dim.x == 0 || viewport_dim.y == 0)
        {
          viewport.width  = render_targets->stage_color_image.extent.width;
          viewport.height = render_targets->stage_color_image.extent.height;
        }
        else
        {
          viewport.x      = params->viewport.p0.x;
          viewport.y      = params->viewport.p0.y;
          viewport.width  = viewport_dim.x;
          viewport.height = viewport_dim.y;
        }
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

        VkRect2D scissor = {0};
        scissor.offset = (VkOffset2D){0, 0};
        scissor.extent = render_targets->stage_color_image.extent;
        vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

        /////////////////////////////////////////////////////////////////////////////////
        // forward pass

        r_vulkan_image_transition(frame->cmd_buf, render_targets->geo2d_color_image.h, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        r_vulkan_image_transition(frame->cmd_buf, render_targets->edge_image.h, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        // NOTE(XXX): id image is cleared at the beginning of the frame
        VkClearValue clear_colors[2] = {0};
        clear_colors[0].color = (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 0.0f }}; /* black with 100% opacity */
        clear_colors[1].color = (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 0.0f }};

        VkRenderingAttachmentInfo color_attachment_infos[3] = {0};
        color_attachment_infos[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_infos[0].imageView = render_targets->geo2d_color_image.view;
        color_attachment_infos[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        color_attachment_infos[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment_infos[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_infos[0].clearValue = clear_colors[0];

        color_attachment_infos[1].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_infos[1].imageView = render_targets->stage_id_image.view;
        color_attachment_infos[1].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        color_attachment_infos[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachment_infos[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        color_attachment_infos[2].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_infos[2].imageView = render_targets->edge_image.view;
        color_attachment_infos[2].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        color_attachment_infos[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment_infos[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_infos[2].clearValue = clear_colors[1];

        VkRenderingInfo render_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        // The render area defiens where shader loads and stores will take place
        // The pixels outside this region will have undefined values
        render_info.renderArea.offset = (VkOffset2D){0, 0};
        render_info.renderArea.extent = wnd->render_targets->stage_color_image.extent;
        render_info.layerCount = 1;
        render_info.colorAttachmentCount = ArrayCount(color_attachment_infos);
        render_info.pColorAttachments = color_attachment_infos;

        // begin drawing
        vkCmdBeginRendering(cmd_buf, &render_info);

        // bind ubo
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.geo2d.forward[0].layout, 0, 1, &ubo_buffer->set.h, 1, &ubo_buffer_offset);

        // issue drawing
        U64 inst_idx = 0;
        ProfScope("geo2d draw issuing") for(R_BatchGroup2DMapNode *n = group_map->first; n != 0; n = n->insert_next)
        {
          // unpack group params
          R_BatchList *batches = &n->batches;
          R_BatchGroup2DParams *group_params = &n->params;

          R_Vulkan_Buffer *vertices = r_vulkan_buffer_from_handle(group_params->vertices);
          R_Vulkan_Buffer *indices = r_vulkan_buffer_from_handle(group_params->indices);
          U64 indice_count = group_params->indice_count;
          U64 inst_count = batches->byte_count / batches->bytes_per_inst;
          U64 line_width = group_params->line_width;
          R_GeoTopologyKind topology = group_params->topology;
          R_GeoPolygonKind polygon = group_params->polygon;

          // get & fill instance buffer
          U64 inst_buffer_off = inst_idx*sizeof(R_Mesh2DInst);
          for(R_BatchNode *batch = batches->first; batch != 0; batch = batch->next)
          {
            U64 batch_inst_count = batch->v.byte_count / sizeof(R_Mesh2DInst);
            U8 *dst_ptr = (U8*)inst_buffer->mapped+inst_idx*sizeof(R_Mesh2DInst);
            MemoryCopy(dst_ptr, batch->v.v, batch->v.byte_count);
            inst_idx += batch_inst_count;
          }

          // unpack specific pipeline for topology and polygon mode
          R_Vulkan_Pipeline *pipeline = &wnd->pipelines.geo2d.forward[R_GeoPolygonKind_COUNT*topology + polygon];

          // bind tex
          R_Handle tex_handle = group_params->tex;
          if(r_handle_match(tex_handle, r_handle_zero()))
          {
            tex_handle = r_vulkan_state->backup_texture;
          }
          R_Vulkan_Tex2D *texture = r_vulkan_tex2d_from_handle(tex_handle);
          vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 1, 1, &texture->desc_set.h, 0, NULL);

          // bind vertex/indice/inst buffer
          vkCmdBindVertexBuffers(cmd_buf, 0, 1, &vertices->h, &(VkDeviceSize){group_params->vertex_buffer_offset});
          vkCmdBindIndexBuffer(cmd_buf, indices->h, (VkDeviceSize){group_params->indice_buffer_offset}, VK_INDEX_TYPE_UINT32);
          vkCmdBindVertexBuffers(cmd_buf, 1, 1, &inst_buffer->h, &(VkDeviceSize){inst_buffer_off});

          // bind pipeline
          vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->h);

          // set line width if device supports widelines feature
          if(r_vulkan_pdevice()->features.wideLines == VK_TRUE)
          {
            vkCmdSetLineWidth(cmd_buf, line_width);
          }

          // draw
          vkCmdDrawIndexed(cmd_buf, indice_count, inst_count, 0, 0, 0);
        }

        vkCmdEndRendering(cmd_buf);

        /////////////////////////////////////////////////////////////////////////////////
        // composite to the main staging buffer

        r_vulkan_image_transition(frame->cmd_buf, render_targets->geo2d_color_image.h, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

        {
          VkRenderingAttachmentInfo color_attachment_info = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
          color_attachment_info.imageView = render_targets->stage_color_image.view;
          color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
          color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

          VkRenderingInfo render_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
          // The render area defiens where shader loads and stores will take place
          // The pixels outside this region will have undefined values
          render_info.renderArea.offset = (VkOffset2D){0, 0};
          render_info.renderArea.extent = wnd->render_targets->stage_color_image.extent;
          render_info.layerCount = 1;
          render_info.colorAttachmentCount = 1;
          render_info.pColorAttachments = &color_attachment_info;

          // begin drawing
          vkCmdBeginRendering(cmd_buf, &render_info);

          vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.geo2d.composite.h);

          // Viewport and scissor
          VkViewport viewport = {0};
          viewport.x        = 0.0f;
          viewport.y        = 0.0f;
          viewport.width    = render_targets->stage_color_image.extent.width;
          viewport.height   = render_targets->stage_color_image.extent.height;
          viewport.minDepth = 0.0f;
          viewport.maxDepth = 1.0f;
          vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
          // Setup scissor rect
          VkRect2D scissor = {0};
          scissor.offset = (VkOffset2D){0, 0};
          scissor.extent = render_targets->stage_color_image.extent;
          vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
          // Bind stage color descriptor
          vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd->pipelines.geo2d.composite.layout, 0, 1, &render_targets->geo2d_color_ds.h, 0, NULL);
          vkCmdDraw(cmd_buf, 4, 1, 0, 0);

          // end drawing
          vkCmdEndRendering(cmd_buf);
        }
        geo2d_pass_index++;
        ProfEnd();
      }break;
      case R_PassKind_Geo3D: 
      {
        ProfBegin("geo3d pass");
        Assert(geo3d_pass_index < R_MAX_GEO3D_PASS);

        // Unpack params
        R_PassParams_Geo3D *params = pass->params_geo3d;
        R_BatchGroup3DMap *mesh_group_map = &params->mesh_batches;

        R_Vulkan_RenderTargets *render_targets = wnd->render_targets;

        // Pre-compute some inverse
        Mat4x4F32 proj_inv = inverse_4x4f32(params->projection);
        Mat4x4F32 view_inv = inverse_4x4f32(params->view);

        // some variables to be initialized
        Vec2U32 grid_size = {0}; // light grid size

        /////////////////////////////////////////////////////////////////////////////////
        // unpack all kinds of buffer offsets

        // inst buffer
        R_Vulkan_Buffer *inst_buffer = &frame->inst_buffer_mesh3d[geo3d_pass_index];

        // geo3d ubo
        // NOTE(k): Geo3d uniform is per Geo3D pass unlike rect pass which is per group
        R_Vulkan_UBOBuffer *geo3d_ubo_buffer = &frame->ubo_buffers[R_Vulkan_UBOTypeKind_Geo3D];
        U32 geo3d_ubo_buffer_off = geo3d_pass_index * geo3d_ubo_buffer->stride;
        U8 *geo3d_ubo_dst = (U8*)geo3d_ubo_buffer->buffer.mapped + geo3d_ubo_buffer_off;

        // tile frustum ubo
        R_Vulkan_UBOBuffer *tile_frustum_ubo_buffer = &frame->ubo_buffers[R_Vulkan_UBOTypeKind_Geo3D_TileFrustum];
        U32 tile_frustum_ubo_buffer_off = geo3d_pass_index * tile_frustum_ubo_buffer->stride;
        U8 *tile_frustum_ubo_dst = (U8*)tile_frustum_ubo_buffer->buffer.mapped + tile_frustum_ubo_buffer_off;

        // light culling ubo
        R_Vulkan_UBOBuffer *light_culling_ubo_buffer = &frame->ubo_buffers[R_Vulkan_UBOTypeKind_Geo3D_LightCulling];
        U32 light_culling_ubo_buffer_off = geo3d_pass_index * light_culling_ubo_buffer->stride;
        U8 *light_culling_ubo_dst = (U8*)light_culling_ubo_buffer->buffer.mapped + light_culling_ubo_buffer_off;

        // lights sbo
        R_Vulkan_SBOBuffer *lights_sbo_buffer = &frame->sbo_buffers[R_Vulkan_SBOTypeKind_Geo3D_Lights];
        U32 lights_sbo_buffer_off = geo3d_pass_index*lights_sbo_buffer->stride;
        U8 *lights_sbo_dst = (U8*)lights_sbo_buffer->buffer.mapped + lights_sbo_buffer_off;

        // tiles sbo (device local)
        R_Vulkan_SBOBuffer *tiles_sbo_buffer = &frame->sbo_buffers[R_Vulkan_SBOTypeKind_Geo3D_Tiles];
        U32 tiles_sbo_buffer_off = geo3d_pass_index * tiles_sbo_buffer->stride;

        // light indices sbo (device local)
        R_Vulkan_SBOBuffer *light_indices_sbo_buffer = &frame->sbo_buffers[R_Vulkan_SBOTypeKind_Geo3D_LightIndices];
        U32 light_indices_sbo_buffer_off = geo3d_pass_index * light_indices_sbo_buffer->stride;

        // tile lights sbo (deviec local)
        R_Vulkan_SBOBuffer *tile_lights_sbo_buffer = &frame->sbo_buffers[R_Vulkan_SBOTypeKind_Geo3D_TileLights];
        U32 tile_lights_sbo_buffer_off = geo3d_pass_index * tile_lights_sbo_buffer->stride;

        // joints sbo
        R_Vulkan_SBOBuffer *joints_sbo_buffer = &frame->sbo_buffers[R_Vulkan_SBOTypeKind_Geo3D_Joints];
        U32 joints_sbo_buffer_off = geo3d_pass_index*R_MAX_JOINTS_PER_PASS * sizeof(R_Vulkan_SBO_Geo3D_Joint);
        U8 *joints_sbo_dst = (U8*)(joints_sbo_buffer->buffer.mapped) + joints_sbo_buffer_off;

        // materials sbo
        R_Vulkan_SBOBuffer *materials_sbo_buffer = &frame->sbo_buffers[R_Vulkan_SBOTypeKind_Geo3D_Materials];
        U32 materials_sbo_buffer_off = geo3d_pass_index*R_MAX_MATERIALS_PER_PASS * sizeof(R_Vulkan_SBO_Geo3D_Material); 
        U8 *materials_sbo_dst = (U8*)(materials_sbo_buffer->buffer.mapped) + materials_sbo_buffer_off;

        /////////////////////////////////////////////////////////////////////////////////
        // upload geo3d ubo buffer

        R_Vulkan_UBO_Geo3D geo3d_ubo = {0};
        geo3d_ubo.proj      = params->projection;
        geo3d_ubo.proj_inv  = proj_inv;
        geo3d_ubo.view      = params->view;
        geo3d_ubo.view_inv  = view_inv;
        geo3d_ubo.show_grid = !params->omit_grid;
        MemoryCopy(geo3d_ubo_dst, &geo3d_ubo, sizeof(R_Vulkan_UBO_Geo3D));

        /////////////////////////////////////////////////////////////////////////////////
        // upload materials sbo buffer

        Temp scratch = scratch_begin(0,0);
        AssertAlways(params->material_count < R_MAX_MATERIALS_PER_PASS);
        R_Vulkan_SBO_Geo3D_Material *materials = push_array(scratch.arena, R_Vulkan_SBO_Geo3D_Material, params->material_count);
        for(U64 i = 0; i < params->material_count; i++)
        {
          R_Vulkan_SBO_Geo3D_Material *mat_dst = &materials[i];
          R_Geo3D_Material *mat_src = &params->materials[i];
          MemoryCopy(mat_dst, mat_src, sizeof(R_Geo3D_Material));

          ///////////////////////////////////////////////////////////////////////////////
          // update sample map
          // Set up texture sample map matrix based on texture format
          // Vulkan use col-major

          // diffuse texture
          if(mat_dst->has_diffuse_texture)
          {
            R_Vulkan_Tex2D *tex = r_vulkan_tex2d_from_handle(params->textures[i].array[R_Geo3D_TexKind_Diffuse]);
            Vec4F32 texture_sample_channel_map[4] = {
              {1, 0, 0, 0},
              {0, 1, 0, 0},
              {0, 0, 1, 0},
              {0, 0, 0, 1},
            };
            if(tex)
            {
              switch(tex->format)
              {
                default: break;
                case R_Tex2DFormat_R8: 
                {
                  MemoryZeroStruct(&texture_sample_channel_map);
                  // TODO(k): why, shouldn't vulkan use col-major order?
                  texture_sample_channel_map[0] = v4f32(1, 1, 1, 1);
                  // texture_sample_channel_map[0].x = 1;
                  // texture_sample_channel_map[1].x = 1;
                  // texture_sample_channel_map[2].x = 1;
                  // texture_sample_channel_map[3].x = 1;
                }break;
              }
            }
            MemoryCopy(&mat_dst->diffuse_sample_channel_map, &texture_sample_channel_map, sizeof(Mat4x4F32));
          }
        }
        MemoryCopy(materials_sbo_dst, materials, sizeof(R_Vulkan_SBO_Geo3D_Material)*params->material_count);
        scratch_end(scratch);

        /////////////////////////////////////////////////////////////////////////////////
        // walk through all mesh node to fill up buffers(joints, inst_buffer)

        {
          U64 joint_idx = 0;
          U64 inst_idx = 0;
          for(R_BatchGroup3DMapNode *n = mesh_group_map->first; n != 0; n = n->insert_next)
          {
            R_BatchList *batches = &n->batches;
            R_BatchGroup3DParams *group_params = &n->params;
            U64 indice_count = group_params->indice_count;
            U64 line_width = group_params->line_width;

            // get & fill inst buffer
            for(R_BatchNode *batch = batches->first; batch != 0; batch = batch->next)
            {
              // Copy instance skinning data to sbo joints buffer
              U64 batch_inst_count = batch->v.byte_count / sizeof(R_Mesh3DInst);
              U8 *dst_ptr = (U8*)inst_buffer->mapped + inst_idx*sizeof(R_Mesh3DInst);
              for(U64 i = 0; i < batch_inst_count; i++)
              {
                R_Mesh3DInst *inst = ((R_Mesh3DInst *)batch->v.v)+i;
                if(inst->joint_count > 0)
                {
                  U32 size = sizeof(Mat4x4F32) * inst->joint_count;
                  // TODO(XXX): we should consider stride here
                  inst->first_joint = joint_idx;
                  MemoryCopy(joints_sbo_dst+joint_idx*sizeof(R_Vulkan_SBO_Geo3D_Joint), inst->joint_xforms, size);
                  joint_idx += inst->joint_count;
                }
              }
              inst_idx += batch_inst_count;

              // Copy to instance buffer
              MemoryCopy(dst_ptr, batch->v.v, batch->v.byte_count);
            }
          }
        }

        /////////////////////////////////////////////////////////////////////////////////
        // viewport and scissor

        VkViewport viewport = {0};
        Vec2F32 viewport_dim = dim_2f32(params->viewport);
        if(viewport_dim.x == 0 || viewport_dim.y == 0)
        {
          viewport.width  = render_targets->stage_color_image.extent.width;
          viewport.height = render_targets->stage_color_image.extent.height;
        }
        else
        {
          viewport.x      = params->viewport.p0.x;
          viewport.y      = params->viewport.p0.y;
          viewport.width  = viewport_dim.x;
          viewport.height = viewport_dim.y;
        }
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

        VkRect2D scissor = {0};
        scissor.offset = (VkOffset2D){0, 0};
        scissor.extent = render_targets->stage_color_image.extent;
        vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

        /////////////////////////////////////////////////////////////////////////////////
        // lighting
        // NOTE(k): we can only do compute pass outside of vulkan renderpass

        if(!params->omit_light)
        {
          // geo3d pre_depth_image: undefined => depth
          r_vulkan_image_transition(cmd_buf, render_targets->geo3d_pre_depth_image.h, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

          ///////////////////////////////////////////////////////////////////////////////
          // pre depth pass (for tiling frustum)

          {
            R_Vulkan_Pipeline *pipelines = wnd->pipelines.geo3d.z_pre;
            VkClearValue clear_colors[1];
            clear_colors[0].depthStencil = (VkClearDepthStencilValue){1.0f, 0};

            VkRenderingAttachmentInfo depth_attachment_infos[1] = {0};
            depth_attachment_infos[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depth_attachment_infos[0].imageView = render_targets->geo3d_pre_depth_image.view;
            depth_attachment_infos[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            depth_attachment_infos[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth_attachment_infos[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depth_attachment_infos[0].clearValue = clear_colors[0];

            VkRenderingInfo render_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
            render_info.renderArea.offset = (VkOffset2D){0, 0};
            render_info.renderArea.extent = wnd->render_targets->stage_color_image.extent;
            render_info.layerCount = 1;
            render_info.colorAttachmentCount = 0;
            render_info.pDepthAttachment = depth_attachment_infos;

            // begin drawing
            vkCmdBeginRendering(cmd_buf, &render_info);

            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout, 0, 1, &geo3d_ubo_buffer->set.h, 1, &geo3d_ubo_buffer_off);
            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout, 1, 1, &joints_sbo_buffer->set.h, 1, &joints_sbo_buffer_off);

            U64 inst_idx = 0;
            for(R_BatchGroup3DMapNode *n = mesh_group_map->first; n!=0; n = n->insert_next) 
            {
              // Unpack group params
              R_BatchList *batches = &n->batches;
              R_BatchGroup3DParams *group_params = &n->params;
              R_Vulkan_Buffer *mesh_vertices = r_vulkan_buffer_from_handle(group_params->mesh_vertices);
              R_Vulkan_Buffer *mesh_indices = r_vulkan_buffer_from_handle(group_params->mesh_indices);
              U64 inst_count = batches->byte_count / batches->bytes_per_inst;
              U64 indice_count = group_params->indice_count;
              U64 line_width = group_params->line_width;

              // Unpack specific pipeline for topology and polygon mode
              R_GeoTopologyKind topology = group_params->mesh_geo_topology;
              R_GeoPolygonKind polygon = group_params->mesh_geo_polygon;

              R_Vulkan_Pipeline *pipeline = &pipelines[R_GeoPolygonKind_COUNT*topology + polygon];
              // Bind pipeline
              // The second parameter specifies if the pipeline object is a graphics or compute pipelinVK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BITe
              // We've now told Vulkan which operations to execute in the graphcis pipeline and which attachment to use in the fragment shader
              vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->h);

              vkCmdBindVertexBuffers(cmd_buf, 0, 1, &mesh_vertices->h, &(VkDeviceSize){group_params->vertex_buffer_offset});
              vkCmdBindIndexBuffer(cmd_buf, mesh_indices->h, (VkDeviceSize){group_params->indice_buffer_offset}, VK_INDEX_TYPE_UINT32);
              vkCmdBindVertexBuffers(cmd_buf, 1, 1, &inst_buffer->h, &(VkDeviceSize){inst_idx*sizeof(R_Mesh3DInst)});

              // set line width if device supports widelines feature
              if(r_vulkan_pdevice()->features.wideLines == VK_TRUE)
              {
                vkCmdSetLineWidth(cmd_buf, line_width);
              }

              // draw mesh
              vkCmdDrawIndexed(cmd_buf, indice_count, inst_count, 0, 0, 0);
              inst_idx += inst_count;
            }

            vkCmdEndRendering(cmd_buf);
          }

          ///////////////////////////////////////////////////////////////////////////////
          // Tile frustum compute pass

          {
            /////////////////////////////////////////////////////////////////////////////
            // calculate grid size based on the size of viewport

            R_Vulkan_Pipeline *pipeline = &wnd->pipelines.geo3d.tile_frustum;

            Vec2F32 viewport_dim = dim_2f32(params->viewport);
            if(viewport_dim.x == 0 || viewport_dim.y == 0)
            {
              viewport_dim.x = render_targets->stage_color_image.extent.width;
              viewport_dim.y = render_targets->stage_color_image.extent.height;
            }
            viewport_dim.x = AlignPow2((U64)ceil_f32(viewport_dim.x), R_VULKAN_TILE_SIZE);
            viewport_dim.y = AlignPow2((U64)ceil_f32(viewport_dim.y), R_VULKAN_TILE_SIZE);

            grid_size.x = viewport_dim.x / R_VULKAN_TILE_SIZE;
            grid_size.y = viewport_dim.y / R_VULKAN_TILE_SIZE;

            /////////////////////////////////////////////////////////////////////////////
            // unpack ubo buffer

            R_Vulkan_UBO_Geo3D_TileFrustum ubo = {0};
            ubo.proj_inv = proj_inv;
            ubo.grid_size = grid_size;
            MemoryCopy(tile_frustum_ubo_dst, &ubo, sizeof(R_Vulkan_UBO_Geo3D_TileFrustum));
            // bind ubo
            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    pipeline->layout, 0, 1,
                                    &tile_frustum_ubo_buffer->set.h,
                                    1, &tile_frustum_ubo_buffer_off);

            /////////////////////////////////////////////////////////////////////////////
            // unpack sbo buffer

            // bind sbo
            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    pipeline->layout, 1, 1,
                                    &tiles_sbo_buffer->set.h,
                                    1, &tiles_sbo_buffer_off);

            /////////////////////////////////////////////////////////////////////////////
            // compute pass

            Vec2U32 thread_group_size = {R_VULKAN_TILE_SIZE,R_VULKAN_TILE_SIZE};
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->h);
            // NOTE(k): shader need to check xy is inboud of grid size
            vkCmdDispatch(cmd_buf, AlignPow2(grid_size.x,thread_group_size.x) / thread_group_size.x, AlignPow2(grid_size.y,thread_group_size.y) / thread_group_size.y, 1);
          }

          ///////////////////////////////////////////////////////////////////////////////
          // light culling compute pass

          // pre_depth_image: color_output => shader_read
          r_vulkan_image_transition(frame->cmd_buf, render_targets->geo3d_pre_depth_image.h,
                                    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

          // TODO(XXX): need a memory berrier here to wait on z_pre and frustum culling to be finished
          {
            R_Vulkan_Pipeline *pipeline = &wnd->pipelines.geo3d.light_culling;

            // ubo upload & bind
            R_Vulkan_UBO_Geo3D_LightCulling ubo = {0};
            ubo.proj_inv = proj_inv;
            ubo.light_count = params->light_count;
            MemoryCopy(light_culling_ubo_dst, &ubo, sizeof(R_Vulkan_UBO_Geo3D_LightCulling));
            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    pipeline->layout, 0, 1,
                                    &light_culling_ubo_buffer->set.h, 1, &light_culling_ubo_buffer_off);
            // global lights
            // upload lights
            MemoryCopy(lights_sbo_dst, params->lights, params->light_count*sizeof(R_Geo3D_Light));
            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    pipeline->layout, 1, 1,
                                    &lights_sbo_buffer->set.h, 1, &lights_sbo_buffer_off);
            // zpre
            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    pipeline->layout, 2, 1,
                                    &render_targets->geo3d_pre_depth_ds.h, 0, 0);
            // tiles
            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    pipeline->layout, 3, 1,
                                    &tiles_sbo_buffer->set.h, 1, &tiles_sbo_buffer_off);

            // global light indices
            // do zero initilization for this buffer (gpu side, buffer clear)
            // TODO(XXX): we may need to add a buffer barrier here for syncronization
            vkCmdFillBuffer(cmd_buf, light_indices_sbo_buffer->buffer.h, light_indices_sbo_buffer_off, light_indices_sbo_buffer->stride, 0);
            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    pipeline->layout, 4, 1,
                                    &light_indices_sbo_buffer->set.h, 1, &light_indices_sbo_buffer_off);
            // tile lights
            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE,
                                    pipeline->layout, 5, 1,
                                    &tile_lights_sbo_buffer->set.h, 1, &tile_lights_sbo_buffer_off);

            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->h);
            vkCmdDispatch(cmd_buf, grid_size.x, grid_size.y, 1);
          }
        }

        // TODO(XXX): missing buffer barrier for compute shader to be finished

        // geo3d_color_image: undefined => color_output
        r_vulkan_image_transition(frame->cmd_buf, render_targets->geo3d_color_image.h,
                                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT);
        // geo3d_normal_depth_image: undefined => color_output
        r_vulkan_image_transition(frame->cmd_buf, render_targets->geo3d_normal_depth_image.h,
                                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT);

        /////////////////////////////////////////////////////////////////////////////////
        //~ start geo3d rendering

        {
          // unpack pipelines
          R_Vulkan_Pipeline *pipelines = wnd->pipelines.geo3d.forward;
          R_Vulkan_Pipeline *debug_pipelines = wnd->pipelines.geo3d.debug;

          VkClearValue clear_colors[3] = {0};
          // color image
          clear_colors[0].color        = (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 0.0f }}; /* black with 100% opacity */
          // The range of depths in the depth buffer is 0.0 to 1.0 in Vulkan, where 1.0 lies at the far view plane and 0.0 at the near view plane. The initial value at each point in the depth buffer should be the furthest possible depth, which is 1.0
          // normal depth image
          clear_colors[1].color        = (VkClearColorValue){0.f, 0.f, 0.f, 1.f};
          // depth
          clear_colors[2].depthStencil = (VkClearDepthStencilValue){1.0f, 0};

          VkRenderingAttachmentInfo attachment_infos[4] = {0};
          attachment_infos[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
          attachment_infos[0].imageView = render_targets->geo3d_color_image.view;
          attachment_infos[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
          attachment_infos[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
          attachment_infos[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          attachment_infos[0].clearValue = clear_colors[0];

          attachment_infos[1].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
          attachment_infos[1].imageView = render_targets->geo3d_normal_depth_image.view;
          attachment_infos[1].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
          attachment_infos[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
          attachment_infos[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          attachment_infos[1].clearValue = clear_colors[1];

          attachment_infos[2].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
          attachment_infos[2].imageView = render_targets->stage_id_image.view;
          attachment_infos[2].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
          attachment_infos[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          attachment_infos[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

          attachment_infos[3].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
          attachment_infos[3].imageView = render_targets->geo3d_depth_image.view;
          attachment_infos[3].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
          attachment_infos[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
          attachment_infos[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
          attachment_infos[3].clearValue = clear_colors[2];

          VkRenderingInfo render_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
          render_info.renderArea.offset = (VkOffset2D){0, 0};
          render_info.renderArea.extent = wnd->render_targets->stage_color_image.extent;
          render_info.layerCount = 1;
          render_info.colorAttachmentCount = 3;
          render_info.pColorAttachments = attachment_infos;
          render_info.pDepthAttachment = &attachment_infos[3];

          // begin drawing
          vkCmdBeginRendering(cmd_buf, &render_info);

          //- draw geo3d debug if asked
          if(!params->omit_grid)
          {
            // TODO(XXX): debug pipeline don't need multiple polygon & topology
            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    debug_pipelines[0].layout, 0, 1,
                                    &geo3d_ubo_buffer->set.h, 1, &geo3d_ubo_buffer_off);
            // Bind geo3d debug pipeline
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              debug_pipelines[R_GeoPolygonKind_COUNT * R_GeoTopologyKind_Triangles + R_GeoPolygonKind_Fill].h);

            // Draw mesh debug info (grid e.g.)
            vkCmdDraw(cmd_buf, 6, 1, 0, 0);
          }

          ///////////////////////////////////////////////////////////////////////////////
          //- push constants for geo3d forward pass

          R_Vulkan_PUSH_Geo3D_Forward push;
          push.light_grid_size = grid_size;
          push.viewport.x = viewport.width;
          push.viewport.y = viewport.height;
          vkCmdPushConstants(cmd_buf, pipelines[0].layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(R_Vulkan_PUSH_Geo3D_Forward), &push);

          ///////////////////////////////////////////////////////////////////////////////
          //- binds (ubo & sbo)

          // 0: ubo
          vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  pipelines[0].layout, 0, 1,
                                  &geo3d_ubo_buffer->set.h, 1, &geo3d_ubo_buffer_off);

          // 1: joints
          vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout, 1, 1, &joints_sbo_buffer->set.h, 1, &joints_sbo_buffer_off);

          // 2: texture (bind when loop through instances)

          // 3: lights
          vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  pipelines[0].layout, 3, 1,
                                  &lights_sbo_buffer->set.h, 1, &lights_sbo_buffer_off);
          // 4: global light indices
          vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  pipelines[0].layout, 4, 1,
                                  &light_indices_sbo_buffer->set.h, 1, &light_indices_sbo_buffer_off);
          // 5: tile lights
          vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  pipelines[0].layout, 5, 1,
                                  &tile_lights_sbo_buffer->set.h, 1, &tile_lights_sbo_buffer_off);

          // 6: materials
          vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  pipelines[0].layout, 6, 1,
                                  &materials_sbo_buffer->set.h, 1, &materials_sbo_buffer_off);

          ///////////////////////////////////////////////////////////////////////////////
          //- geo3d forward pass

          U64 inst_idx = 0;
          for(R_BatchGroup3DMapNode *n = mesh_group_map->first; n!=0; n = n->insert_next) 
          {
            // Unpack group params
            R_BatchList *batches = &n->batches;
            R_BatchGroup3DParams *group_params = &n->params;
            R_Vulkan_Buffer *mesh_vertices = r_vulkan_buffer_from_handle(group_params->mesh_vertices);
            R_Vulkan_Buffer *mesh_indices = r_vulkan_buffer_from_handle(group_params->mesh_indices);
            U64 inst_count = batches->byte_count / batches->bytes_per_inst;
            U64 indice_count = group_params->indice_count;
            U64 line_width = group_params->line_width;

            // Unpack specific pipeline for topology and polygon mode
            R_GeoTopologyKind topology = group_params->mesh_geo_topology;
            R_GeoPolygonKind polygon = group_params->mesh_geo_polygon;
            R_Vulkan_Pipeline *pipeline = &pipelines[R_GeoPolygonKind_COUNT*topology + polygon];

            // Bind texture
            // TODO(XXX): bind other texture (ambient, normal, bump, ...)
            // TODO(XXX): make use of texture sampler kind
            R_Handle tex_handle = params->textures[group_params->mat_idx].array[R_Geo3D_TexKind_Diffuse];
            if(r_handle_match(tex_handle, r_handle_zero()))
            {
              tex_handle = r_vulkan_state->backup_texture;
            }
            R_Vulkan_Tex2D *texture = r_vulkan_tex2d_from_handle(tex_handle);
            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 2, 1, &texture->desc_set.h, 0, NULL);

            vkCmdBindVertexBuffers(cmd_buf, 0, 1, &mesh_vertices->h, &(VkDeviceSize){group_params->vertex_buffer_offset});
            vkCmdBindIndexBuffer(cmd_buf, mesh_indices->h, (VkDeviceSize){group_params->indice_buffer_offset}, VK_INDEX_TYPE_UINT32);
            vkCmdBindVertexBuffers(cmd_buf, 1, 1, &inst_buffer->h, &(VkDeviceSize){inst_idx*sizeof(R_Mesh3DInst)});

            // Bind pipeline
            // The second parameter specifies if the pipeline object is a graphics or compute pipelinVK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BITe
            // We've now told Vulkan which operations to execute in the graphcis pipeline and which attachment to use in the fragment shader
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->h);

            // set line width if device supports widelines feature
            if(r_vulkan_pdevice()->features.wideLines == VK_TRUE)
            {
              vkCmdSetLineWidth(cmd_buf, line_width);
            }

            // draw mesh
            vkCmdDrawIndexed(cmd_buf, indice_count, inst_count, 0, 0, 0);
            inst_idx += inst_count;
          }
          vkCmdEndRendering(cmd_buf);
        }

        /////////////////////////////////////////////////////////////////////////////////
        // Composite to the main staging buffer

        // geo3d_color_image: color_output => shader_read
        r_vulkan_image_transition(frame->cmd_buf, render_targets->geo3d_color_image.h,
                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT);
        // geo3d_normal_depth_image: color_output => shader_read
        r_vulkan_image_transition(frame->cmd_buf, render_targets->geo3d_normal_depth_image.h,
                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT);
        {
          // unpack pipelines
          R_Vulkan_Pipeline *pipeline = &wnd->pipelines.geo3d.composite;

          VkRenderingAttachmentInfo attachment_infos[1] = {0};
          attachment_infos[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
          attachment_infos[0].imageView = render_targets->stage_color_image.view;
          attachment_infos[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
          attachment_infos[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
          attachment_infos[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

          VkRenderingInfo render_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
          render_info.renderArea.offset = (VkOffset2D){0, 0};
          render_info.renderArea.extent = wnd->render_targets->stage_color_image.extent;
          render_info.layerCount = 1;
          render_info.colorAttachmentCount = 1;
          render_info.pColorAttachments = attachment_infos;

          // begin rendering
          vkCmdBeginRendering(cmd_buf, &render_info);

          vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->h);
          // Viewport and scissor
          VkViewport viewport = {0};
          viewport.x        = 0.0f;
          viewport.y        = 0.0f;
          viewport.width    = render_targets->stage_color_image.extent.width;
          viewport.height   = render_targets->stage_color_image.extent.height;
          viewport.minDepth = 0.0f;
          viewport.maxDepth = 1.0f;
          vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
          // Setup scissor rect
          VkRect2D scissor = {0};
          scissor.offset = (VkOffset2D){0, 0};
          scissor.extent = render_targets->stage_color_image.extent;
          vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
          // Bind stage color descriptor
          vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &render_targets->geo3d_color_ds.h, 0, NULL);
          vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 1, 1, &render_targets->geo3d_normal_depth_ds.h, 0, NULL);
          vkCmdDraw(cmd_buf, 4, 1, 0, 0);

          // end rendering
          vkCmdEndRendering(cmd_buf);
        }
        geo3d_pass_index++;
        ProfEnd();
      }break;
      default: {InvalidPath;}break;
    }
  }

  wnd->ui_group_index = ui_group_index;
  wnd->ui_pass_index = ui_pass_index;
  wnd->geo2d_pass_index = geo2d_pass_index;
  wnd->geo3d_pass_index = geo3d_pass_index;
  ProfEnd();
}
