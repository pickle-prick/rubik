// TODO: memory leak when window resized

internal void
r_init(const char* app_name, OS_Handle window, bool debug)
{
    Arena *arena = arena_alloc();
    r_vulkan_state = push_array(arena, R_Vulkan_State, 1);
    r_vulkan_state->arena = arena;
    r_vulkan_state->enable_validation_layer = debug;
    Temp temp = scratch_begin(0,0);

    // Now, to create an instance we'll first have to fill in a struct with 
    //      some information about our application.
    // This data is technically optional, but it may provide some useful information 
    //      to the driver in order to optimize our specific application 
    //      (e.g. because it uses a well-known graphics engine with certain special behavior). 
    VkApplicationInfo app_info = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = app_name,
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName        = "Custom",
        .engineVersion      = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion         = VK_API_VERSION_1_0,
        .pNext              = NULL, // point to extension information
    };

    // Instance extensions info
    U64 enabled_ext_count;
    char **enabled_ext_names;
    {
        // NOTE(@k): instance exntesions is not the same as the physical device extensions
        U32 ext_count;
        vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);
        VkExtensionProperties extensions[ext_count]; 
        vkEnumerateInstanceExtensionProperties(NULL, &ext_count, extensions); /* query the extension details */

        printf("%d extensions supported\n", ext_count);
        for(U64 i = 0; i < ext_count; i++)
        {
            printf("[%3ld]: %s [%d] is supported\n", i, extensions[i].extensionName, extensions[i].specVersion);
        }

        // Required Extensions
        // Glfw required instance extensions
        U64 required_inst_ext_count = 2;
        const char *required_inst_exts[required_inst_ext_count];
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
        Assert(found == required_inst_ext_count);

        enabled_ext_count = required_inst_ext_count;
        // NOTE(@k): add one for optional debug extension
        enabled_ext_names = push_array(temp.arena, char *, required_inst_ext_count + 1);

        for(U64 i = 0; i < required_inst_ext_count; i++)
        {
            enabled_ext_names[i] = (char *)required_inst_exts[i];
        }

        if(r_vulkan_state->enable_validation_layer)
        {
            enabled_ext_names[enabled_ext_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        }

    }

    /////////////////////////////////////////////////////////////////////////////////
    //~ Validation Layer

    // All of the useful standard validation layer is bundle into a layer included in
    //      the SDK that is known as VK_LAYER_KHRONOS_validation 
    U64 layer_count = 0;
    const char *layers[] = { "VK_LAYER_KHRONOS_validation" };
    if(r_vulkan_state->enable_validation_layer)
    {
        layer_count = 1;
        U32 count;
        VK_Assert(vkEnumerateInstanceLayerProperties(&count, NULL), "failed to enumerate instance layer properties");
        VkLayerProperties available_layers[count];
        VK_Assert(vkEnumerateInstanceLayerProperties(&count, available_layers), "failed to enumerate instance layer properties");

        bool support_validation_layer;
        for(U64 i = 0; i < count; i++)
        {
            if(strcmp(available_layers[i].layerName, layers[0]))
            {
                support_validation_layer = true;
                break;
            }
        }
        // TODO(@k): handle it later
        // Assert(support_validation_layer);
    }

    /////////////////////////////////////////////////////////////////////////////////
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
        VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {
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

        if(r_vulkan_state->enable_validation_layer)
        {
            create_info.enabledLayerCount   = layer_count;
            create_info.ppEnabledLayerNames = layers;
            create_info.pNext               = &debug_messenger_create_info;
        }

        VK_Assert(vkCreateInstance(&create_info, NULL, &r_vulkan_state->instance), "Failed to create instance");

        // This struct should be passed to the vkCreateDebugUtilsMessengerEXT function to create the VkDebugUtilsMessengerEXT object
        // Unfortunately, because this function is an extension function, it is not automatically loaded, we have to load it ourself
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(r_vulkan_state->instance, "vkCreateDebugUtilsMessengerEXT");
        AssertAlways(vkCreateDebugUtilsMessengerEXT != NULL);

        r_vulkan_state->vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(r_vulkan_state->instance, "vkDestroyDebugUtilsMessengerEXT");
        AssertAlways(r_vulkan_state->vkDestroyDebugUtilsMessengerEXT != NULL);

        /* VK_ERROR_EXTENSION_NOT_PRESENT */
        VK_Assert(vkCreateDebugUtilsMessengerEXT(r_vulkan_state->instance, &debug_messenger_create_info, NULL, &r_vulkan_state->debug_messenger), "Failed to create debug_messenger");
    }

    /////////////////////////////////////////////////////////////////////////////////
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

    /////////////////////////////////////////////////////////////////////////////////
    // Pick the physical device
    // This object will be implicitly destroyed when the VkInstance is destroyed
    //      so we won't need to do anything new in the "Cleanup" section
#define REQUIRED_PHYSICAL_DEVICE_EXTENSIONS_COUNT 1
    const char *enabled_pdevice_ext_names[REQUIRED_PHYSICAL_DEVICE_EXTENSIONS_COUNT] = {
        // It should be noted that the availablility of a presentation queue, implies
        //      that the swap chain extension must be supported, and vice vesa
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    {
        U32 pdevice_count = 0;
        vkEnumeratePhysicalDevices(r_vulkan_state->instance, &pdevice_count, NULL);
        VkPhysicalDevice pdevices[pdevice_count];
        vkEnumeratePhysicalDevices(r_vulkan_state->instance, &pdevice_count, pdevices);


        int best_score = 0;
        r_vulkan_state->gpu.h = 0;
        for(U64 i = 0; i < pdevice_count; i++)
        {
            int score = 0;
            VkPhysicalDeviceProperties properties;
            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceProperties(pdevices[i], &properties);
            vkGetPhysicalDeviceFeatures(pdevices[i], &features);

            U32 ext_count;
            vkEnumerateDeviceExtensionProperties(pdevices[i], NULL, &ext_count, NULL);
            VkExtensionProperties extensions[ext_count];
            vkEnumerateDeviceExtensionProperties(pdevices[i], NULL, &ext_count, extensions);

            // Check if current device supports all the required physical device extensions
            U64 found = 0;
            for(U64 i = 0; i < REQUIRED_PHYSICAL_DEVICE_EXTENSIONS_COUNT; i++)
            {
                for(U64 j = 0; j < ext_count; j++)
                {
                    if(strcmp(extensions[j].extensionName, enabled_pdevice_ext_names[i]))
                    {
                        found++;
                        break;
                    } 
                }
            }
            if(found != REQUIRED_PHYSICAL_DEVICE_EXTENSIONS_COUNT) continue;

            // Querying details of swap chain support
            // Just checking if a swap chain is avaiable is not sufficient  
            //      because it may not actually be compatible with our window surface
            // Creating a swap chain also involves a lot more settings than instance and device creation
            //      so we need to query for some more details before we're able to proceed
            //
            // There are basically three kinds of properties we need to check:
            // 1. Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
            // 2. Surface formats (pixel format, color space)
            // 3. Available presentation modes

            VkSurfaceCapabilitiesKHR surface_caps;
            // This function takes the specified **VkPhysicalDevice** and **VkSurfaceKHR window surface** into account when determining the supported capabilities.
            // All of the support querying functions have these two as first parameters, because they are the **core components** of the swap chain
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevices[i], surface.h, &surface_caps);

            U32 surface_format_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(pdevices[i], surface.h, &surface_format_count, NULL);
            VkSurfaceFormatKHR surface_formats[surface_format_count];
            vkGetPhysicalDeviceSurfaceFormatsKHR(pdevices[i], surface.h, &surface_format_count, surface_formats);

            U32 surface_present_mode_count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(pdevices[i], surface.h, &surface_present_mode_count, NULL);
            VkPresentModeKHR surface_present_modes[surface_present_mode_count];
            vkGetPhysicalDeviceSurfacePresentModesKHR(pdevices[i], surface.h, &surface_present_mode_count, surface_present_modes);

            // For now, swap chain support is sufficient if there is at least one supported image format and one supported presentation mode given the window surface we have
            if(surface_format_count == 0 || surface_present_mode_count == 0) continue;

            // If application can't function without geometry shaders
            if(features.geometryShader == VK_FALSE) continue;
            // We want Anisotropy
            if(features.samplerAnisotropy == VK_FALSE) continue;
            if(features.independentBlend == VK_FALSE)  continue;
            if(features.fillModeNonSolid == VK_FALSE)  continue;

            if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 300;
            score += properties.limits.maxImageDimension2D;

            if(score > best_score)
            {
                best_score = score;
                r_vulkan_state->gpu.h = pdevices[i];
                r_vulkan_state->gpu.properties = properties;
                r_vulkan_state->gpu.features = features;

                surface.caps = surface_caps;

                assert(surface_format_count <= MAX_SURFACE_FORMAT_COUNT);
                surface.format_count = surface_format_count;
                memcpy(surface.formats, surface_formats, sizeof(VkSurfaceFormatKHR) * surface_format_count);

                assert(surface_present_mode_count <= MAX_SURFACE_PRESENT_MODE_COUNT);
                surface.prest_mode_count = surface_present_mode_count;
                memcpy(surface.prest_modes, surface_present_modes, sizeof(VkPresentModeKHR) * surface_present_mode_count);
            }
        }

        AssertAlways(r_vulkan_state->gpu.h != 0 && "No suitable physical device was founed");
        vkGetPhysicalDeviceMemoryProperties(r_vulkan_state->gpu.h, &r_vulkan_state->gpu.mem_properties);
        printf("Device %s is selected\n", r_vulkan_state->gpu.properties.deviceName);

        r_vulkan_state->gpu.depth_image_format = r_vulkan_dep_format();
    }

    /////////////////////////////////////////////////////////////////////////////////
    // Queue families information

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

        bool has_graphic_queue_family = false;

        // Since the presentation is a queue-specific feature, we would need to find a queue
        //      family that supports presenting to the surface we created
        // It's actually possible that the queue families supporting drawing commands 
        //      (aka graphic queue family) and the ones supporting presentation do not overlap
        // Therefore we have to take into account that there could be a distinct presentation queue
        bool has_present_queue_family = false;

        U32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(r_vulkan_state->gpu.h, &queue_family_count, NULL);
        VkQueueFamilyProperties queue_family_properties[queue_family_count];
        vkGetPhysicalDeviceQueueFamilyProperties(r_vulkan_state->gpu.h, &queue_family_count, queue_family_properties);

        for(U64 i = 0; i < queue_family_count; i++)
        {
            if(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                r_vulkan_state->gpu.gfx_queue_family_index = i;
                has_graphic_queue_family = true;
            }

            VkBool32 present_supported = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(r_vulkan_state->gpu.h, i, surface.h, &present_supported);
            if(present_supported == VK_TRUE)
            {
                r_vulkan_state->gpu.prest_queue_family_index = i;
                has_present_queue_family = true;
            }

            // Note that it's very likely that these end up being the same queue family after all
            //      but throughout the program we will treat them as if they were seprate queues for a uniform approach.
            // TODO(@k): Nevertheless, we would prefer a physical device that supports drawing and presentation in the same queue for improved performance.
            if(has_graphic_queue_family && has_present_queue_family) break;

        }
        Assert(has_graphic_queue_family == true);
        Assert(has_present_queue_family == true);
    }

    /////////////////////////////////////////////////////////////////////////////////
    // Logical device and queus

    // After selecting a physical device to use we need to set up a logical device to interface with it.
    // The logical device creation process is similar to the instance creation process and describes the features we want to use
    // We also need to specify which queues to create nwo that we've queried which queue families are available
    // You can even create multiple logical devices from the same physical device if you have varying requirements
    // Create one graphic queue
    // The currently available drivers will only allow you to create a small number of queues for each queue family
    // And you don't really need more than one. That's because you can create all of 
    //      the commands buffers on multiple threads and then submit them all at once on the main thread with a single low-overhead call
    {
        const F32 graphic_queue_priority = 1.0f;
        const F32 present_queue_priority = 1.0f;

        // If a queue family both support drawing and presentation, could we just create one queue
        U32 num_queues_to_create = r_vulkan_state->gpu.gfx_queue_family_index == r_vulkan_state->gpu.prest_queue_family_index ? 1 : 2;
        VkDeviceQueueCreateInfo queue_create_infos[2] = {
            // graphic queue
            {
                .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = r_vulkan_state->gpu.gfx_queue_family_index,
                .queueCount       = 1,
                .pQueuePriorities = &graphic_queue_priority,
            },
            // present queue, this second create info will be ignored if num_queues_to_create is 1
            {
                .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = r_vulkan_state->gpu.prest_queue_family_index,
                .queueCount       = 1,
                .pQueuePriorities = &present_queue_priority,
            },
        };

        // Specifying used device features, don't need anything special for now, leave everything to VK_FALSE
        VkPhysicalDeviceFeatures required_device_features = { VK_FALSE };
        required_device_features.samplerAnisotropy = VK_TRUE;
        required_device_features.independentBlend  = VK_TRUE;
        required_device_features.fillModeNonSolid  = VK_TRUE;

        // Create the logical device
        VkDeviceCreateInfo create_info = {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pQueueCreateInfos       = queue_create_infos,
            .queueCreateInfoCount    = num_queues_to_create,
            .pEnabledFeatures        = &required_device_features,
            .enabledLayerCount       = 0,
            .enabledExtensionCount   = REQUIRED_PHYSICAL_DEVICE_EXTENSIONS_COUNT,
            .ppEnabledExtensionNames = enabled_pdevice_ext_names,
        };

        // Device specific extension VK_KHR_swapchain, which allows you to present rendered images from that device to windows 
        // It's possible that there are Vulkan devices in the system that lack this ability, for example because they only support compute operations
        if(r_vulkan_state->enable_validation_layer)
        {
            // Previous implementations of Vulkan made a distinction between instance and device specific validation layers, but this is no longer the case
            // That means that the enabledLayerCount and ppEnabledLayerNames fields of VkDeviceCreateInfo are ignored by up-to-date implementations. 
            // However, it is still a good idea to set them anyway to be compatible with older implementations
            create_info.enabledLayerCount   = layer_count;
            create_info.ppEnabledLayerNames = layers;
        }
        VK_Assert(vkCreateDevice(r_vulkan_state->gpu.h, &create_info, NULL, &r_vulkan_state->device.h), "Failed to create logical device");

        // Retrieving queue handles
        // The queues are automatically created along with the logical device, but we don't have a handle to itnerface with them yet
        // Also, device queues are implicitly cleaned up when the device is destroyed, so we don't need to do anything in cleanup
        // VkQueue graphics_queue;
        // The third parameter is queu index, since we're only creating a single queue from this family, we'll simply use index 0
        vkGetDeviceQueue(r_vulkan_state->device.h, r_vulkan_state->gpu.gfx_queue_family_index, 0, &r_vulkan_state->device.gfx_queue);
        // VkQueue present_queue;
        // If the queue families are the same, then those two queue handler will be the same
        vkGetDeviceQueue(r_vulkan_state->device.h, r_vulkan_state->gpu.prest_queue_family_index, 0, &r_vulkan_state->device.prest_queue);
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
            // TODO(@k): this part don't make too much sense to me
            // We will be recording a command buffer every frame, so we want to be able to reset and re-record over it
            // Thus, we need to set the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag bit for our command pool
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = r_vulkan_state->gpu.gfx_queue_family_index,
        };
        // Command buffers are executed by submitting them on one of the device queues, like the graphcis and presentation queues we retrieved
        // Each command pool can only allocate command buffers that are submitted on a single type of queue
        // We're going to record commands for drawing, which is why we've chosen the graphcis queue family
        VK_Assert(vkCreateCommandPool(r_vulkan_state->device.h, &create_info, NULL, &r_vulkan_state->cmd_pool), "Failed to create command pool");

        VkCommandBufferAllocateInfo alloc_info = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool        = r_vulkan_state->cmd_pool,
            .commandBufferCount = 1,
        };

        VK_Assert(vkAllocateCommandBuffers(r_vulkan_state->device.h, &alloc_info, &r_vulkan_state->oneshot_cmd_buf), "Failed to allocate command buffer");
    }

    // Create samplers
    {
        r_vulkan_state->samplers[R_Tex2DSampleKind_Nearest] = r_vulkan_sampler2d(R_Tex2DSampleKind_Nearest);
        r_vulkan_state->samplers[R_Tex2DSampleKind_Linear]  = r_vulkan_sampler2d(R_Tex2DSampleKind_Linear);
    }

    /////////////////////////////////////////////////////////////////////////////////
    // Load shader modules

    // Shader modules are just a thin wrapper around the shader bytecode that we've previously loaded from a file and the functions defined in it
    // The compilation and linking of the SPIR-V bytecode to machine code fro execution by the GPU doesn't happen until the graphics pipeline is created
    // That means that we're allowed to destroy the shader modules again as soon as pipeline creation is finished
    // The one catch here is that the size of the bytecode is specified in bytes, but the bytecode pointer is uint32_t pointer rather than a char pointer
    // You also need to ensure that the data satisfies the alignment requirements of uin32_t 
    for(U64 kind = 0; kind < R_Vulkan_VShadKind_COUNT; kind++)
    {
        VkShaderModule *vshad_mo = &r_vulkan_state->vshad_modules[kind];
        String8 vshad_path;
        switch(kind)
        {
            case (R_Vulkan_VShadKind_Rect):           {vshad_path = str8_lit("src/render/vulkan/shader/rect_vert.spv");}break;
            case (R_Vulkan_VShadKind_MeshDebug):      {vshad_path = str8_lit("src/render/vulkan/shader/mesh_debug_vert.spv");}break;
            case (R_Vulkan_VShadKind_Mesh):           {vshad_path = str8_lit("src/render/vulkan/shader/mesh_vert.spv");}break;
            case (R_Vulkan_VShadKind_Geo3DComposite): {vshad_path = str8_lit("src/render/vulkan/shader/geo3d_composite_vert.spv");}break;
            case (R_Vulkan_VShadKind_Finalize):       {vshad_path = str8_lit("src/render/vulkan/shader/finalize_vert.spv");}break;
            default:                                  {InvalidPath;}break;
        }
        long vshad_size = 0;
        U8 *vshad_code  = 0;
        FileReadAll(temp.arena, vshad_path, &vshad_code, &vshad_size);

        Assert(vshad_size != 0); Assert(vshad_code != NULL);
        VkShaderModuleCreateInfo create_info = {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = vshad_size,
            .pCode    = (U32 *)vshad_code,
        };
        VK_Assert(vkCreateShaderModule(r_vulkan_state->device.h, &create_info, NULL, vshad_mo), "Failed to create shader module");
    }
    for(U64 kind = 0; kind < R_Vulkan_FShadKind_COUNT; kind++)
    {
        VkShaderModule *fshad_mo = &r_vulkan_state->fshad_modules[kind];
        String8 fshad_path;
        switch(kind)
        {
            case (R_Vulkan_FShadKind_Rect):           {fshad_path = str8_lit("src/render/vulkan/shader/rect_frag.spv");}break;
            case (R_Vulkan_FShadKind_MeshDebug):      {fshad_path = str8_lit("src/render/vulkan/shader/mesh_debug_frag.spv");}break;
            case (R_Vulkan_FShadKind_Mesh):           {fshad_path = str8_lit("src/render/vulkan/shader/mesh_frag.spv");}break;
            case (R_Vulkan_FShadKind_Geo3DComposite): {fshad_path = str8_lit("src/render/vulkan/shader/geo3d_composite_frag.spv");}break;
            case (R_Vulkan_FShadKind_Finalize):       {fshad_path = str8_lit("src/render/vulkan/shader/finalize_frag.spv");}break;
            default:                                  {InvalidPath;}break;
        }
        long fshad_size = 0;
        U8 *fshad_code  = 0;
        FileReadAll(temp.arena, fshad_path, &fshad_code, &fshad_size);
        Assert(fshad_size != 0); Assert(fshad_code != NULL);
        VkShaderModuleCreateInfo create_info = {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = fshad_size,
            .pCode    = (U32 *)fshad_code,
        };
        VK_Assert(vkCreateShaderModule(r_vulkan_state->device.h, &create_info, NULL, fshad_mo), "Failed to create shader module");
    }

    // Create set layouts
    /////////////////////////////////////////////////////////////////////////////////

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
        bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        // NOTE(@k): The pImmutableSampers field is only relevant for image sampling related descriptors
        bindings[0].pImmutableSamplers = NULL;

        // All of the descriptor bindings are combined into a single VkDescriptorSetLayout object
        VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        create_info.bindingCount = 1;
        create_info.pBindings    = bindings;
        VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->device.h, &create_info, NULL, &set_layout->h), "Failed to create descriptor set layout");

    }
    // R_Vulkan_DescriptorSetKind_UBO_Mesh
    {
        R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Mesh];
        VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
        set_layout->bindings = bindings;
        set_layout->binding_count = 1;

        bindings[0].binding            = 0;
        bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount    = 1;
        bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[0].pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        create_info.bindingCount = 1;
        create_info.pBindings    = bindings;
        VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->device.h, &create_info, NULL, &set_layout->h), "Failed to create descriptor set layout");
    }
    // R_Vulkan_DescriptorSetKind_Storage_Mesh
    {
        R_Vulkan_DescriptorSetLayout *set_layout = &r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Storage_Mesh];
        VkDescriptorSetLayoutBinding *bindings = push_array(r_vulkan_state->arena, VkDescriptorSetLayoutBinding, 1);
        set_layout->bindings = bindings;
        set_layout->binding_count = 1;

        bindings[0].binding            = 0;
        bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount    = 1;
        bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[0].pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        create_info.bindingCount = 1;
        create_info.pBindings    = bindings;
        VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->device.h, &create_info, NULL, &set_layout->h), "Failed to create descriptor set layout");
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
        bindings[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0].pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        create_info.bindingCount = 1;
        create_info.pBindings    = bindings;
        VK_Assert(vkCreateDescriptorSetLayout(r_vulkan_state->device.h, &create_info, NULL, &set_layout->h), "Failed to create descriptor set layout");
    }

    // Create backup/default texture
    // U32 backup_texture_data[] = {
    //         0xff00ffff, 0x330033ff,
    //         0x330033ff, 0xff00ffff,
    // };

    U32 backup_texture_data[] = {
        0xFFFF00FF, 0xFF330033,
        0xFF330033, 0xFFFF00FF,
    };
    r_vulkan_state->backup_texture = r_tex2d_alloc(R_ResourceKind_Static, R_Tex2DSampleKind_Nearest, v2s32(2,2), R_Tex2DFormat_RGBA8, backup_texture_data);
    scratch_end(temp);
}

internal R_Handle
r_window_equip(OS_Handle wnd_handle)
{
    R_Handle ret = {0};
    R_Vulkan_Window *window = r_vulkan_state->first_free_window;

    if(window == 0)
    {
        window = push_array(r_vulkan_state->arena, R_Vulkan_Window, 1);
        window->arena = arena_alloc();
    }
    else
    {
        U64 gen = window->generation;
        SLLStackPop(r_vulkan_state->first_free_window);
        MemoryZeroStruct(window);
        window->generation = gen;
    }

    // Fill the basic
    window->generation++;
    window->os_wnd = wnd_handle;

    R_Vulkan_Surface *surface = &window->surface;
    surface->h = os_vulkan_surface_from_window(wnd_handle, r_vulkan_state->instance);
    r_vulkan_surface_update(surface);

    // Create bag
    window->bag = r_vulkan_bag(window, surface, NULL);

    // Create renderpasses
    window->rendpass_grp = r_vulkan_rendpass_grp(window, window->bag->swapchain.format, NULL);
    r_vulkan_rendpass_grp_submit(window->bag, window->rendpass_grp);

    // Command buffers for frames
    {
        // Command buffers will be automatically freed when their command pool is destroyed, so we don't need explicit cleanup
        // Command buffers are allocated with the vkAllocateCommandBuffers function
        //      which takes a VkCommandBufferAllocateInfo struct as parameter that specifies the command pool and number of buffers to allcoate
        VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
        VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        alloc_info.commandPool        = r_vulkan_state->cmd_pool;
        // The level parameter specifies if the allcoated command buffers are primary or secondary command buffers
        // 1. VK_COMMAND_BUFFER_LEVEL_PRIMARY:   can be submitted to a queue for execution, but cannot be called from other command buffers
        // 2. VK_COMMAND_BUFFER_LEVEL_SECONDARY: cannot be submitted directly, but can be called from primary command buffers, useful to reuse common operations
        alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
        VK_Assert(vkAllocateCommandBuffers(r_vulkan_state->device.h, &alloc_info, command_buffers), "Failed to allocate command buffer");

        for(U64 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            window->frames[i].cmd_buf = command_buffers[i];

            // Create the synchronization objects for frames
            // One semaphore to signal that an image has been acquired from the swapchain and is ready for rendering
            // Another one to signal that rendering has finished and presentation can happen
            // One fence to make sure only one frame is rendering at a time
            for(U64 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                window->frames[i].img_acq_sem   = r_vulkan_semaphore(r_vulkan_state->device.h);
                window->frames[i].rend_comp_sem = r_vulkan_semaphore(r_vulkan_state->device.h);
                window->frames[i].inflt_fence   = r_vulkan_fence(r_vulkan_state->device.h);
            }

            // Create uniform buffers
            for(U64 kind = 0; kind < R_Vulkan_UniformTypeKind_COUNT; kind++)
            {
                // TODO(@k): not ideal, since mesh uniform wouldn't grow
                window->frames[i].uniform_buffers[kind] = r_vulkan_uniform_buffer_alloc(kind, 900);
            }

            // Create storage buffers
            for(U64 kind = 0; kind < R_Vulkan_StorageTypeKind_COUNT; kind++)
            {
                window->frames[i].storage_buffers[kind] = r_vulkan_storage_buffer_alloc(kind, 3000);
            }

            // Create instance buffers
            /////////////////////////////////////////////////////////////////

            R_Vulkan_Buffer *buffers[2] = {
                &window->frames[i].inst_buffer_rect,
                &window->frames[i].inst_buffer_mesh,
            };

            for(U64 i = 0; i < 2; i++)
            {
                R_Vulkan_Buffer *buffer = buffers[i];
                VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
                create_info.size        = MB(16);
                create_info.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                VK_Assert(vkCreateBuffer(r_vulkan_state->device.h, &create_info, NULL, &buffer->h), "Failed to create vk buffer");

                VkMemoryRequirements mem_requirements;
                vkGetBufferMemoryRequirements(r_vulkan_state->device.h, buffer->h, &mem_requirements);
                buffer->size = mem_requirements.size;

                VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
                VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                alloc_info.allocationSize = mem_requirements.size;
                alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);
                
                VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &buffer->memory), "Failed to allocate buffer memory");
                VK_Assert(vkBindBufferMemory(r_vulkan_state->device.h, buffer->h, buffer->memory, 0), "Failed to bind buffer memory");
                Assert(buffer->size != 0);
                VK_Assert(vkMapMemory(r_vulkan_state->device.h, buffer->memory, 0, buffer->size, 0, &buffer->mapped), "Failed to map buffer memory");
            }
        }
    }
    ret = r_vulkan_handle_from_window(window);
    return ret;
}

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
    R_Vulkan_Window *wnd = (R_Vulkan_Window *)handle.u64[0];
    // TODO(k): do we need this?
    // if(wnd->generation != handle.u64[1]) {
    //         return NULL;
    // }
    AssertAlways(wnd->generation == handle.u64[1]);
    return wnd;
}

internal R_Handle
r_vulkan_handle_from_buffer(R_Vulkan_Buffer *buffer)
{
    R_Handle handle = {0};
    handle.u64[0] = (U64)buffer;
    handle.u64[1] = buffer->generation;
    return handle;
}

internal S32
r_vulkan_memory_index_from_type_filer(U32 type_filter, VkMemoryPropertyFlags properties)
{
    // The VkMemoryRequirements struct has three fields
    // 1.           size: the size of the required amount of memory in bytes, may differ from vertex_buffer.size, e.g. (60 requested vs 64 got, alignment is 16)
    // 2.      alignment: the offset in bytes where the buffer begins in the allocated region of memory, depends on vertex_buffer.usage and vertex_buffer.flags
    // 3. memoryTypeBits: bit field of the memory types that are suitable for the buffer
    // Graphics cards can offer different tyeps of memory to allcoate from
    // Each type of memory varies in terms of allowed operations and performance characteristics
    // We need to combine the requirements of the buffer and our own application requirements to find the right type of memory to use
    // First we need to query info about the available types of memory using vkGetPhysicalDeviceMemoryProperties
    // TODO(@k): we should query mem_properties once in the initial fn
    // The VkPhyscicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps
    // Memory heaps are distinct memory resources like didicated VRAM and swap space in RAM for when VRAM runs out
    // The different types of memory exist within these heaps
    // TODO(k): Right now we'll only concern ourselves with the type of memory and not the heap it comes from, but you can imagine that this can affect performance
    S32 ret = -1;
    VkMemoryType *mem_types = r_vulkan_state->gpu.mem_properties.memoryTypes;
    U32 mem_type_count = r_vulkan_state->gpu.mem_properties.memoryTypeCount;
    for(U64 i = 0; i < mem_type_count; i++)
    {
        // However, we're not just interested in a memory type that is suitable for the vertex buffer
        // We also need to be able to write our vertex data to that memory
        // The memoryTypes array consist of VkMemoryType structs that specify the heap and properties of each type of memory
        // The properties define special features of the memory, like being able to map it so we can write to it from the CPU
        // This property is indicated with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, but we also need to the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property
        if((type_filter & (1<<i)) && (mem_types[i].propertyFlags & properties) == properties) { ret = i; }
    }
    AssertAlways(ret != -1);
    return ret;
}

internal R_Vulkan_Swapchain
r_vulkan_swapchain(R_Vulkan_Surface *surface, OS_Handle os_wnd, VkFormat format, VkColorSpaceKHR color_space, R_Vulkan_Swapchain *old_swapchain)
{
    R_Vulkan_Swapchain swapchain = {0};
    swapchain.format             = format;
    swapchain.color_space        = color_space;

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
    // VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    // TODO(@k): select 4 in production
    VkPresentModeKHR preferred_prest_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    // TODO: is this working?
    VkPresentModeKHR selected_prest_mode  = VK_PRESENT_MODE_IMMEDIATE_KHR;
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

    // Swap extent
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
        Rng2F32 client_rect = os_client_rect_from_window(os_wnd);
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

    // *imageArrayLayers*
    // The imageArrayLayers specifies the amount of layers each image consists of
    // This is always 1 unless you are developing a stereoscopic 3D application
    // TODO(@k): kind of confused here, come back later
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
    VkSwapchainCreateInfoKHR create_info = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = surface->h,
        .minImageCount    = min_swapchain_image_count,
        .imageFormat      = swapchain.format,
        .imageColorSpace  = swapchain.color_space,
        .imageExtent      = swapchain.extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform     = surface->caps.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = selected_prest_mode,
        .clipped          = VK_TRUE,
        .oldSwapchain     = VK_NULL_HANDLE,
    };
    if(old_swapchain != 0) create_info.oldSwapchain = old_swapchain->h;

    // We need to specify how to handle swap chain images that will be used across multiple queue families
    // That will be the case in our application if the grpahics queue family is different from the presentation queue
    // We'll be drawing on the images in the swap chain from the graphics queue and then submitting them on the presentation queue
    // There are two ways to handle images that are accessed from multiple type of queues
    // 1. VK_SHARING_MODE_EXCLUSIVE: an image is owned by one queue family at a time and ownership must be explicitly transfered before using
    //    it in another queue family. This option offers the best performance
    // 2. VK_SHARING_MODE_CONCURRENT: images can be used across multiple queue families without explicit ownership transfers
    U32 queue_family_indices[2] = { r_vulkan_state->gpu.gfx_queue_family_index, r_vulkan_state->gpu.prest_queue_family_index };
    if(r_vulkan_state->gpu.gfx_queue_family_index != r_vulkan_state->gpu.prest_queue_family_index)
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

    VK_Assert(vkCreateSwapchainKHR(r_vulkan_state->device.h, &create_info, NULL, &swapchain.h), "Failed to create swapchain");

    // Retrieving the swap chain images
    // The swapchain has been created now, so all the remains is retrieving the handles of the [VkImage]s in it
    // We will reference these during rendering operations
    vkGetSwapchainImagesKHR(r_vulkan_state->device.h, swapchain.h, &swapchain.image_count, NULL);
    assert(swapchain.image_count <= MAX_IMAGE_COUNT);
    vkGetSwapchainImagesKHR(r_vulkan_state->device.h, swapchain.h, &swapchain.image_count, swapchain.images);

    // Create image views
    // To use any VkImage, including those in the swap chain, in the render pipeline we have to a VkImageView object
    // An image view is quite literally a view into an image.
    // It describes how to access the image and which part of the image to access
    //      for example if it should be treated as 2D texture depth texture without any mipmapping levels
    // Here, we are create a basic image view for every image in the swapchain so that we can use them as color targets later on
    // VkImageView swapchain_image_views[swapchain_image_count];
    for(U64 i = 0; i < swapchain.image_count; i++)
    {
        VkImageViewCreateInfo create_info = {
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

        VK_Assert(vkCreateImageView(r_vulkan_state->device.h, &create_info, NULL, &swapchain.image_views[i]), "Failed to create image view");
        // Unlike images, the image views were explicitly created by us, so we need to add a similar loop to destroy them again at the end of the program
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
        // For the color space, we'll use SRGB if it's available, because it ressults in more accurate perceived colors.
        // It is also pretty much the standard color space for images, like the textures we'll use later on. Because of that we should also 
        // use an SRGB color format, of which one of the most common ones is VK_FORMAT_B8G8R8A8_SRGB
        if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            *format      = formats[i].format;
            *color_space = formats[i].colorSpace;
            break;
        }
    }
}

internal VkFormat
r_vulkan_dep_format()
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
        vkGetPhysicalDeviceFormatProperties(r_vulkan_state->gpu.h, format_candidates[i], &props);

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

internal R_Vulkan_RenderPassGroup *
r_vulkan_rendpass_grp(R_Vulkan_Window *window, VkFormat color_format, R_Vulkan_RenderPassGroup *old)
{
    // TODO: we never free the renderpass_grp
    R_Vulkan_RenderPassGroup *rendpass_grp = window->first_free_rendpass_grp;

    U64 gen = 0;
    if(old != 0) { gen = old->generation + 1; }

    if(rendpass_grp == 0)
    {
        rendpass_grp = push_array(window->arena, R_Vulkan_RenderPassGroup, 1);
    }
    rendpass_grp->generation = gen;

    for(U64 kind = 0; kind < R_Vulkan_RenderPassKind_COUNT; kind++)
    {
        R_Vulkan_RenderPass *rendpass = &rendpass_grp->passes[kind];
        R_Vulkan_RenderPass *old_rendpass = 0;
        if(old) old_rendpass = &old->passes[kind];
        switch(kind)
        {
            case R_Vulkan_RenderPassKind_Rect:
            {
                // Output to stage_color_att
                U64                     attachment_count = 1;
                U64                     subpass_count    = 1;
                U64                     dep_count        = 1;
                VkAttachmentDescription att_descs[attachment_count] = {};
                VkSubpassDescription    subpasses[subpass_count] = {};
                VkSubpassDependency     deps[dep_count] = {};

                VkRenderPassCreateInfo create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
                // Before we can finish creating the pipeline, we need to tell Vulkan about the framebuffer attachments that will be used while rendering
                // We need specify how many color and depth buffers there will be, how many samples to use for each of them and how their contents should be
                // handled throughout the rendering operations
                // All of this information is wrapped in a render pass object
                // In our case we'll have just a single color buffer attachment represented by one of the images from the swap chain

                // Color attachment
                // The format of the color attachment should match the format of the swap chain images, and we're not doing anything with multisampling yet, so we'll stick to 1 sample
                att_descs[0].format  = color_format;
                att_descs[0].samples = VK_SAMPLE_COUNT_1_BIT;
                // The loadOp and storeOp dtermine what to do with the data in the attachment before rendering and after rendering. We have the following choices for loadOp
                // 1. VK_ATTACHMENT_LOAD_OP_LOAD:      preserve the existing contents of the attachment
                // 2. VK_ATTACHMENT_LOAD_OP_CLEAR:     clear the values to a constant at the start
                // 3. VK_ATTACHMENT_LOAD_OP_DONT_CARE: existing contents are undefined; we don't care about them
                att_descs[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                // There are only two possibilities for the storeOp
                // 1. VK_ATTACHMENT_STORE_OP_STORE:     rendered contents will be stored in memory and can be read later
                // 2. VK_ATTACHMENT_STORE_OP_DONT_CARE: contents of the framebuffer will be undefined after the rendering operation
                att_descs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                // The loadOp and storeOp apply to color and depth data, and stencilLoadOp/stencilStoreOp apply to stencil data
                // Out application won't do anything with the stencil buffer, so the resutls of loading and storing are irrelevant
                att_descs[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                att_descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                // Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format, however the layout of the pixels in memory can
                // change based on what you're trying to do with an image
                // The important take away is that images need to be transitioned to specific layouts that are sutitable for the operation that they're going to be involved in next
                // The initialLayout specifies which layout the image will have before the render pass begins
                // The finalLayout specifies the layout to automatically transition to when the render pass finishes
                // Some of common layouts are:
                // 1. VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: images used as color attachment
                // 2. VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:          images to be presented in the swap chain
                // 3. VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:     images to be used as destination for a memory copy operation
                att_descs[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                att_descs[0].finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


                VkAttachmentReference refs[attachment_count];
                // Subpasses and attachment references
                // A single render pass can consist of multiple subpasses
                // Subpasses are subsequent rendering operations that depend on the contents of framebuffers in previous passes
                // For example a sequence of post-processing effects that are applied one after another
                // If you group these rendering operations into one render pass, then Vulkan is able to reorder the operations and conserve memory bandwitdh for possibly better performance
                // Every subpass references one or more of the attachments that we've described using the structure in the previous sections

                // The attachment parameter specifies which attachment to reference by its index in the attachment descriptions array
                // Our array currently only consists of a single VkAttachmentDescription, so its index is 0
                refs[0].attachment = 0;
                // The layout specifies which layout we would like the attachment to have during a subpass that uses this reference
                // Vulkan will automatically transition the attachment to this layout when the subpass is started
                // We intend to use the attachment to function as a color buffer and the VK_IAMGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout will give use the best performance as its name implies
                refs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                // Vulkan may also support compute subpasses in the future, so we have to be explicit about this being a graphics subpass
                subpasses[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpasses[0].colorAttachmentCount    = 1,
                subpasses[0].pColorAttachments       = refs;
                // Unlike the color attachments, a subpass can only use a single depth (+stencil) attachment
                // It wouldn't really make any sense to do depth tests on multiple buffers
                subpasses[0].pDepthStencilAttachment = VK_NULL_HANDLE;
                // The index of the attachment in this array is directly referenced from the fragment shader with the 
                // layout(location = 0) out vec4 outColor directive
                // The following other types of attachments can be reference by subpass:
                // 1. pInputAttachments: attachments that are read from a shader
                // 2. pResolveAttachments: attachments used for multisampling color attachments
                // 3. pDepthStencilAttachments: attachments for depth and stencil data
                // 4. pPreserveAttachments: attachments that are not used by this subpass, but for which the data must be perserved

                // Subpasses Dependencies
                // Remember that the subpasses in a render pass automatically take care of image layout transitions
                // These transitions are controlled by subpass dependencies, which specify memory and execution dependencies between subpasses
                // We have only a single subpass right now, but the operations right before and right after this subpass also count as implicit "subpasses"
                // There are two built-in dependencies that take care of the transition at the start of the render pass and at the end of the render pass, but the former does not 
                // occur at the right time. It assumes that the transition occurs at the start of the pipeline, but we haven't accquired the image yet at that point
                // There are two ways to deal with this problem
                // 1. We could change the waitStages for the imageAvailableSemaphore to VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT(deprecated) (ALL_COMMANDS in newer api) to ensure that render passes
                //    don't begin until the image is available
                // 2. We can make the render pass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage

                // The first two fields specify the indices of the dependency and the dependent subpass
                // The special value VK_SUBPASS_EXTERNAL refers to the implicit subpass before or after the render pass depending on the whether it is specified in srcSubpass or dstSubpass
                // The dstSubpass must always be higher than srcSubpass to prevent the cycles in the dependency graph (unless one of the subpasses is VK_SUBPASS_EXTERNAL)
                // The main purpose of external subpass dependencies is to deal with initialLayout and finalLayout of an attachment reference
                // If initialLayout != layout used in the first subpass, the render pass is forced to perform a layout transition (by injecting a memory barrier)
                deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                deps[0].dstSubpass = 0; /* the first subpass */
                // The next two fields specify the operations to wait on and the stages in which these operations occur
                // We need to wait for the swap chain to finish reading from the image before we can access it
                // This can be accomplished by waiting on the color attachment output stage itself
                deps[0].srcStageMask  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                deps[0].srcAccessMask = 0;
                // Basically memory barrier
                // The operations that should wait on this are in the color attachment stage and involve the writing of the color attachment
                // These settings will prevent the layout transition from happening until it's actually necessary
                deps[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                deps[0].dstAccessMask = 0;
                // There is a very similar external subpass dependency setup for finalLayout
                // If finalLayout differs from the last use in a subpass, driver will transition into the final layout automatically
                // Here you get to change dstStageMask/dstAccessMask
                // If you do nothing here, you get BOTTOM_OF_PIPE/0, which can actually be just fine

                // Creat render pass
                create_info.attachmentCount = attachment_count;
                create_info.pAttachments    = att_descs;
                create_info.subpassCount    = subpass_count;
                create_info.pSubpasses      = subpasses;
                create_info.dependencyCount = dep_count;
                create_info.pDependencies   = deps;
                VK_Assert(vkCreateRenderPass(r_vulkan_state->device.h, &create_info, NULL, &rendpass->h), "Failed to create render pass");

                // Create pipelines
                R_Vulkan_Pipeline *old_p_rect = 0;
                if(old_rendpass) {old_p_rect = &old_rendpass->pipeline.rect;};
                rendpass->pipeline.rect = r_vulkan_pipeline(R_Vulkan_PipelineKind_Rect, R_GeoTopologyKind_TriangleStrip, R_GeoPolygonKind_Fill, rendpass->h, old_p_rect);
            } break;
            case R_Vulkan_RenderPassKind_Mesh:
            {
                // Output to geo3d_color buffer, using ge3d_depth
                U64                     attachment_count = 3;
                U64                     subpass_count    = 1;
                U64                     dep_count        = 1;
                VkAttachmentDescription att_descs[attachment_count] = {};
                VkSubpassDescription    subpasses[subpass_count] = {};
                VkSubpassDependency     deps[dep_count] = {};

                VkRenderPassCreateInfo create_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };

                // Color attachment
                att_descs[0].format         = color_format;
                att_descs[0].samples        = VK_SAMPLE_COUNT_1_BIT;
                att_descs[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
                att_descs[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                att_descs[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                att_descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                att_descs[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                att_descs[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                att_descs[1].format         = VK_FORMAT_R32G32_UINT;
                att_descs[1].samples        = VK_SAMPLE_COUNT_1_BIT;
                att_descs[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
                att_descs[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                att_descs[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                att_descs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                att_descs[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                att_descs[1].finalLayout    = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

                // Depth attachment
                att_descs[2].format         = r_vulkan_state->gpu.depth_image_format;
                att_descs[2].samples        = VK_SAMPLE_COUNT_1_BIT;
                att_descs[2].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
                att_descs[2].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                att_descs[2].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                att_descs[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                att_descs[2].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                att_descs[2].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentReference refs[attachment_count];
                refs[0].attachment = 0;
                refs[0].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                refs[1].attachment = 1;
                refs[1].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                refs[2].attachment = 2;
                refs[2].layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                subpasses[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpasses[0].colorAttachmentCount    = 2,
                subpasses[0].pColorAttachments       = &refs[0];
                subpasses[0].pDepthStencilAttachment = &refs[2];

                deps[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
                deps[0].dstSubpass    = 0; /* the first subpass */
                deps[0].srcStageMask  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                deps[0].srcAccessMask = 0;
                deps[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                deps[0].dstAccessMask = 0;

                // Creat render pass
                create_info.attachmentCount = attachment_count;
                create_info.pAttachments    = att_descs;
                create_info.subpassCount    = subpass_count;
                create_info.pSubpasses      = subpasses;
                create_info.dependencyCount = dep_count;
                create_info.pDependencies   = deps;
                VK_Assert(vkCreateRenderPass(r_vulkan_state->device.h, &create_info, NULL, &rendpass->h), "Failed to create render pass");

                // Create pipelines
                R_Vulkan_Pipeline *old_p = 0;
                // Mesh debug pipeline
                if(old_rendpass) old_p = old_rendpass->pipeline.mesh[0];
                for(U64 i = 0; i < R_GeoTopologyKind_COUNT; i++)
                {
                    for(U64 j = 0; j < R_GeoPolygonKind_COUNT; j++)
                    {
                        rendpass->pipeline.mesh[0][i*R_GeoPolygonKind_COUNT + j] = r_vulkan_pipeline(R_Vulkan_PipelineKind_MeshDebug, i, j, rendpass->h, old_p);
                    }
                }

                // Mesh pipeline
                if(old_rendpass) old_p = old_rendpass->pipeline.mesh[1];
                for(U64 i = 0; i < R_GeoTopologyKind_COUNT; i++)
                {
                    for(U64 j = 0; j < R_GeoPolygonKind_COUNT; j++)
                    {
                        rendpass->pipeline.mesh[1][i*R_GeoPolygonKind_COUNT + j] = r_vulkan_pipeline(R_Vulkan_PipelineKind_Mesh, i, j, rendpass->h, old_p);
                    }
                }
            } break;
            case R_Vulkan_RenderPassKind_Geo3DComposite:
            {
                // Output to stage buffer, using geo3d as texture
                U64                     attachment_count = 1;
                U64                     subpass_count    = 1;
                U64                     dep_count        = 1;
                VkAttachmentDescription att_descs[attachment_count] = {};
                VkSubpassDescription    subpasses[subpass_count]    = {};
                VkSubpassDependency     deps[dep_count]             = {};

                // Color attachment
                att_descs[0].format         = color_format;
                att_descs[0].samples        = VK_SAMPLE_COUNT_1_BIT;
                att_descs[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
                att_descs[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                att_descs[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                att_descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                att_descs[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                att_descs[0].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentReference refs[attachment_count];
                refs[0].attachment = 0;
                refs[0].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                subpasses[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpasses[0].colorAttachmentCount    = 1,
                subpasses[0].pColorAttachments       = refs;
                subpasses[0].pDepthStencilAttachment = VK_NULL_HANDLE;

                deps[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
                deps[0].dstSubpass    = 0; /* the first subpass */
                deps[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                deps[0].srcAccessMask = 0;
                deps[0].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                deps[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                // Create render pass
                VkRenderPassCreateInfo create_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
                create_info.attachmentCount = attachment_count;
                create_info.pAttachments    = att_descs;
                create_info.subpassCount    = subpass_count;
                create_info.pSubpasses      = subpasses;
                create_info.dependencyCount = dep_count;
                create_info.pDependencies   = deps;
                VK_Assert(vkCreateRenderPass(r_vulkan_state->device.h, &create_info, NULL, &rendpass->h), "Failed to create render pass");

                // Create pipelines
                R_Vulkan_Pipeline *old_p_geo3d_composite = 0;
                if(old_rendpass) {old_p_geo3d_composite = &old_rendpass->pipeline.geo3d_composite;};
                rendpass->pipeline.geo3d_composite = r_vulkan_pipeline(R_Vulkan_PipelineKind_Geo3DComposite, R_GeoTopologyKind_TriangleStrip, R_GeoPolygonKind_Fill, rendpass->h, old_p_geo3d_composite);
            } break;
            case R_Vulkan_RenderPassKind_Finalize:
            {
                // Output to swapchain image, read stage_color as texture 
                U64                     attachment_count = 1;
                U64                     subpass_count    = 1;
                U64                     dep_count        = 1;
                VkAttachmentDescription att_descs[attachment_count] = {};
                VkSubpassDescription    subpasses[subpass_count]    = {};
                VkSubpassDependency     deps[dep_count]             = {};
                VkRenderPassCreateInfo  create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};

                // Color attachment
                att_descs[0].format         = color_format;
                att_descs[0].samples        = VK_SAMPLE_COUNT_1_BIT;
                att_descs[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
                att_descs[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                att_descs[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                att_descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                att_descs[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                att_descs[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                VkAttachmentReference refs[attachment_count];
                refs[0].attachment = 0;
                refs[0].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                subpasses[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpasses[0].colorAttachmentCount    = 1,
                subpasses[0].pColorAttachments       = refs;
                subpasses[0].pDepthStencilAttachment = VK_NULL_HANDLE;

                deps[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
                deps[0].dstSubpass    = 0; /* the first subpass */
                deps[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                deps[0].srcAccessMask = 0;
                deps[0].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                deps[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                // Creat render pass
                create_info.attachmentCount = attachment_count;
                create_info.pAttachments    = att_descs;
                create_info.subpassCount    = subpass_count;
                create_info.pSubpasses      = subpasses;
                create_info.dependencyCount = dep_count;
                create_info.pDependencies   = deps;
                VK_Assert(vkCreateRenderPass(r_vulkan_state->device.h, &create_info, NULL, &rendpass->h), "Failed to create render pass");

                // Create pipelines
                R_Vulkan_Pipeline *old_p_finalize = 0;
                if(old_rendpass) {old_p_finalize = &old_rendpass->pipeline.finalize;};
                rendpass->pipeline.finalize = r_vulkan_pipeline(R_Vulkan_PipelineKind_Finalize, R_GeoTopologyKind_TriangleStrip, R_GeoPolygonKind_Fill, rendpass->h, old_p_finalize);
            } break;
            default: {InvalidPath;}break;
        }
    }
    return rendpass_grp;
}

internal VkSemaphore
r_vulkan_semaphore(VkDevice device)
{
        // In current version of the VK API it doesn't actually have any required fields besides sType
        VkSemaphoreCreateInfo create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkSemaphore sem;
        VK_Assert(vkCreateSemaphore(device, &create_info, NULL, &sem), "Failed to create semaphore");
        return sem;
}

internal VkFence
r_vulkan_fence(VkDevice device)
{
        VkFenceCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                // Create the fence in the signaled state, so that the first call to vkWaitForFences() returns immediately
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        VkFence fence;
        VK_Assert(vkCreateFence(device, &create_info, NULL, &fence), "Failed to create fence");
        return fence;
}

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
        VK_Assert(vkBeginCommandBuffer(cmd_buf, &begin_info), "Failed to begin command buffer");
}

internal void
r_vulkan_cmd_end(VkCommandBuffer cmd_buf, VkSubmitInfo *submit_info)
{
        VK_Assert(vkEndCommandBuffer(cmd_buf), "Failed to end command buffer");
        vkQueueSubmit(r_vulkan_state->device.gfx_queue, 1, submit_info, VK_NULL_HANDLE);
}

// Texture2D
/////////////////////////////////////////////////////////////////////////////////////////

internal R_Handle
r_tex2d_alloc(R_ResourceKind kind, R_Tex2DSampleKind sample_kind, Vec2S32 size, R_Tex2DFormat format, void *data)
{
    R_Vulkan_Tex2D *texture = r_vulkan_state->first_free_tex2d;
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
    texture->generation++;
    texture->image.extent.width  = size.x;
    texture->image.extent.height = size.y;
    texture->format = format;

    VkDeviceSize vk_image_size = size.x * size.y;
    VkFormat vk_image_format   = 0;

    switch(format)
    {
        case R_Tex2DFormat_R8:    {vk_image_format = VK_FORMAT_R8_UNORM;}break;
        case R_Tex2DFormat_RGBA8: {vk_image_format = VK_FORMAT_R8G8B8A8_SRGB; vk_image_size *= 4;}break;
        default:                  {InvalidPath;}break;
    }
    texture->image.format = vk_image_format;

    AssertAlways(kind == R_ResourceKind_Static && "Only static texture is supoorted for now");
    AssertAlways(data != 0);

    // Staging buffer
    VkBuffer       staging_buffer;
    VkDeviceMemory staging_memory;
    {
        VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        create_info.size        = vk_image_size;
        create_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_Assert(vkCreateBuffer(r_vulkan_state->device.h, &create_info, NULL, &staging_buffer), "Failed to create vk buffer");

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->device.h, staging_buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &staging_memory), "Failed to allocate buffer memory");
        VK_Assert(vkBindBufferMemory(r_vulkan_state->device.h, staging_buffer, staging_memory, 0), "Failed to bind buffer memory");

        void *mapped;
        VK_Assert(vkMapMemory(r_vulkan_state->device.h, staging_memory, 0, mem_requirements.size, 0, &mapped), "Failed to map buffer memory");
        MemoryCopy(mapped, data, vk_image_size);
        vkUnmapMemory(r_vulkan_state->device.h, staging_memory);
    }

    // Create the gpu device local buffer
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
        create_info.flags = 0; // Optional
        VK_Assert(vkCreateImage(r_vulkan_state->device.h, &create_info, NULL, &texture->image.h), "Failed to create vk buffer");

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(r_vulkan_state->device.h, texture->image.h, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &texture->image.memory), "Failed to allocate buffer memory");
        VK_Assert(vkBindImageMemory(r_vulkan_state->device.h, texture->image.h, texture->image.memory, 0), "Failed to bind buffer memory");
    }

    // Create image view
    {
        VkImageViewCreateInfo create_info = {
            .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image                           = texture->image.h,
            .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
            .format                          = vk_image_format,
            .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel   = 0,
            .subresourceRange.levelCount     = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount     = 1,
        };
        VK_Assert(vkCreateImageView(r_vulkan_state->device.h, &create_info, NULL, &texture->image.view), "Failed to create image view");
    }

    // Transition image layout 
    // We don't care about its contents before performing the copy creation
    // There is actually a special type of image layout that supports all operations, VK_IMAGE_LAYOUT_GENERAL
    // The problem with it, of course, is that it doesn't necessarily offer the best performance for any operation
    // It is required for some special cases, like using an image as both input and output, or for reading an image after it has left the preinitialized layout
    r_vulkan_image_transition(texture->image.h, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy buffer to image
    VkCommandBuffer cmd = r_vulkan_state->oneshot_cmd_buf;
    OneshotCmdScope(cmd)
    {
        VkBufferImageCopy region = {
            // Specify the byte offset in the buffer at which the pixel values start
            .bufferOffset                    = 0,
            // These two fields specify how the pixels are laid out in memory
            // For example, you could have some padding bytes between rows of the image
            // Specifying 0 for both indicates that the pixels are simply tightly packed like they are in our case
            // The imageSubresource, imageOffset and imageExtent fields indicate to which part of the image we want to copy the pixels
            .bufferRowLength                 = 0,
            .bufferImageHeight               = 0,
            .imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .imageSubresource.mipLevel       = 0,
            .imageSubresource.baseArrayLayer = 0,
            .imageSubresource.layerCount     = 1,
            // These two fields indicate to which part of the image we want to copy the pixels
            .imageOffset                     = {0, 0, 0},
            .imageExtent                     = {size.x, size.y, 1},
        };
        // The fourth parameter indicates which layout the image is currently using
        // We are assuming here that the image has already been transitioned to the layout that is optimal for copying pixels to
        // Right now we're only copying one chunk of pixels to the whole image, but it's possible to specify an array of VkBufferImageCopy to perform many different copies from this buffer to the image in on operation 
        vkCmdCopyBufferToImage(cmd, staging_buffer, texture->image.h, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    // Free staging buffer and memory
    vkDestroyBuffer(r_vulkan_state->device.h, staging_buffer, 0);
    vkFreeMemory(r_vulkan_state->device.h, staging_memory, 0);

    r_vulkan_image_transition(texture->image.h, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // TODO(@k): All of the helper functions that submit commands so far have been set up to execute synchronously by waiting for the queue to become idle
    // For practical applications it is recommended to combine these operations in a single command buffer and execute them asynchronously for higher throughput
    //     especially the transitions and copy in the create_texture_image function
    // We could experiment with this by creating a setup_command_buffer that the helper functions record commands into, and add a flish_setup_commands to execute the 
    //     the commands that have been recorded so far

    // TODO(@k): we could create two descriptor set for two types of sampler
    VkSampler *sampler = &r_vulkan_state->samplers[sample_kind];
    r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind_Tex2D, 1, 64, NULL, &texture->image.view, sampler, &texture->desc_set);

    R_Handle ret = r_vulkan_handle_from_tex2d(texture);
    return ret;
}

internal R_Handle
r_vulkan_handle_from_tex2d(R_Vulkan_Tex2D *texture)
{
    R_Handle handle = {0};
    handle.u64[0] = (U64)texture;
    handle.u64[1] = texture->generation;
    return handle;
}

internal R_Vulkan_Tex2D *
r_vulkan_tex2d_from_handle(R_Handle handle)
{
    R_Vulkan_Tex2D *texture = (R_Vulkan_Tex2D *)handle.u64[0];
    Assert(texture != 0 && texture->generation == handle.u64[1]);
    return texture;
}

//- rjf: buffers

internal R_Handle
r_buffer_alloc(R_ResourceKind kind, U64 size, void *data)
{
    R_Vulkan_Buffer *buffer = 0;

    buffer = r_vulkan_state->first_free_buffer;
    if(buffer == 0)
    {
        buffer = push_array(r_vulkan_state->arena, R_Vulkan_Buffer, 1);
    }
    else
    {
        U64 gen = buffer->generation;
        SLLStackPop(r_vulkan_state->first_free_buffer);
        MemoryZeroStruct(buffer);
        buffer->generation = gen;
    }
    buffer->generation++;

    if(kind == R_ResourceKind_Static) Assert(data != 0);

    // Fill basics
    buffer->kind = kind;
    buffer->size = size;

    // It should be noted that in a real world application, you're not supposed to actually call vkAllocateMemory for every individual buffer
    // The maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount physical device limit
    // which may be as low as 4096 even on high end hardward like an NVIDIA GTX1080
    // The right way to allocate memory for a large number of objects at the same time is to create a custom allocator that splits
    //      up a single allocation among many different objects by using the offset parameters that we've seen in many functions
    // We can either implement such an allocator ourself, or use the VulkanMemoryAllocator library provided by the GPUOpen initiative

    // TODO(@k): hate vma, remove it asap
    // Create staging buffer
    if(kind == R_ResourceKind_Static)
    {
        VkBuffer staging_buffer;
        VkDeviceMemory staging_memory;
        {
            VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            create_info.size        = size;
            create_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VK_Assert(vkCreateBuffer(r_vulkan_state->device.h, &create_info, NULL, &staging_buffer), "Failed to create vk buffer");

            VkMemoryRequirements mem_requirements;
            vkGetBufferMemoryRequirements(r_vulkan_state->device.h, staging_buffer, &mem_requirements);

            VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            alloc_info.allocationSize = mem_requirements.size;
            alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);
            
            VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &staging_memory), "Failed to allocate buffer memory");
            VK_Assert(vkBindBufferMemory(r_vulkan_state->device.h, staging_buffer, staging_memory, 0), "Failed to bind buffer memory");

            void *mapped;
            VK_Assert(vkMapMemory(r_vulkan_state->device.h, staging_memory, 0, mem_requirements.size, 0, &mapped), "Failed to map buffer memory");
            MemoryCopy(mapped, data, size);
            vkUnmapMemory(r_vulkan_state->device.h, staging_memory);
        }

        // Create buffer <GPU Device Local> 
        {
            VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            create_info.size        = size;
            create_info.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VK_Assert(vkCreateBuffer(r_vulkan_state->device.h, &create_info, NULL, &buffer->h), "Failed to create vk buffer");

            VkMemoryRequirements mem_requirements;
            vkGetBufferMemoryRequirements(r_vulkan_state->device.h, buffer->h, &mem_requirements);

            VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            alloc_info.allocationSize = mem_requirements.size;
            alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

            VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &buffer->memory), "Failed to allocate buffer memory");
            VK_Assert(vkBindBufferMemory(r_vulkan_state->device.h, buffer->h, buffer->memory, 0), "Failed to bind buffer memory");
        }

        // Copy buffer
        VkCommandBuffer cmd = r_vulkan_state->oneshot_cmd_buf;
        OneshotCmdScope(cmd)
        {
            VkBufferCopy copy_region = {
                .srcOffset = 0, // Optional
                .dstOffset = 0, // Optional
                .size      = size,
            };

            // It is not possible to specify VK_WHOLE_SIZE here unlike the vkMapMemory command
            vkCmdCopyBuffer(cmd, staging_buffer, buffer->h, 1, &copy_region);
        }

        // Free staging buffer and memory
        vkDestroyBuffer(r_vulkan_state->device.h, staging_buffer, NULL);
        vkFreeMemory(r_vulkan_state->device.h, staging_memory, NULL);
    } 
    else
    {
        VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create_info.size        = size;
        // TODO(@k): we may want to differiciate index and vertex buffer
        create_info.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_Assert(vkCreateBuffer(r_vulkan_state->device.h, &create_info, NULL, &buffer->h), "Failed to create vk buffer");

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->device.h, buffer->h, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &buffer->memory), "Failed to allocate buffer memory");
        VK_Assert(vkBindBufferMemory(r_vulkan_state->device.h, buffer->h, buffer->memory, 0), "Failed to bind buffer memory");
        Assert(buffer->size != 0);
        VK_Assert(vkMapMemory(r_vulkan_state->device.h, buffer->memory, 0, buffer->size, 0, &buffer->mapped), "Failed to map buffer memory");

        if(data != 0) { MemoryCopy(buffer->mapped, data, size); }
    }

    R_Handle ret = r_vulkan_handle_from_buffer(buffer);
    return ret;
}

/*
 * TODO(@k): refactor this code later
 * Using image memory barrier to transition image layout
 * One of the most common ways to perform layout transitions is using an image memory barrier
 * A pipeline barrier like that is geenrally used to synchronize access to resources, like ensuring that a write to a buffer completes before reading from it
 * But it can also be used to transition image layouts and transfer queue family ownership when VK_SHARING_MODE_EXCLUSIVE is used
 * There is an equivalent buffer memory barrier to do this for buffers
 */
internal void
r_vulkan_image_transition(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
    VkImageMemoryBarrier barrier = {
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
        .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel   = 0,
        .subresourceRange.levelCount     = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1,
        // Barriers are primaryly used for synchronization purpose, so you must specify which types of operations that involve the resource must 
        // happen before the barrier, and which operations that involve the resource must wait on the barrier
        // We need to do that despite already using vkQueueWaitIdle to manually synchronize
        // The right values depend on the old and new layout
        .srcAccessMask = 0, 
        .dstAccessMask = 0,
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
    if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage             = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
        // The image will be written in the same pipeline stage and subsequently read by the fragment shader, which is why we specify
        //     shader reading access in the fragment shader pipeline stage
    } 
    else if(old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage             = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else { assert(false && "Unsupported layout transition"); }

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
    VkCommandBuffer cmd = r_vulkan_state->oneshot_cmd_buf;
    OneshotCmdScope(cmd)
    {
        vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
    }

}

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
        case (R_Tex2DSampleKind_Nearest): {create_info.magFilter = VK_FILTER_NEAREST; create_info.minFilter = VK_FILTER_NEAREST;}break;
        case (R_Tex2DSampleKind_Linear):  {create_info.magFilter = VK_FILTER_LINEAR; create_info.minFilter = VK_FILTER_LINEAR;}break;
        default:                          {InvalidPath;}break;
    }

    // Note that the axes are called U, V and W instead of X, Y and Z
    //               VK_SAMPLER_ADDRESS_MODE_REPEAT: repeat the texture when going beyond the image dimension
    //      VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: like repeat mode, but inverts the coordinates to mirror the image when going beyond the dimension
    //        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: take the color of the edge cloesest to the coordinate beyond the image dimensions
    // VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: like clamp to edge, but instead uses the edge opposite the closest edge
    //      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: return a solid color when sampling beyond the dimensions of the image
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
    create_info.maxAnisotropy    = r_vulkan_state->gpu.properties.limits.maxSamplerAnisotropy;
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
    // TODO(@k): This si mainly used for percentage-closer filtering "https://developer.nvidia.com/gpugems/GPUGems/gpugems_ch11.html" on shadow maps
    // We are not using any of that
    create_info.compareEnable = VK_FALSE;
    create_info.compareOp     = VK_COMPARE_OP_ALWAYS;
    // NOTE(@k): These 4 fields apply to mipmapping
    // We will look at mipmapping in a later chapter, but basically it's another type of filter that can be applied
    create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    create_info.mipLodBias = 0.0f;
    create_info.minLod     = 0.0f;
    create_info.maxLod     = 0.0f;

    VK_Assert(vkCreateSampler(r_vulkan_state->device.h, &create_info, NULL, &sampler), "Failed to create sampler");
    // Note the sampler does not reference a VkImage anywhere
    // The sampler is a distinct object that provides an interface to extrat colors from a texture
    // It can be applied to any image you want, whether it is 1D, 2D or 3D
    // This is different from many older APIS, which combined texture images and filtering into a single state

    return sampler;
}

internal R_Vulkan_Pipeline
r_vulkan_pipeline(R_Vulkan_PipelineKind kind, R_GeoTopologyKind topology, R_GeoPolygonKind polygon, VkRenderPass renderpass, R_Vulkan_Pipeline *old)
{
    R_Vulkan_Pipeline pipeline;
    pipeline.kind = kind;

    // We will need to recreate the pipeline if the format of swapchain image changed thus changed renderpass
    VkShaderModule vshad_mo, fshad_mo;
    switch(kind)
    {
        case (R_Vulkan_PipelineKind_Rect):
        {
            vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Rect];
            fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Rect];
        }break;
        case (R_Vulkan_PipelineKind_MeshDebug):
        {
            vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_MeshDebug];
            fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_MeshDebug];
        }break;
        case (R_Vulkan_PipelineKind_Mesh):
        {
            vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Mesh];
            fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Mesh];
        }break;
        case (R_Vulkan_PipelineKind_Geo3DComposite):
        {
            vshad_mo = r_vulkan_state->vshad_modules[R_Vulkan_VShadKind_Geo3DComposite];
            fshad_mo = r_vulkan_state->fshad_modules[R_Vulkan_FShadKind_Geo3DComposite];
        }break;
        case (R_Vulkan_PipelineKind_Finalize):
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
    VkDynamicState dynamic_states[2] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates    = dynamic_states,
    };
    // And then you only need to specify their count at pipeline creation time
    // The actual viewport(s) and scissor rectangle(s) will then later be set up at drawing time
    // With dynamic state it's even possbile to specify different viewports and or scissor rectangles within a single command buffer
    // TODO(@k): Maybe we could use that to draw some minimap
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

    VkVertexInputBindingDescription vtx_binding_desc[2] = {};
#define MAX_VERTEX_ATTRIBUTE_DESCRIPTION_COUNT 15
    VkVertexInputAttributeDescription vtx_attr_descs[MAX_VERTEX_ATTRIBUTE_DESCRIPTION_COUNT];

    U64 vtx_binding_desc_count = 0;
    U64 vtx_attr_desc_cnt      = 0;
    switch(kind)
    {
        case (R_Vulkan_PipelineKind_MeshDebug):
        {
            vtx_binding_desc_count = 0;
            vtx_attr_desc_cnt = 0;
        }break;
        case (R_Vulkan_PipelineKind_Mesh):
        {
            vtx_binding_desc_count = 2;
            vtx_attr_desc_cnt = 15;

            // All of our per-vertex data is packed together in one array, so we'are only going to have one binding for now
            // This specifies the index of the binding in the array of bindings
            vtx_binding_desc[0].binding   = 0;
            // This specifies the number of bytes from one entry to the next
            vtx_binding_desc[0].stride    = sizeof(R_Vertex);
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
            vtx_attr_descs[0].offset = offsetof(R_Vertex, pos);

            vtx_attr_descs[1].binding  = 0;
            vtx_attr_descs[1].location = 1;
            vtx_attr_descs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
            vtx_attr_descs[1].offset   = offsetof(R_Vertex, nor);

            vtx_attr_descs[2].binding  = 0;
            vtx_attr_descs[2].location = 2;
            vtx_attr_descs[2].format   = VK_FORMAT_R32G32_SFLOAT;
            vtx_attr_descs[2].offset   = offsetof(R_Vertex, tex);

            vtx_attr_descs[3].binding  = 0;
            vtx_attr_descs[3].location = 3;
            vtx_attr_descs[3].format   = VK_FORMAT_R32G32B32_SFLOAT;
            vtx_attr_descs[3].offset   = offsetof(R_Vertex, tan);

            vtx_attr_descs[4].binding  = 0;
            vtx_attr_descs[4].location = 4;
            vtx_attr_descs[4].format   = VK_FORMAT_R32G32B32_SFLOAT;
            vtx_attr_descs[4].offset   = offsetof(R_Vertex, col);

            vtx_attr_descs[5].binding  = 0;
            vtx_attr_descs[5].location = 5;
            vtx_attr_descs[5].format   = VK_FORMAT_R32G32B32A32_UINT;
            vtx_attr_descs[5].offset   = offsetof(R_Vertex, joints);

            vtx_attr_descs[6].binding  = 0;
            vtx_attr_descs[6].location = 6;
            vtx_attr_descs[6].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
            vtx_attr_descs[6].offset   = offsetof(R_Vertex, weights);

            // Instance binding xform (vec4 x4)
            vtx_binding_desc[1].binding   = 1;
            vtx_binding_desc[1].stride    = sizeof(R_Mesh3DInst);
            vtx_binding_desc[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

            vtx_attr_descs[7].binding  = 1;
            vtx_attr_descs[7].location = 7;
            vtx_attr_descs[7].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
            vtx_attr_descs[7].offset   = offsetof(R_Mesh3DInst, white_texture_override);

            vtx_attr_descs[8].binding  = 1;
            vtx_attr_descs[8].location = 8;
            vtx_attr_descs[8].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
            vtx_attr_descs[8].offset   = sizeof(Vec4F32) * 0;

            vtx_attr_descs[9].binding  = 1;
            vtx_attr_descs[9].location = 9;
            vtx_attr_descs[9].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
            vtx_attr_descs[9].offset   = sizeof(Vec4F32) * 1;

            vtx_attr_descs[10].binding  = 1;
            vtx_attr_descs[10].location = 10;
            vtx_attr_descs[10].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
            vtx_attr_descs[10].offset   = sizeof(Vec4F32) * 2;

            vtx_attr_descs[11].binding  = 1;
            vtx_attr_descs[11].location = 11;
            vtx_attr_descs[11].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
            vtx_attr_descs[11].offset   = sizeof(Vec4F32) * 3;

            vtx_attr_descs[12].binding  = 1;
            vtx_attr_descs[12].location = 12;
            vtx_attr_descs[12].format   = VK_FORMAT_R32G32_UINT;
            vtx_attr_descs[12].offset   = offsetof(R_Mesh3DInst, key);

            vtx_attr_descs[13].binding  = 1;
            vtx_attr_descs[13].location = 13;
            vtx_attr_descs[13].format   = VK_FORMAT_R32_UINT;
            vtx_attr_descs[13].offset   = offsetof(R_Mesh3DInst, first_joint);

            vtx_attr_descs[14].binding  = 1;
            vtx_attr_descs[14].location = 14;
            vtx_attr_descs[14].format   = VK_FORMAT_R32_UINT;
            vtx_attr_descs[14].offset   = offsetof(R_Mesh3DInst, joint_count);
        }break;
        case (R_Vulkan_PipelineKind_Rect):
        {
            vtx_binding_desc_count = 1;
            vtx_attr_desc_cnt = 8;

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

            // crad
            vtx_attr_descs[6].binding  = 0;
            vtx_attr_descs[6].location = 6;
            vtx_attr_descs[6].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
            vtx_attr_descs[6].offset   = offsetof(R_Rect2DInst, corner_radii);

            // sty
            vtx_attr_descs[7].binding  = 0;
            vtx_attr_descs[7].location = 7;
            vtx_attr_descs[7].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
            vtx_attr_descs[7].offset   = offsetof(R_Rect2DInst, border_thickness);
        }break;
        case (R_Vulkan_PipelineKind_Geo3DComposite):
        case (R_Vulkan_PipelineKind_Finalize):
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
        case R_GeoPolygonKind_Fill:           {vk_polygon = VK_POLYGON_MODE_FILL;}break;
        case R_GeoPolygonKind_Line:           {vk_polygon = VK_POLYGON_MODE_LINE;}break;
        case R_GeoPolygonKind_Point:          {vk_polygon = VK_POLYGON_MODE_POINT;}break;
        default:                              {InvalidPath;}break;
    }

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
        case (R_Vulkan_PipelineKind_Mesh):
        {
            // rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_LINE;
            rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }break;
        case (R_Vulkan_PipelineKind_MeshDebug):
        case (R_Vulkan_PipelineKind_Rect):
        case (R_Vulkan_PipelineKind_Geo3DComposite):
        case (R_Vulkan_PipelineKind_Finalize):
        default: {}break;
    }

    // Multisampling
    // The VkPipelineMultisamplesStateCreateInfo struct configures multisampling, which is one of the ways to perform anti-aliasing
    // It works by combining the fragment shader resutls of multiple polygons that rasterize to the same pixel
    // This mainly occurs along edges, which is also where the most noticable aliasing artifacts occur
    // Because it doesn't need to run the fragment shader multiple times if only one polygon maps to a pixel, it is significantly less expensive than simply rendering
    // to a higher resolution and then downscaling (aka SSA). Enabling it requires enabling a GPU feature
    // TODO(@k): this doesn't make too much sense to me
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
        case (R_Vulkan_PipelineKind_MeshDebug): 
        case (R_Vulkan_PipelineKind_Mesh): 
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
            depth_stencil_state_create_info.front                 = (VkStencilOpState){}; // Optional
            depth_stencil_state_create_info.back                  = (VkStencilOpState){}; // Optional
        }break;
        case (R_Vulkan_PipelineKind_Rect): 
        case (R_Vulkan_PipelineKind_Geo3DComposite): 
        case (R_Vulkan_PipelineKind_Finalize): 
        {
            depth_stencil_state_create_info.depthTestEnable       = VK_FALSE;
            depth_stencil_state_create_info.depthWriteEnable      = VK_FALSE;
            depth_stencil_state_create_info.depthCompareOp        = VK_COMPARE_OP_LESS;
            depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
            depth_stencil_state_create_info.minDepthBounds        = 0.0f; // Optional
            depth_stencil_state_create_info.maxDepthBounds        = 1.0f; // Optional
            depth_stencil_state_create_info.stencilTestEnable     = VK_FALSE;
            depth_stencil_state_create_info.front                 = (VkStencilOpState){}; // Optional
            depth_stencil_state_create_info.back                  = (VkStencilOpState){}; // Optional
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
    /////////////////////////////////////////////////////////////////////////////////
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
    VkPipelineColorBlendAttachmentState color_blend_attachment_state[2] = {
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
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, // Optional
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // Optional
            .colorBlendOp        = VK_BLEND_OP_ADD, // Optional
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, // Optional
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // Optional
            .alphaBlendOp        = VK_BLEND_OP_ADD, // Optional
        }
    };
    switch(kind)
    {
        default:{}break;
        case R_Vulkan_PipelineKind_Rect: {}break;
        case R_Vulkan_PipelineKind_MeshDebug: {}break;
        case R_Vulkan_PipelineKind_Mesh: {}break;
        case R_Vulkan_PipelineKind_Geo3DComposite:
        {
            color_blend_attachment_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            color_blend_attachment_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
            color_blend_attachment_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        }break;
        case R_Vulkan_PipelineKind_Finalize: 
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
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
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
        default:{}break;
        case R_Vulkan_PipelineKind_MeshDebug:
        case R_Vulkan_PipelineKind_Mesh:
        {
            color_blend_state_create_info.attachmentCount = 2;
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

        switch(kind)
        {
            case (R_Vulkan_PipelineKind_Rect): 
            {
                set_layout_count = 2;
                set_layouts[0]   = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Rect].h;
                set_layouts[1]   = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;
            }break;
            case (R_Vulkan_PipelineKind_MeshDebug):
            {
                set_layout_count = 1;
                set_layouts[0]   = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Mesh].h;
            }break;
            case (R_Vulkan_PipelineKind_Mesh):
            {
                set_layout_count = 3;
                set_layouts[0]   = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_UBO_Mesh].h;
                set_layouts[1]   = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Storage_Mesh].h;
                set_layouts[2]   = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;
            }break;
            case (R_Vulkan_PipelineKind_Geo3DComposite):
            {
                set_layout_count = 1;
                set_layouts[0]   = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;
            }break;
            case (R_Vulkan_PipelineKind_Finalize): 
            {
                set_layout_count = 1;
                set_layouts[0]   = r_vulkan_state->set_layouts[R_Vulkan_DescriptorSetKind_Tex2D].h;
            }break;
            default: {InvalidPath;}break;
        }

        create_info.setLayoutCount         = set_layout_count; // Optional
        create_info.pSetLayouts            = set_layouts;      // Optional
                                                               // The structure also specifies push constants, which are another way of passing dynamic values to shaders that we may get into in future
        create_info.pushConstantRangeCount = 0;                // Optional
        create_info.pPushConstantRanges    = NULL;             // Optional

        // The pipeline layout will be referenced throughout the program's lifetime, so we should destroy it at the end
        VK_Assert(vkCreatePipelineLayout(r_vulkan_state->device.h, &create_info, NULL, &pipeline.layout), "Failed to create pipeline layout");
    }

    // Pipeline
    {
        VkGraphicsPipelineCreateInfo create_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        create_info.stageCount          = 2; // vertex and fragment shader
        create_info.pStages             = shad_stages;
        // fixed-function stages
        create_info.pVertexInputState   = &vtx_input_state_create_info;
        create_info.pInputAssemblyState = &input_assembly_state_create_info;
        create_info.pViewportState      = &viewport_state_create_info;
        create_info.pRasterizationState = &rasterization_state_create_info;
        create_info.pMultisampleState   = &multi_sampling_state_create_info;
        create_info.pDepthStencilState  = &depth_stencil_state_create_info;
        create_info.pColorBlendState    = &color_blend_state_create_info;
        create_info.pDynamicState       = &dynamic_state_create_info;
        // layout
        create_info.layout              = pipeline.layout;
        // And finally we have the reference to the render pass and the index of the sub pass where the graphics pipeline will be used
        // It is also possible to use other render passes with this pipeline instead of this specific instance, but they have to be compatible with renderPass 
        create_info.renderPass          = renderpass;
        // subpass index within the render_pass
        switch(kind)
        {
            case (R_Vulkan_PipelineKind_Rect):           {create_info.subpass = 0;}break;
            case (R_Vulkan_PipelineKind_MeshDebug):      {create_info.subpass = 0;}break;
            case (R_Vulkan_PipelineKind_Mesh):           {create_info.subpass = 0;}break;
            case (R_Vulkan_PipelineKind_Geo3DComposite): {create_info.subpass = 0;}break;
            case (R_Vulkan_PipelineKind_Finalize):       {create_info.subpass = 0;}break;
            default:                                     {InvalidPath;}break;
        }
        // Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline
        // The idea of pipeline derivatives is that it is less expensive to set up pipelines when they have much functionality in common with an existing pipeline and 
        // switching between pipelines from the same parent can also be done quicker
        // You can either specify the handle of an existing pipeline with basePipelineHandle or reference another pipeline that is about to be created by index with basePipelineIndex
        // These values are only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is also specified in the flags field of VkGraphicsPipelineCreateInfo
        create_info.basePipelineHandle  = VK_NULL_HANDLE; // Optional
        create_info.basePipelineIndex   = -1; // Optional
        // TODO(@k): not sure if we should do it
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
        VK_Assert(vkCreateGraphicsPipelines(r_vulkan_state->device.h, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline.h), "Failed to create pipeline");
    }
    return pipeline;
}

// Frame markers
/////////////////////////////////////////////////////////////////////////////////////////
internal void
r_begin_frame(void) {}

internal void
r_end_frame(void)
{
    // TODO(k): some cleanup jobs here?
}

internal U64
r_window_begin_frame(OS_Handle os_wnd, R_Handle window_equip)
{
    ProfBeginFunction();
    R_Vulkan_Window *wnd = r_vulkan_window_from_handle(window_equip);
    R_Vulkan_Frame *frame = &wnd->frames[wnd->curr_frame_idx];
    R_Vulkan_Device *device = &r_vulkan_state->device;

    // Wait until the previous frame has finished
    // This function takes an array of fences and waits on the host for either any or all of the fences to be signaled before returning
    // The VK_TRUE we pass here indicates that we want to wait for all fences
    // This function also has a timeout parameter that we set to the maxium value of 64 bit unsigned integer, which effectively disables the timeout
    vkWaitForFences(r_vulkan_state->device.h, 1, &frame->inflt_fence, VK_TRUE, UINT64_MAX);

    // Get point entity id
    U64 w = wnd->bag->geo3d_color_image.extent.width;
    void *ids = wnd->bag->geo3d_id_cpu.mapped;
    // U64 id = ((U64 *)ids)[(U64)(ptr.y*w+ptr.x)];
    U64 id = ((U64 *)ids)[0];

    // Vulkan will usually just tell us that the swapchain is no longer adequate during presentation
    // The vkAcquireNextImageKHR and vkQueuePresentKHR functions can return the following special values to indicate this
    // 1. VK_ERROR_OUT_OF_DATE_KHR: the swap chain has become incompatible with the surface and can no longer be used for rendering
    //    Usually happens after a window resize
    // 2. VK_SUBOPTIMAL_KHR: the swap chain can still be used to successfully present to the surface, but the surface properties are no longer matched exactly
    VkResult ret;
    while(1)
    {
        // Acquire image from swapchain
        ret = vkAcquireNextImageKHR(device->h, wnd->bag->swapchain.h, UINT64_MAX, frame->img_acq_sem, VK_NULL_HANDLE, &frame->img_idx);
        if(ret == VK_ERROR_OUT_OF_DATE_KHR || ret == VK_SUBOPTIMAL_KHR)
        {
            r_vulkan_window_resize(wnd);
            continue;
        }
        AssertAlways(ret == VK_SUCCESS);
        break;
    }

    // Update generation
    frame->bag_gen = wnd->bag->generation;
    frame->rendpass_grp_gen = wnd->rendpass_grp->generation;

    // Cleanup swapchain and render pass group
    /////////////////////////////////////////////////////////////////////////////////
    bool bag_gen_synced = true;
    bool rendpass_grp_gen_synced = true;
    for(U64 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        R_Vulkan_Frame *frame = &wnd->frames[i];
        if(frame->bag_gen != wnd->bag->generation) bag_gen_synced = false;
        if(frame->rendpass_grp_gen != wnd->rendpass_grp->generation) rendpass_grp_gen_synced = false;
    }
    if(bag_gen_synced)
    {
        for(R_Vulkan_Bag *b = wnd->first_to_free_bag; b != 0; b = b->next)
        {
            r_vulkan_bag_destroy(b);
        }
        wnd->first_to_free_bag = 0;
    }
    if(rendpass_grp_gen_synced)
    {
        for(R_Vulkan_RenderPassGroup *g = wnd->first_to_free_rendpass_grp; g != 0; g = g->next)
        {
            r_vulkan_rendpass_grp_destroy(g);
        }
        wnd->first_to_free_rendpass_grp = 0;
    }

    // Reset frame fence
    VK_Assert(vkResetFences(device->h, 1, &frame->inflt_fence), "Failed to reset fence");
    // Reset command buffers
    VK_Assert(vkResetCommandBuffer(frame->cmd_buf, 0), "Failed to reset command buffer");

    // Start command recrod
    r_vulkan_cmd_begin(frame->cmd_buf);
    ProfEnd();
    return id;
}

internal void
r_window_end_frame(OS_Handle os_wnd, R_Handle window_equip)
{
    // TODO(@k): Should every window have its own thread?
    //           since different window need to wait on its fence, we don't want to block it for other windows
    R_Vulkan_Window *wnd              = r_vulkan_window_from_handle(window_equip);
    R_Vulkan_Frame *frame             = &wnd->frames[wnd->curr_frame_idx];
    VkCommandBuffer cmd_buf           = frame->cmd_buf;
    R_Vulkan_Swapchain *swapchain     = &wnd->bag->swapchain;
    R_Vulkan_RenderPass *renderpasses = wnd->rendpass_grp->passes;
    VkFramebuffer *framebuffers       = wnd->bag->framebuffers;

    // Copy stage buffer to swapchain image
    /////////////////////////////////////////////////////////////////////////////////
    R_Vulkan_RenderPass *renderpass = &renderpasses[R_Vulkan_RenderPassKind_Finalize];
    VkRenderPassBeginInfo begin_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    begin_info.renderPass        = renderpass->h;
    begin_info.framebuffer       = framebuffers[R_Vulkan_RenderPassKind_Finalize+frame->img_idx];
    begin_info.renderArea.offset = (VkOffset2D){0, 0};
    begin_info.renderArea.extent = wnd->bag->stage_color_image.extent;

    // Clear swapchain image
    VkClearValue clear_colors[1];
    clear_colors[0].color = (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 0.0f }}; /* black with 100% opacity */
    begin_info.clearValueCount = 1;
    begin_info.pClearValues    = clear_colors;
    vkCmdBeginRenderPass(cmd_buf, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // Bind pipeline
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderpass->pipeline.finalize.h);
    // Viewport and scissor
    VkViewport viewport = {0};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = swapchain->extent.width;
    viewport.height   = swapchain->extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
    // Setup scissor rect
    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent = swapchain->extent;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
    // Bind stage color descriptor
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderpass->pipeline.finalize.layout, 0, 1, &wnd->bag->stage_color_ds.h, 0, NULL);
    // Draw the quad
    vkCmdDraw(cmd_buf, 4, 1, 0, 0);
    vkCmdEndRenderPass(cmd_buf);

    // End and submit command buffer
    /////////////////////////////////////////////////////////////////////////////////
    VK_Assert(vkEndCommandBuffer(cmd_buf), "Failed to end command buffer");
    VkSemaphore wait_sems[1]            = { frame->img_acq_sem };
    VkPipelineStageFlags wait_stages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signal_sems[1]          = { frame->rend_comp_sem };

    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = wait_sems;
    submit_info.pWaitDstStageMask    = wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &cmd_buf;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = signal_sems;

    VK_Assert(vkQueueSubmit(r_vulkan_state->device.gfx_queue, 1, &submit_info, frame->inflt_fence), "Failed to submit command buffer");

    // Present
    /////////////////////////////////////////////////////////////////////////////////
    // TODO(@k): should we move this to r_end_frame? we could submit to all swapchains at once
    VkSwapchainKHR target_swapchains[1] = { swapchain->h };
    VkPresentInfoKHR prest_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    prest_info.waitSemaphoreCount = 1;
    prest_info.pWaitSemaphores    = signal_sems;
    prest_info.swapchainCount     = 1;
    prest_info.pSwapchains        = target_swapchains;
    prest_info.pImageIndices      = &frame->img_idx;
    // There is one last optional parameter called pResults
    // It allows you to specify an array of VkResult values to check for every individual swapchain if presentation was successful
    // It's not necessary if you're only using a single swapchain, because you can simply use the return value of the present function
    prest_info.pResults           = NULL;

    VkResult prest_ret = vkQueuePresentKHR(r_vulkan_state->device.prest_queue, &prest_info);

    // NOTE(@k): It is important to only check window_resized here to ensure that the job-done semaphores are in a consistent state
    //           otherwise a signaled semaphore may never be properly waited upon
    if(prest_ret == VK_ERROR_OUT_OF_DATE_KHR || prest_ret == VK_SUBOPTIMAL_KHR)
    {
        r_vulkan_window_resize(wnd);
    } 
    else { AssertAlways(prest_ret == VK_SUCCESS); }

    // Update curr_frame_idx
    wnd->curr_frame_idx = (wnd->curr_frame_idx + 1) % MAX_FRAMES_IN_FLIGHT;
}

internal void
r_window_submit(OS_Handle os_wnd, R_Handle window_equip, R_PassList *passes, Vec2F32 ptr)
{
    R_Vulkan_Window *wnd              = r_vulkan_window_from_handle(window_equip);
    R_Vulkan_RenderPass *renderpasses = wnd->rendpass_grp->passes;
    VkFramebuffer *framebuffers       = wnd->bag->framebuffers;

    R_Vulkan_Frame *frame = &wnd->frames[wnd->curr_frame_idx];
    VkCommandBuffer cmd_buf = frame->cmd_buf;

    // TODO(@k): Build command buffers in parallel and evenly across several threads/cores
    //           to multiple command lists
    //           Recording commands is a CPU intensive operation and no driver threads come to reuse

    U64 ui_inst_buffer_offset = 0;
    U64 ui_group_count        = 0;

    U64 mesh_inst_buffer_offset = 0;

    // Do passing
    for(R_PassNode *pass_n = passes->first; pass_n != 0; pass_n = pass_n->next)
    {
        R_Pass *pass = &pass_n->v;
        switch(pass->kind)
        {
            case R_PassKind_UI:
            {
                // Unpack renderpass and pipeline
                R_Vulkan_RenderPass *renderpass = &renderpasses[R_Vulkan_RenderPassKind_Rect];

                // Start renderpass
                VkRenderPassBeginInfo begin_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
                begin_info.renderPass        = renderpass->h;
                begin_info.framebuffer       = framebuffers[R_Vulkan_RenderPassKind_Rect];
                // Those two parameters define the size of the render area
                // TODO(@k): why it effect the shader loads
                // The render area defiens where shader loads and stores will take place
                // The pixels outside this region will have undefined values
                // It should match the size of attachments for best performance
                begin_info.renderArea.offset = (VkOffset2D){0, 0};
                begin_info.renderArea.extent = wnd->bag->stage_color_image.extent;

                // Note that the order of clear_values should be identical to the order of your attachments
                VkClearValue clear_colors[1];
                clear_colors[0].color = (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 0.0f }}; /* black with 100% opacity */
                // Those two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, which we used as load operations for the color attachment
                begin_info.clearValueCount = 0;
                begin_info.pClearValues    = clear_colors;
                // All of the functions that record commands can be recongnized by their vkCmd prefix
                // They all return void, so there will be no error handling until we've finished recording
                // 1. VK_SUBPASS_CONTENTS_INLINE: This flag indicates that the draw commands for this render pass will be recorded directly in the primary command buffer.
                //    There will be no secondary command buffers executed within this render pass. All rendering commands are specified directly between vkCmdBeginRenderPass and vkCmdEndRenderPass
                // 2. VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: When this flag is used, it indicates that the rendering commands are recorded in secondary command buffers, which are then executed from the primary command buffer.
                //    This method is often used for more complex rendering strategies where commands are modularized into different secondary command buffers.
                vkCmdBeginRenderPass(cmd_buf, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

                // Unpack params 
                R_PassParams_UI *params = pass->params_ui;
                R_BatchGroup2DList *rect_batch_groups = &params->rects;

                // Unpack uniform buffer
                R_Vulkan_UniformBuffer *uniform_buffer = &frame->uniform_buffers[R_Vulkan_UniformTypeKind_Rect];
                // TODO(@k): dynamic allocate uniform buffer if needed
                AssertAlways(rect_batch_groups->count <= 900);

                // Bind pipeline
                // The second parameter specifies if the pipeline object is a graphics or compute pipelinVK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BITe
                // We've now told Vulkan which operations to execute in the graphcis pipeline and which attachment to use in the fragment shader
                vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderpass->pipeline.rect.h);

                // Draw each group
                // Rects in the same group share some features
                for(R_BatchGroup2DNode *group_n = rect_batch_groups->first; group_n != 0; group_n = group_n->next)
                {
                    R_BatchList *batches               = &group_n->batches;
                    R_BatchGroup2DParams *group_params = &group_n->params;

                    // Get & fill instance bufer
                    // TODO(@k): Dynamic allocaate instance buffer if needed
                    U8 *dst_ptr = frame->inst_buffer_rect.mapped + ui_inst_buffer_offset;
                    U64 off = 0;
                    for(R_BatchNode *batch = batches->first; batch != 0; batch = batch->next)
                    {
                        MemoryCopy(dst_ptr+off, batch->v.v, batch->v.byte_count);
                        off += batch->v.byte_count;
                    }
                    // Bind instance buffer
                    VkDeviceSize inst_buffer_offset = {ui_inst_buffer_offset};
                    vkCmdBindVertexBuffers(cmd_buf, 0, 1, &frame->inst_buffer_rect.h, &inst_buffer_offset);

                    // Get texture
                    R_Handle tex_handle = group_params->tex;
                    if(r_handle_match(tex_handle, r_handle_zero()))
                    {
                        tex_handle = r_vulkan_state->backup_texture;
                    }
                    R_Vulkan_Tex2D *texture = r_vulkan_tex2d_from_handle(tex_handle);
                    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderpass->pipeline.rect.layout, 1, 1, &texture->desc_set.h, 0, NULL);

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
                            // TODO(@k): why, shouldn't vulkan use col-major order?
                            texture_sample_channel_map[0] = v4f32(1, 1, 1, 1);
                            // texture_sample_channel_map[0].x = 1;
                            // texture_sample_channel_map[1].x = 1;
                            // texture_sample_channel_map[2].x = 1;
                            // texture_sample_channel_map[3].x = 1;
                        }break;
                    }

                    //~ Bind uniform buffer
                    /////////////////////////////////////////////////

                    // Upload uniforms
                    R_Vulkan_Uniforms_Rect uniforms = {0};
                    uniforms.viewport_size = v2f32(wnd->bag->stage_color_image.extent.width, wnd->bag->stage_color_image.extent.height);
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

                    U32 uniform_buffer_offset = ui_group_count * uniform_buffer->stride;
                    MemoryCopy((U8 *)uniform_buffer->buffer.mapped + uniform_buffer_offset, &uniforms, sizeof(R_Vulkan_Uniforms_Rect));
                    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderpass->pipeline.rect.layout, 0, 1, &uniform_buffer->set.h, 1, &uniform_buffer_offset);

                    VkViewport viewport = {0};
                    viewport.x        = 0.0f;
                    viewport.y        = 0.0f;
                    viewport.width    = wnd->bag->stage_color_image.extent.width;
                    viewport.height   = wnd->bag->stage_color_image.extent.height;
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;
                    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

                    VkRect2D scissor = {0};
                    if(group_params->clip.x0 == 0 && group_params->clip.x1 == 0 && group_params->clip.y0 == 0 && group_params->clip.y1 == 0)
                    {
                        scissor.offset = (VkOffset2D){0,0};
                        scissor.extent = wnd->bag->stage_color_image.extent;
                    }
                    else if(group_params->clip.x0 > group_params->clip.x1 || group_params->clip.y0 > group_params->clip.y1)
                    {
                        scissor.offset = (VkOffset2D){0,0};
                        scissor.extent = (VkExtent2D){0,0};
                    }
                    else
                    {
                        scissor.offset = (VkOffset2D){(U32)group_params->clip.p0.x, (U32)group_params->clip.p0.y};
                        Vec2F32 clip_dim = dim_2f32(group_params->clip);
                        scissor.extent = (VkExtent2D){(U32)clip_dim.x, (U32)clip_dim.y};
                        Assert(!(clip_dim.x == 0 && clip_dim.y == 0));
                    }
                    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

                    U64 inst_count = batches->byte_count / batches->bytes_per_inst;
                    vkCmdDraw(cmd_buf, 4, inst_count, 0, 0);

                    ui_group_count++;
                    ui_inst_buffer_offset += batches->byte_count;
                }
                vkCmdEndRenderPass(cmd_buf);
            }break;
            case R_PassKind_Geo3D: 
            {
                // Unpack params
                R_PassParams_Geo3D *params = pass->params_geo3d;
                R_BatchGroup3DMap *mesh_group_map = &params->mesh_batches;

                // Unpack renderpass and pipelines
                R_Vulkan_RenderPass *renderpass = &renderpasses[R_Vulkan_RenderPassKind_Mesh];

                R_Vulkan_Pipeline *mesh_debug_pipelines = renderpass->pipeline.mesh[0];
                R_Vulkan_Pipeline *mesh_pipelines       = renderpass->pipeline.mesh[1];

                // Start renderpass
                VkRenderPassBeginInfo begin_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
                begin_info.renderPass        = renderpass->h;
                begin_info.framebuffer       = framebuffers[R_Vulkan_RenderPassKind_Mesh];
                begin_info.renderArea.offset = (VkOffset2D){0, 0};
                begin_info.renderArea.extent = wnd->bag->stage_color_image.extent;
                
                // Note that the order of clear_values should be identical to the order of your attachments
                VkClearValue clear_colors[3];
                clear_colors[0].color        = (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 0.0f }}; /* black with 100% opacity */
                clear_colors[1].color        = (VkClearColorValue){{ 0.0f }}; /* black with 100% opacity */
                // The range of depths in the depth buffer is 0.0 to 1.0 in Vulkan, where 1.0 lies at the far view plane and 0.0 at the near view plane. The initial value at each point in the depth buffer should be the furthest possible depth, which is 1.0
                clear_colors[2].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
                // Those two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, which we used as load operations for the color attachment
                begin_info.clearValueCount = 3;
                begin_info.pClearValues    = clear_colors;
                vkCmdBeginRenderPass(cmd_buf, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

                // Unpack uniform buffer
                R_Vulkan_UniformBuffer *uniform_buffer = &frame->uniform_buffers[R_Vulkan_UniformTypeKind_Mesh];
                // TODO(@k): dynamic allocate uniform buffer if needed

                // Setup uniforms buffer
                R_Vulkan_Uniforms_Mesh uniforms = {0};
                uniforms.proj          = params->projection;
                uniforms.view          = params->view;
                uniforms.global_light  = v4f32(params->global_light.x, params->global_light.y, params->global_light.z, 0.0);
                uniforms.show_grid     = params->show_grid;
                uniforms.show_gizmos   = params->show_gizmos;
                uniforms.gizmos_xform  = params->gizmos_xform;
                uniforms.gizmos_origin = params->gizmos_origin;

                MemoryCopy((U8 *)uniform_buffer->buffer.mapped, &uniforms, sizeof(R_Vulkan_Uniforms_Mesh));
                vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pipelines[0].layout, 0, 1, &uniform_buffer->set.h, 0, NULL);

                // Setup viewport and scissor 
                VkViewport viewport = {0};
                Vec2F32 viewport_dim = dim_2f32(params->viewport);
                if(viewport_dim.x == 0 || viewport_dim.y == 0)
                {
                    viewport.width    = wnd->bag->stage_color_image.extent.width;
                    viewport.height   = wnd->bag->stage_color_image.extent.height;
                }
                else
                {

                    viewport.x        = params->viewport.p0.x;
                    viewport.y        = params->viewport.p0.y;
                    viewport.width    = params->viewport.p1.x;
                    viewport.height   = params->viewport.p1.y;
                }
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

                VkRect2D scissor = {0};
                scissor.offset = (VkOffset2D){0, 0};
                scissor.extent = wnd->bag->stage_color_image.extent;
                // scissor.offset = (VkOffset2D){viewport.x, viewport.y};
                // scissor.extent = (VkExtent2D){viewport.width, viewport.height};
                vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

                // Bind mesh debug pipeline
                vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_debug_pipelines[R_GeoPolygonKind_COUNT * R_GeoTopologyKind_Triangles + R_GeoPolygonKind_Fill].h);
                // Draw mesh debug info (grid, gizmos e.g.)
                vkCmdDraw(cmd_buf, 6, 1, 0, 0);

                // Unpack storage buffer (Currently only for joints)
                R_Vulkan_StorageBuffer *storage_buffer = &frame->storage_buffers[R_Vulkan_StorageTypeKind_Mesh];
                U8 *storage_buffer_dst = (U8 *)(storage_buffer->buffer.mapped);
                U32 storage_buffer_offset = 0;
                vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pipelines[0].layout, 1, 1, &storage_buffer->set.h, 0, 0);

                for(U64 slot_idx = 0; slot_idx < mesh_group_map->slots_count; slot_idx++)
                {
                    for(R_BatchGroup3DMapNode *n = mesh_group_map->slots[slot_idx]; n!=0; n = n->next) 
                    {
                        // Unpack group params
                        R_BatchList *batches = &n->batches;
                        R_BatchGroup3DParams *group_params = &n->params;
                        R_Vulkan_Buffer *mesh_vertices = r_vulkan_buffer_from_handle(group_params->mesh_vertices);
                        R_Vulkan_Buffer *mesh_indices = r_vulkan_buffer_from_handle(group_params->mesh_indices);
                        U64 inst_count = batches->byte_count / batches->bytes_per_inst;

                        // Unpack specific pipeline for topology and polygon mode
                        R_GeoTopologyKind topology = group_params->mesh_geo_topology;
                        R_GeoPolygonKind polygon = group_params->mesh_geo_polygon;
                        R_Vulkan_Pipeline *pipeline = &mesh_pipelines[R_GeoPolygonKind_COUNT*topology + polygon];

                        // Get & fill instance buffer
                        // TODO(k): Dynamic allocate instance buffer if needed
                        U8 *dst_ptr = frame->inst_buffer_mesh.mapped + mesh_inst_buffer_offset;
                        U64 off = 0;
                        for(R_BatchNode *batch = batches->first; batch != 0; batch = batch->next)
                        {
                            // Copy instance skinning data to storage buffer
                            U64 batch_inst_count = batch->v.byte_count / sizeof(R_Mesh3DInst);
                            R_Mesh3DInst *inst = 0; 
                            for(U64 inst_idx = 0; inst_idx < batch_inst_count; inst_idx++)
                            {
                                inst = ((R_Mesh3DInst *)batch->v.v)+inst_idx;
                                if(inst->joint_count > 0)
                                {
                                    U32 size = sizeof(Mat4x4F32) * inst->joint_count;
                                    // TODO: we may need to consider the stride here
                                    inst->first_joint = storage_buffer_offset / sizeof(Mat4x4F32);
                                    MemoryCopy(storage_buffer_dst+storage_buffer_offset, inst->joint_xforms, size);
                                    storage_buffer_offset += size;
                                }
                            }

                            // Copy to instance buffer
                            MemoryCopy(dst_ptr+off, batch->v.v, batch->v.byte_count);
                            off += batch->v.byte_count;
                        }

                        // Bind texture
                        R_Handle tex_handle = group_params->albedo_tex;
                        if(r_handle_match(tex_handle, r_handle_zero()))
                        {
                            tex_handle = r_vulkan_state->backup_texture;
                        }
                        R_Vulkan_Tex2D *texture = r_vulkan_tex2d_from_handle(tex_handle);
                        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 2, 1, &texture->desc_set.h, 0, NULL);

                        vkCmdBindVertexBuffers(cmd_buf, 0, 1, &mesh_vertices->h, &(VkDeviceSize){0});
                        vkCmdBindIndexBuffer(cmd_buf, mesh_indices->h, (VkDeviceSize){0}, VK_INDEX_TYPE_UINT32);
                        VkDeviceSize inst_buffer_offset = { mesh_inst_buffer_offset };
                        vkCmdBindVertexBuffers(cmd_buf, 1, 1, &frame->inst_buffer_mesh.h, &inst_buffer_offset);

                        // Bind mesh pipeline
                        // The second parameter specifies if the pipeline object is a graphics or compute pipelinVK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BITe
                        // We've now told Vulkan which operations to execute in the graphcis pipeline and which attachment to use in the fragment shader
                        vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->h);

                        // Draw mesh
                        vkCmdDrawIndexed(cmd_buf, mesh_indices->size/sizeof(U32), inst_count, 0, 0, 0);

                        // increament group counter
                        mesh_inst_buffer_offset += batches->byte_count;
                    }
                }
                vkCmdEndRenderPass(cmd_buf);

                // Copy geo3d_image to geo3d_cpu(buffer)
                // TODO(k): this is a slow operation, fps is reduced by 100
                //          maybe we could use a dedicated transfer queue to do it instead of block graphic queue
                {
                    VkImageMemoryBarrier barrier = {};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    // NOTE(k): weird here, it's alreay in src optimal, but we need to wait for the writing
                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // Not changing queue families
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = wnd->bag->geo3d_id_image.h;
                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Prior access
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT; // Next access

                    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

                    vkCmdPipelineBarrier(
                        cmd_buf,
                        srcStage, dstStage, // Source and destination pipeline stages
                        0,                  // No dependency flags
                        0, NULL,            // Memory barriers
                        0, NULL,            // Buffer memory barriers
                        1, &barrier         // Image memory barriers
                    );

                    VkBufferImageCopy region = {0};
                    region.bufferOffset = 0;
                    region.bufferRowLength = 0; // Tightly packed
                    region.bufferImageHeight = 0;
                    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    region.imageSubresource.mipLevel = 0;
                    region.imageSubresource.baseArrayLayer = 0;
                    region.imageSubresource.layerCount = 1;
                    Vec2F32 range = {ptr.x, ptr.y};
                    range.x = Clamp(0,ptr.x,wnd->bag->geo3d_id_image.extent.width-1);
                    range.y = Clamp(0,ptr.y,wnd->bag->geo3d_id_image.extent.height-1);
                    region.imageOffset = (VkOffset3D){range.x,range.y,0};
                    // region.imageExtent = (VkExtent3D){wnd->bag->geo3d_id_image.extent.width, wnd->bag->geo3d_id_image.extent.height, 1};
                    region.imageExtent = (VkExtent3D){1, 1, 1};
                    vkCmdCopyImageToBuffer(cmd_buf, wnd->bag->geo3d_id_image.h, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, wnd->bag->geo3d_id_cpu.h, 1, &region);
                }

                // Composite to the main staging buffer
                {
                    // Unpack renderpass and pipeline
                    R_Vulkan_RenderPass *renderpass = &renderpasses[R_Vulkan_RenderPassKind_Geo3DComposite];
                    // Start renderpass
                    VkRenderPassBeginInfo begin_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
                    begin_info.renderPass        = renderpass->h;
                    begin_info.framebuffer       = framebuffers[R_Vulkan_RenderPassKind_Geo3DComposite];
                    begin_info.renderArea.offset = (VkOffset2D){0, 0};
                    begin_info.renderArea.extent = wnd->bag->stage_color_image.extent;
                    VkClearValue clear_colors[1];
                    clear_colors[0].color = (VkClearColorValue){{ 0.0f, 0.0f, 0.0f, 0.0f }}; /* black with 100% opacity */
                    // Those two parameters define the clear values to use for VK_ATTACHMENT_LOAD_OP_CLEAR, which we used as load operations for the color attachment
                    begin_info.clearValueCount = 1;
                    begin_info.pClearValues    = clear_colors;
                    vkCmdBeginRenderPass(cmd_buf, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderpass->pipeline.geo3d_composite.h);
                    // Viewport and scissor
                    VkViewport viewport = {0};
                    viewport.x        = 0.0f;
                    viewport.y        = 0.0f;
                    viewport.width    = wnd->bag->stage_color_image.extent.width;
                    viewport.height   = wnd->bag->stage_color_image.extent.height;
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;
                    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
                    // Setup scissor rect
                    VkRect2D scissor = {0};
                    scissor.offset = (VkOffset2D){0, 0};
                    scissor.extent = wnd->bag->stage_color_image.extent;
                    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
                    // Bind stage color descriptor
                    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, renderpass->pipeline.geo3d_composite.layout, 0, 1, &wnd->bag->geo3d_color_ds.h, 0, NULL);
                    vkCmdDraw(cmd_buf, 4, 1, 0, 0);
                    vkCmdEndRenderPass(cmd_buf);
                }
            }break;
            default: {InvalidPath;}break;
        }
    }
}

internal R_Vulkan_UniformBuffer
r_vulkan_uniform_buffer_alloc(R_Vulkan_UniformTypeKind kind, U64 unit_count)
{
    R_Vulkan_UniformBuffer uniform_buffer = {0};

    U64 stride = 0;

    switch(kind)
    {
        case R_Vulkan_UniformTypeKind_Rect: {stride = AlignPow2(sizeof(R_Vulkan_Uniforms_Rect), r_vulkan_state->gpu.properties.limits.minUniformBufferOffsetAlignment);}break;
        case R_Vulkan_UniformTypeKind_Mesh: {stride = AlignPow2(sizeof(R_Vulkan_Uniforms_Mesh), r_vulkan_state->gpu.properties.limits.minUniformBufferOffsetAlignment);}break;
        default:                            {InvalidPath;}break;
    }
    U64 buf_size = stride * unit_count;

    uniform_buffer.unit_count  = unit_count;
    uniform_buffer.stride      = stride;
    uniform_buffer.buffer.size = buf_size;

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

    VK_Assert(vkCreateBuffer(r_vulkan_state->device.h, &buf_ci, NULL, &uniform_buffer.buffer.h), "Failed to create vk buffer");
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(r_vulkan_state->device.h, uniform_buffer.buffer.h, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    alloc_info.allocationSize  = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

    VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &uniform_buffer.buffer.memory), "Failed to allocate buffer memory");
    VK_Assert(vkBindBufferMemory(r_vulkan_state->device.h, uniform_buffer.buffer.h, uniform_buffer.buffer.memory, 0), "Failed to bind buffer memory");
    Assert(uniform_buffer.buffer.size != 0);
    VK_Assert(vkMapMemory(r_vulkan_state->device.h, uniform_buffer.buffer.memory, 0, uniform_buffer.buffer.size, 0, &uniform_buffer.buffer.mapped), "Failed to map buffer memory");

    // Create descriptor set
    R_Vulkan_DescriptorSetKind ds_type = 0;
    switch(kind)
    {
        case R_Vulkan_UniformTypeKind_Rect: {ds_type = R_Vulkan_DescriptorSetKind_UBO_Rect;}break;
        case R_Vulkan_UniformTypeKind_Mesh: {ds_type = R_Vulkan_DescriptorSetKind_UBO_Mesh;}break;
        default:                            {InvalidPath;}break;
    }

    r_vulkan_descriptor_set_alloc(ds_type, 1, 3, &uniform_buffer.buffer.h, NULL, NULL, &uniform_buffer.set);
    return uniform_buffer;
}

internal R_Vulkan_StorageBuffer
r_vulkan_storage_buffer_alloc(R_Vulkan_StorageTypeKind kind, U64 unit_count)
{
    R_Vulkan_StorageBuffer storage_buffer = {0};
    U64 stride = 0;

    switch(kind)
    {
        case R_Vulkan_StorageTypeKind_Mesh: {stride = AlignPow2(sizeof(R_Vulkan_Storage_Mesh), r_vulkan_state->gpu.properties.limits.minStorageBufferOffsetAlignment);}break;
        default:                            {InvalidPath;}break;
    }

    U64 buf_size = stride * unit_count;

    storage_buffer.unit_count  = unit_count;
    storage_buffer.stride      = stride;
    storage_buffer.buffer.size = buf_size;

    VkBufferCreateInfo buf_ci = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buf_ci.size = buf_size;
    buf_ci.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    // Just like the images in the swapchain, buffers can also be owned by a specific queue family or be shared between multiple at the same time
    // Our buffer will only be used from the graphics queue, so we an stick to exclusive access
    // .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // The flags parameter is used to configure sparse buffer memory
    // Sparse bfufers in VUlkan refer to a memory management technique that allows more flexible and efficient use of GPU memory
    // This technique is particularly useful for handling large datasets, such as textures or vertex buffers, that might not fit contiguously in GPU
    // memory or that require efficient streaming of data in and out of GPU memory
    buf_ci.flags = 0;

    VK_Assert(vkCreateBuffer(r_vulkan_state->device.h, &buf_ci, NULL, &storage_buffer.buffer.h), "Failed to create vk buffer");
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(r_vulkan_state->device.h, storage_buffer.buffer.h, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    alloc_info.allocationSize  = mem_requirements.size;
    alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

    VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &storage_buffer.buffer.memory), "Failed to allocate buffer memory");
    VK_Assert(vkBindBufferMemory(r_vulkan_state->device.h, storage_buffer.buffer.h, storage_buffer.buffer.memory, 0), "Failed to bind buffer memory");
    Assert(storage_buffer.buffer.size != 0);
    VK_Assert(vkMapMemory(r_vulkan_state->device.h, storage_buffer.buffer.memory, 0, storage_buffer.buffer.size, 0, &storage_buffer.buffer.mapped), "Failed to map buffer memory");

    // Create descriptor set
    R_Vulkan_DescriptorSetKind ds_type = 0;
    switch(kind)
    {
        case R_Vulkan_StorageTypeKind_Mesh: {ds_type = R_Vulkan_DescriptorSetKind_Storage_Mesh;}break;
        default:                            {InvalidPath;}break;
    }

    r_vulkan_descriptor_set_alloc(ds_type, 1, 3, &storage_buffer.buffer.h, NULL, NULL, &storage_buffer.set);
    return storage_buffer;
}

// TODO(@k): ugly, split into alloc and update
internal void
r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind kind,
                              U64 set_count, U64 cap, VkBuffer *buffers,
                              VkImageView *image_views, VkSampler *samplers,
                              R_Vulkan_DescriptorSet *sets)
{
    R_Vulkan_DescriptorSetLayout set_layout = r_vulkan_state->set_layouts[kind];
    Assert(set_count <= cap);

    U64 remaining = set_count;
    R_Vulkan_DescriptorSetPool *pool;
    while(remaining > 0)
    {
        pool = r_vulkan_state->first_avail_ds_pool[kind];
        if(pool == 0)
        {
            pool = push_array(r_vulkan_state->arena, R_Vulkan_DescriptorSetPool, 1);
            pool->kind = kind;
            pool->cmt  = 0;
            pool->cap  = AlignPow2(cap, 16);

            VkDescriptorPoolSize pool_sizes[set_layout.binding_count];
            for(U64 i = 0; i < set_layout.binding_count; i++)
            {
                pool_sizes[i].type            = set_layout.bindings[i].descriptorType;
                pool_sizes[i].descriptorCount = set_layout.bindings[i].descriptorCount * pool->cap;
            }

            VkDescriptorPoolCreateInfo pool_create_info = {
                .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .poolSizeCount = 1,
                .pPoolSizes    = pool_sizes,
                // Aside from the maxium number of individual descriptors that are available, we also need to specify the maxium number of descriptor sets that may be allcoated
                .maxSets       = pool->cap,
                // The structure has an optional flag similar to command pools that determines if individual descriptor sets can be freed or not
                // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
                // We're not going to touch the descriptor set after creating it, so we don't nedd this flag 
                // You can leave flags to 0
                .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            };
            VK_Assert(vkCreateDescriptorPool(r_vulkan_state->device.h, &pool_create_info, NULL, &pool->h), "Failed to create descriptor pool");

        } else { SLLStackPop(r_vulkan_state->first_avail_ds_pool[kind]); }

        U64 alloc_count = (pool->cap - pool->cmt);
        Assert(alloc_count > 0);
        if(remaining < alloc_count) alloc_count = remaining;

        VkDescriptorSet temp_sets[alloc_count];
        VkDescriptorSetAllocateInfo set_alloc_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        set_alloc_info.descriptorPool     = pool->h;
        set_alloc_info.descriptorSetCount = alloc_count;
        set_alloc_info.pSetLayouts        = &set_layout.h;
        VK_Assert(vkAllocateDescriptorSets(r_vulkan_state->device.h, &set_alloc_info, temp_sets), "Failed to allcoate descriptor sets");

        for(U64 i = 0; i < alloc_count; i++) 
        {
            U64 offset = set_count - remaining;
            sets[i+offset].h = temp_sets[i];
            sets[i+offset].pool = pool;
        }

        pool->cmt += alloc_count;
        remaining -= alloc_count;

        if(pool->cmt < pool->cap) { SLLStackPush(r_vulkan_state->first_avail_ds_pool[kind], pool); }
    }

    // NOTE(@k): set could have multiple writes due multiple bindings
    U64 writes_count = set_count * set_layout.binding_count;
    VkWriteDescriptorSet writes[writes_count];
    MemoryZeroArray(writes);

    Temp temp = scratch_begin(0,0);
    for(U64 i = 0; i < set_count; i++)
    {
        for(U64 j = 0; j < set_layout.binding_count; j++)
        {
            U64 set_idx = i + j;
            switch(kind)
            {
                case (R_Vulkan_DescriptorSetKind_UBO_Rect):
                {
                    VkDescriptorBufferInfo *buffer_info = push_array(temp.arena, VkDescriptorBufferInfo, 1);
                    buffer_info->buffer = buffers[i];
                    buffer_info->offset = 0;
                    buffer_info->range  = sizeof(R_Vulkan_Uniforms_Rect);

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
                case (R_Vulkan_DescriptorSetKind_UBO_Mesh):
                {
                    VkDescriptorBufferInfo *buffer_info = push_array(temp.arena, VkDescriptorBufferInfo, 1);
                    buffer_info->buffer = buffers[i];
                    buffer_info->offset = 0;
                    buffer_info->range  = sizeof(R_Vulkan_Uniforms_Mesh);

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
                case (R_Vulkan_DescriptorSetKind_Storage_Mesh):
                {
                    VkDescriptorBufferInfo *buffer_info = push_array(temp.arena, VkDescriptorBufferInfo, 1);
                    buffer_info->buffer = buffers[i];
                    buffer_info->offset = 0;
                    // NOTE: we want to access it as an array of R_Vulkan_Storage_Mesh 
                    buffer_info->range  = VK_WHOLE_SIZE;

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
                case (R_Vulkan_DescriptorSetKind_Tex2D):
                {
                    VkDescriptorImageInfo *image_info = push_array(temp.arena, VkDescriptorImageInfo, 1);
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
                default: {InvalidPath;}break;
            }
        }
    }

    // The updates are applied using vkUpdateDescriptorSets
    // It accepts two kinds of arrays as parameters: an array of VkWriteDescriptorSet and an array of VkCopyDescriptorSet
    // The latter can be used to copy descriptors to each other, as its name implies
    vkUpdateDescriptorSets(r_vulkan_state->device.h, writes_count, writes, 0, NULL);
    scratch_end(temp);
}

void r_vulkan_descriptor_set_destroy(R_Vulkan_DescriptorSet *set)
{
    VK_Assert(vkFreeDescriptorSets(r_vulkan_state->device.h, set->pool->h, 1, &set->h), "Failed to free descriptor sets");
    set->pool->cmt -= 1;
    if((set->pool->cmt+1) == set->pool->cap)
    {
        SLLStackPush(r_vulkan_state->first_avail_ds_pool[set->pool->kind], set->pool);
    }
}

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

internal
void r_vulkan_window_resize(R_Vulkan_Window *window)
{
    // Unpack some variables
    R_Vulkan_GPU *gpu             = &r_vulkan_state->gpu;
    R_Vulkan_Surface *surface     = &window->surface;
    R_Vulkan_Device *device       = &r_vulkan_state->device;

    // Handle minimization
    // There is another case where a swapchain may become out of date and that is a special kind of window resizing: window minimization
    // This case is special because it will result in a frame buffer size of 0
    // We will handle that by pausing until the window is in the foreground again int w = 0, h = 0;
    U32 w = 0;
    U32 h = 0;
    Vec2F32 dim = dim_2f32(os_client_rect_from_window(window->os_wnd));
    w = dim.x;
    h = dim.y;

    while(w == 0 || h == 0)
    {
        Temp temp = scratch_begin(0,0);
        Vec2F32 dim = dim_2f32(os_client_rect_from_window(window->os_wnd));
        w = dim.x;
        h = dim.y;

        // NOTE(k): keep processing events
        os_get_events(temp.arena, 0);
        scratch_end(temp);
    }

    // TODO(@k): The disadvantage of this approach is that we need to stop all rendering before create the new swap chain
    // It is possible to create a new swapchain while drawing commands on an image from the old swap chain are still in-flight
    // You would need to pass the previous swapchain to the oldSwapChain field in the VkSwapchainCreateInfoKHR struct and destroy 
    // the old swap chain as soon as you're finished using it
    // vkDeviceWaitIdle(device->h);

    // NOTE(@k): if format is changed, we would also need to recreate the render pass
    // In theory it can be possible for the swapchain image format to change during an applications'lifetime, e.g. when moving a window from an standard range to an high dynamic range monitor
    // This may require the application to recreate the renderpass to make sure the change between dynamic ranges is properly reflected
    VkFormat old_format = window->bag->swapchain.format;
    r_vulkan_surface_update(surface);
    SLLStackPush(window->first_to_free_bag, window->bag);
    window->bag = r_vulkan_bag(window, surface, window->bag);

    bool rendpass_outdated = window->bag->swapchain.format != old_format;
    if(!rendpass_outdated)
    {
        SLLStackPush(window->first_to_free_rendpass_grp, window->rendpass_grp);
        window->rendpass_grp = r_vulkan_rendpass_grp(window, window->bag->swapchain.format, window->rendpass_grp);
    }
    r_vulkan_rendpass_grp_submit(window->bag, window->rendpass_grp);
}

internal R_Vulkan_Bag *
r_vulkan_bag(R_Vulkan_Window *window, R_Vulkan_Surface *surface, R_Vulkan_Bag *old_bag)
{
    R_Vulkan_Bag *bag = window->first_free_bag;
    U64 gen = 0;
    R_Vulkan_Swapchain *old_swapchain = 0;

    if(old_bag != 0)
    {
        gen = old_bag->generation + 1;
        old_swapchain = &old_bag->swapchain;
    }
    if(bag == 0) { bag = push_array(window->arena, R_Vulkan_Bag, 1); }

    // Fill basic
    bag->generation = gen;
    bag->next = NULL;

    // Query format and color space
    VkFormat swp_format;
    VkColorSpaceKHR swp_color_space;
    r_vulkan_format_for_swapchain(surface->formats, surface->format_count, &swp_format, &swp_color_space);

    // Create swapchain
    bag->swapchain = r_vulkan_swapchain(surface, window->os_wnd, swp_format, swp_color_space, old_swapchain);

    R_Vulkan_Swapchain *swapchain          = &bag->swapchain;
    R_Vulkan_Image *stage_color_image      = &bag->stage_color_image;
    R_Vulkan_DescriptorSet *stage_color_ds = &bag->stage_color_ds;
    R_Vulkan_Image *geo3d_id_image         = &bag->geo3d_id_image;
    R_Vulkan_Buffer *geo3d_id_cpu         =  &bag->geo3d_id_cpu;
    R_Vulkan_Image *geo3d_color_image      = &bag->geo3d_color_image;
    R_Vulkan_DescriptorSet *geo3d_color_ds = &bag->geo3d_color_ds;
    R_Vulkan_Image *geo3d_depth_image      = &bag->geo3d_depth_image;

    // Create staging buffer and staging color sampler descriptor set
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
        create_info.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
        create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
        create_info.flags             = 0; // Optional
        VK_Assert(vkCreateImage(r_vulkan_state->device.h, &create_info, NULL, &stage_color_image->h), "Failed to create vk image");

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(r_vulkan_state->device.h, stage_color_image->h, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; 
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);
        VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &stage_color_image->memory), "Failed to allocate buffer memory");
        VK_Assert(vkBindImageMemory(r_vulkan_state->device.h, stage_color_image->h, stage_color_image->memory, 0), "Failed to map image memory");

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
        VK_Assert(vkCreateImageView(r_vulkan_state->device.h, &image_view_create_info, NULL, &stage_color_image->view), "Failed to create image view");

        // TODO(@k): we may need to transition the image layout here

        // Create staging color sampler descriptor set
        r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind_Tex2D,
                                      1, 16, NULL, &stage_color_image->view,
                                      &r_vulkan_state->samplers[R_Tex2DSampleKind_Nearest],
                                      stage_color_ds);
    }

    // geo3d_id_att
    {
        geo3d_id_image->format        = VK_FORMAT_R32G32_UINT; // Key is U64
        geo3d_id_image->extent.width  = swapchain->extent.width;
        geo3d_id_image->extent.height = swapchain->extent.height;

        VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        create_info.imageType         = VK_IMAGE_TYPE_2D;
        create_info.extent.width      = geo3d_id_image->extent.width;
        create_info.extent.height     = geo3d_id_image->extent.height;
        create_info.extent.depth      = 1;
        create_info.mipLevels         = 1;
        create_info.arrayLayers       = 1;
        create_info.format            = geo3d_id_image->format;
        create_info.tiling            = VK_IMAGE_TILING_OPTIMAL;
        create_info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        create_info.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        create_info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
        create_info.samples           = VK_SAMPLE_COUNT_1_BIT;
        create_info.flags             = 0; // Optional

        VK_Assert(vkCreateImage(r_vulkan_state->device.h, &create_info, NULL, &geo3d_id_image->h), "Failed to create vk image");

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(r_vulkan_state->device.h, geo3d_id_image->h, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);
        VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &geo3d_id_image->memory), "Failed to allocate buffer memory");
        VK_Assert(vkBindImageMemory(r_vulkan_state->device.h, geo3d_id_image->h, geo3d_id_image->memory, 0), "Failed to map image memory");

        VkImageViewCreateInfo image_view_create_info = {
            .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image                           = geo3d_id_image->h,
            .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
            .format                          = geo3d_id_image->format,
            .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel   = 0,
            .subresourceRange.levelCount     = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount     = 1,
        };
        VK_Assert(vkCreateImageView(r_vulkan_state->device.h, &image_view_create_info, NULL, &geo3d_id_image->view), "Failed to create image view");
    }

    // geo3d_id_cpu
    {
        // VkDeviceSize size = geo3d_id_image->extent.width * geo3d_id_image->extent.height * sizeof(U64);
        VkDeviceSize size = 1*1*sizeof(U64);
        VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        create_info.size = size;
        create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_Assert(vkCreateBuffer(r_vulkan_state->device.h, &create_info, NULL, &geo3d_id_cpu->h), "Failed to create vk buffer");
        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(r_vulkan_state->device.h, geo3d_id_cpu->h, &mem_requirements);
        geo3d_id_cpu->size = mem_requirements.size;

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &geo3d_id_cpu->memory), "Failed to allocate buffer memory");
        VK_Assert(vkBindBufferMemory(r_vulkan_state->device.h, geo3d_id_cpu->h, geo3d_id_cpu->memory, 0), "Failed to bind buffer memory");
        Assert(geo3d_id_cpu->size != 0);
        VK_Assert(vkMapMemory(r_vulkan_state->device.h, geo3d_id_cpu->memory, 0, geo3d_id_cpu->size, 0, &geo3d_id_cpu->mapped), "Failed to map geo3d_id_cpu memory");
    }

    // Create geo3d_color_att and its sampler descriptor set
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
        VK_Assert(vkCreateImage(r_vulkan_state->device.h, &create_info, NULL, &geo3d_color_image->h), "Failed to create vk buffer");

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(r_vulkan_state->device.h, geo3d_color_image->h, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &geo3d_color_image->memory), "Failed to allocate buffer memory");
        VK_Assert(vkBindImageMemory(r_vulkan_state->device.h, geo3d_color_image->h, geo3d_color_image->memory, 0), "Failed to bind buffer memory");

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
        VK_Assert(vkCreateImageView(r_vulkan_state->device.h, &image_view_create_info, NULL, &geo3d_color_image->view), "Failed to create image view");

        // Create geo3d_color sampler descriptor set
        r_vulkan_descriptor_set_alloc(R_Vulkan_DescriptorSetKind_Tex2D,
                                      1, 16, NULL, &geo3d_color_image->view,
                                      &r_vulkan_state->samplers[R_Tex2DSampleKind_Nearest],
                                      geo3d_color_ds);
    }

    // Create geo3d_depth_image
    {
        geo3d_depth_image->format        = r_vulkan_state->gpu.depth_image_format;
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
        create_info.format = r_vulkan_state->gpu.depth_image_format;
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
        VK_Assert(vkCreateImage(r_vulkan_state->device.h, &create_info, NULL, &geo3d_depth_image->h), "Failed to create vk buffer");

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(r_vulkan_state->device.h, geo3d_depth_image->h, &mem_requirements);

        VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = r_vulkan_memory_index_from_type_filer(mem_requirements.memoryTypeBits, properties);

        VK_Assert(vkAllocateMemory(r_vulkan_state->device.h, &alloc_info, NULL, &geo3d_depth_image->memory), "Failed to allocate buffer memory");
        VK_Assert(vkBindImageMemory(r_vulkan_state->device.h, geo3d_depth_image->h, geo3d_depth_image->memory, 0), "Failed to bind buffer memory");

        VkImageViewCreateInfo image_view_create_info = {
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
        VK_Assert(vkCreateImageView(r_vulkan_state->device.h, &image_view_create_info, NULL, &geo3d_depth_image->view), "Failed to create image view for depth image");
    }
    return bag;
}

internal void
r_vulkan_surface_update(R_Vulkan_Surface *surface)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r_vulkan_state->gpu.h, surface->h, &surface->caps);
    vkGetPhysicalDeviceSurfaceFormatsKHR(r_vulkan_state->gpu.h, surface->h, &surface->format_count, NULL);
    vkGetPhysicalDeviceSurfaceFormatsKHR(r_vulkan_state->gpu.h, surface->h, &surface->format_count, surface->formats);
    vkGetPhysicalDeviceSurfacePresentModesKHR(r_vulkan_state->gpu.h, surface->h, &surface->prest_mode_count, NULL);
    vkGetPhysicalDeviceSurfacePresentModesKHR(r_vulkan_state->gpu.h, surface->h, &surface->prest_mode_count, surface->prest_modes);
}

internal void
r_vulkan_rendpass_grp_submit(R_Vulkan_Bag *bag, R_Vulkan_RenderPassGroup *grp)
{
    for(U64 kind = 0; kind < R_Vulkan_RenderPassKind_COUNT; kind++)
    {
        VkFramebuffer *framebuffer        = &bag->framebuffers[kind];
        R_Vulkan_Swapchain *swapchain     = &bag->swapchain;
        R_Vulkan_Image *stage_color_image = &bag->stage_color_image;
        R_Vulkan_Image *geo3d_id_image    = &bag->geo3d_id_image;
        R_Vulkan_Image *geo3d_color_image = &bag->geo3d_color_image;
        R_Vulkan_Image *geo3d_depth_image = &bag->geo3d_depth_image;
        R_Vulkan_RenderPass *renderpass   = &grp->passes[kind];

        switch(kind)
        {
            case R_Vulkan_RenderPassKind_Rect:
            {
                // The attachments specified during render pass creation are bound by wrapping them into a VkFramebuffer object
                // A framebuffer object references all of the VkImageView objects that represent the attachments
                // VkFramebuffer swapchain_frame_buffers[swapchain_image_count];
                // You can only use a framebuffer with the render passes that it is compatible width
                //      which roughly means that they use the same number and type of attachments 
                VkImageView attachments[1] = { stage_color_image->view };
                VkFramebufferCreateInfo frame_buffer_create_info = {
                    .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .renderPass      = renderpass->h,
                    .attachmentCount = 1,
                    .pAttachments    = attachments,
                    .width           = swapchain->extent.width,
                    .height          = swapchain->extent.height,
                    .layers          = 1,
                };

                VK_Assert(vkCreateFramebuffer(r_vulkan_state->device.h, &frame_buffer_create_info, NULL, framebuffer), "Failed to create famebuffer");

            }break;
            case R_Vulkan_RenderPassKind_Mesh:
            {
                VkImageView attachments[3] = { geo3d_color_image->view, geo3d_id_image->view, geo3d_depth_image->view };
                VkFramebufferCreateInfo frame_buffer_create_info = {
                    .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .renderPass      = renderpass->h,
                    .attachmentCount = 3,
                    .pAttachments    = attachments,
                    .width           = swapchain->extent.width,
                    .height          = swapchain->extent.height,
                    .layers          = 1,
                };
                VK_Assert(vkCreateFramebuffer(r_vulkan_state->device.h, &frame_buffer_create_info, NULL, framebuffer), "Failed to create famebuffer");
            }break;
            case R_Vulkan_RenderPassKind_Geo3DComposite:
            {
                VkImageView attachments[1] = { stage_color_image->view };
                VkFramebufferCreateInfo frame_buffer_create_info = {
                    .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .renderPass      = renderpass->h,
                    .attachmentCount = 1,
                    .pAttachments    = attachments,
                    .width           = swapchain->extent.width,
                    .height          = swapchain->extent.height,
                    .layers          = 1,
                };
                VK_Assert(vkCreateFramebuffer(r_vulkan_state->device.h, &frame_buffer_create_info, NULL, framebuffer), "Failed to create famebuffer");
            }break;
            case R_Vulkan_RenderPassKind_Finalize: 
            {
                for(U64 i = 0; i < swapchain->image_count; i++)
                {
                    VkImageView attachments[1] = { bag->swapchain.image_views[i] };
                    VkFramebufferCreateInfo fb_create_info = {
                        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                        .renderPass      = renderpass->h,
                        .attachmentCount = 1,
                        .pAttachments    = attachments,
                        .width           = swapchain->extent.width,
                        .height          = swapchain->extent.height,
                        .layers          = 1,
                    };

                    VK_Assert(vkCreateFramebuffer(r_vulkan_state->device.h, &fb_create_info, NULL, framebuffer+i), "Failed to create famebuffer");
                }
            }break;
            default: {InvalidPath;}break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ Free resource

internal void
r_vulkan_bag_destroy(R_Vulkan_Bag *bag)
{
    // Destroy swapchain
    for(U64 i = 0; i < bag->swapchain.image_count; i++)
    {
        vkDestroyImageView(r_vulkan_state->device.h, bag->swapchain.image_views[i], NULL);
    }
    vkDestroySwapchainKHR(r_vulkan_state->device.h, bag->swapchain.h, NULL);

    for(U64 i = 0; i < R_Vulkan_RenderPassKind_COUNT; i++)
    {
        VkFramebuffer *ptr = &bag->framebuffers[i];
        U64 count = 1;
        if(i == R_Vulkan_RenderPassKind_COUNT) { count = MAX_FRAMES_IN_FLIGHT; }
        for(U64 j = 0; j < count; j++)
        {
            vkDestroyFramebuffer(r_vulkan_state->device.h, *(ptr+j), NULL);
        }
    }

    vkDestroyImage(r_vulkan_state->device.h, bag->stage_color_image.h, NULL);
    vkDestroyImageView(r_vulkan_state->device.h, bag->stage_color_image.view, NULL);
    r_vulkan_descriptor_set_destroy(&bag->stage_color_ds);
    vkFreeMemory(r_vulkan_state->device.h, bag->stage_color_image.memory, NULL);

    vkDestroyImage(r_vulkan_state->device.h, bag->geo3d_id_image.h, NULL);
    vkDestroyImageView(r_vulkan_state->device.h, bag->geo3d_id_image.view, NULL);
    vkUnmapMemory(r_vulkan_state->device.h, bag->geo3d_id_cpu.memory);
    vkDestroyBuffer(r_vulkan_state->device.h, bag->geo3d_id_cpu.h, NULL);
    vkFreeMemory(r_vulkan_state->device.h, bag->geo3d_id_cpu.memory, NULL);

    vkDestroyImage(r_vulkan_state->device.h, bag->geo3d_color_image.h, NULL);
    vkDestroyImageView(r_vulkan_state->device.h, bag->geo3d_color_image.view, NULL);
    r_vulkan_descriptor_set_destroy(&bag->geo3d_color_ds);
    vkFreeMemory(r_vulkan_state->device.h, bag->geo3d_color_image.memory, NULL);

    vkDestroyImage(r_vulkan_state->device.h, bag->geo3d_depth_image.h, NULL);
    vkDestroyImageView(r_vulkan_state->device.h, bag->geo3d_depth_image.view, NULL);
    vkFreeMemory(r_vulkan_state->device.h, bag->geo3d_depth_image.memory, NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ Helpers

internal void
r_vulkan_rendpass_grp_destroy(R_Vulkan_RenderPassGroup *grp)
{
    for(U64 kind = 0; kind < R_Vulkan_RenderPassKind_COUNT; kind++)
    {
        R_Vulkan_RenderPass *rendpass = &grp->passes[kind];
        vkDestroyPipelineLayout(r_vulkan_state->device.h, rendpass->pipeline.first.layout, NULL);
        vkDestroyPipeline(r_vulkan_state->device.h, rendpass->pipeline.first.h, NULL);
        vkDestroyRenderPass(r_vulkan_state->device.h, rendpass->h, NULL);
    }
}

internal R_Vulkan_Buffer *
r_vulkan_buffer_from_handle(R_Handle handle)
{
    U64 generation = handle.u64[1];
    R_Vulkan_Buffer *buffer = (R_Vulkan_Buffer *)handle.u64[0];
    AssertAlways(buffer->generation == generation);
    return buffer;
}
