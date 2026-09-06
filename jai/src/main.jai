#import "Basic";
#import "File";
#import "Input";

#import "Window_Creation";
#import "Vulkan"; // we use this instead of "jai/modules"

#if OS == .WINDOWS {
    Win32 :: #import "Windows";
}

the_window : Window_Type;
window_width : u32 = 1200;
window_height : u32 = 800;

main :: () {
    the_window = create_window(window_width, window_height, "Hello, Sailor!");

    init_vulkan();
    main_loop();
    clean_up();
}


main_loop :: () {
    quit := false;
    while !quit {
        update_window_events();
        for events_this_frame {
            if it.type == {
                case .QUIT; quit = true;
                case .KEYBOARD;
                    if !it.key_pressed continue;
                    if it.key_code == .ESCAPE then quit = true;
            }
        }

        // The swapchain is tied to the size of the window, so a resize invalidates it.
        for get_window_resizes() {
            if it.window != the_window  continue;
            window_width  = it.width.(u32);
            window_height = it.height.(u32);
            swapchain_is_stale = true;
        }

        draw_one_frame();
    }

    vkDeviceWaitIdle(device);
}

// ----------------------------------------------------------------------------: Config

DEBUG :: true; // if enable VALIDATION

VALIDATION_LAYER_NAME :: "VK_LAYER_KHRONOS_validation";
VULKAN_API_VERSION    :: VK_API_VERSION_1_3;

// Everything we cannot draw a triangle without.
REQUIRED_DEVICE_EXTENSIONS :: string.[
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
];

// Queue families are addressed by index, this stands for "we did not find one".
QUEUE_FAMILY_NONE : u32 : 0xFFFF_FFFF;

// Passed to the calls that can block until the GPU catches up. There is nothing
// sensible for us to do on a timeout, so we simply never time out.
TIMEOUT_NEVER : u64 : 0xFFFF_FFFF_FFFF_FFFF;

// ----------------------------------------------------------------------------: Vulkan

init_vulkan :: () {
    create_instance();
    create_debug_messenger();
    create_surface();
    pick_physical_device();
    create_device();
    create_swapchain();
    create_command_buffers();
    create_sync_objects();
    create_pipeline();
}

// ----------------------------------------------------------------------------: Instance

instance        : VkInstance;
debug_messenger : VkDebugUtilsMessengerEXT;

create_instance :: () {
    // See: Jai - Temporary_Storage (我们使用了一些动态数组，他们之后不再使用了，仅为了创建 Instance 而存在)
    auto_release_temp();
    push_allocator(temp);

    // Step 1: Query: get all available extensions from vulkan environment.
    available_extensions := vkEnumerateInstanceExtensionProperties(null);

    // Step 2: Decide which extensions we must need.
    required_extensions: [..] string;

    // Surface extension, Genericly we need VK_KHR_surface,
    // and the others depend on the platform we run on.
    array_add(*required_extensions, VK_KHR_SURFACE_EXTENSION_NAME);
    #if OS == .WINDOWS {
        array_add(*required_extensions, VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    } else #if OS == .LINUX {
        // @NOTE @TODO 虽然但是，后面的内容还没有适配 Linux，当前我只是适配了 Windows
        array_add(*required_extensions, "VK_KHR_xlib_surface");
    } else #if OS == .MACOS {
        // @NOTE @TODO 虽然但是，后面的内容还没有适配 MACOS，当前我只是适配了 Windows
        array_add(*required_extensions, "VK_EXT_metal_surface");
    }

    // Step 3: check if all of them can be enabled. (if failed, panic)
    enabled_extensions: [..] *u8;
    for required_extensions {
        assert(has_extension(available_extensions, it), "The Vulkan loader does not have the required instance extension '%'.", it);
        array_add(*enabled_extensions, it.data);
    }

    // The layer reports what it finds through VK_EXT_debug_utils.
    // If either of the two is missing we simply run without any diagnostics instead of giving up.
    enabled_layers: [..] *u8;
    #if DEBUG {
        if has_layer(VALIDATION_LAYER_NAME) && has_extension(available_extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) {
            array_add(*enabled_layers,     VALIDATION_LAYER_NAME.data);
            array_add(*enabled_extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME.data);
        } else {
            log("Validation was requested but '%' is not installed, running without it.", VALIDATION_LAYER_NAME);
        }
    }

    // Just provide some information about our application.
    // thanks to jai default values of struct, we don't need to write sType manually.
    app_info := VkApplicationInfo.{
        pApplicationName   = "Vulkan Example",
        pEngineName        = "Poison QIEKN",
        applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        apiVersion         = VULKAN_API_VERSION,
    };

    create_info := VkInstanceCreateInfo.{
        pApplicationInfo        = *app_info,
        enabledLayerCount       = enabled_layers.count.(u32),    // .(u32) is equal to cast(u32), count is s64 originally
        ppEnabledLayerNames     = enabled_layers.data,           // pp means pointer to pointer (二级指针)
        enabledExtensionCount   = enabled_extensions.count.(u32),
        ppEnabledExtensionNames = enabled_extensions.data,
    };

    // The messenger below is created *after* the instance exists,
    // If something wrong in current procedure, how we konw?
    // so we chain one here to catch mistakes in vkCreateInstance and vkDestroyInstance.
    messenger_info := make_debug_messenger_info();
    if enabled_layers.count > 0 {
        create_info.pNext = *messenger_info;
    }

    result := vkCreateInstance(*create_info, null, *instance);
    assert(result == .SUCCESS, "vkCreateInstance failed: %", result);
}

has_extension :: (extensions: [] VkExtensionProperties, name: string) -> bool {
    for extensions {
        if to_string(it.extensionName) == name
            return true;
    }
    return false;
}

has_layer :: (name: string) -> bool {
    layers := vkEnumerateInstanceLayerProperties();
    for layers {
        if to_string(it.layerName) == name
            return true;
    }
    return false;
}

// ----------------------------------------------------------------------------: Debug messenger

// What we want to receive
make_debug_messenger_info :: () -> VkDebugUtilsMessengerCreateInfoEXT {
    return .{
        messageSeverity = .WARNING_BIT_EXT | .ERROR_BIT_EXT,
        messageType     = .GENERAL_BIT_EXT | .VALIDATION_BIT_EXT | .PERFORMANCE_BIT_EXT,
        pfnUserCallback = vulkan_debug_callback, // pfn: pointer to function
    };
}

create_debug_messenger :: () {
    #if DEBUG {
        // Vulkan Loader, please tell me where is the function named "xxxx"
        vkCreateDebugUtilsMessengerEXT: PFN_vkCreateDebugUtilsMessengerEXT =
            xx vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if !vkCreateDebugUtilsMessengerEXT  return; // The layer is not loaded.

        info := make_debug_messenger_info();
        result := vkCreateDebugUtilsMessengerEXT(instance, *info, null, *debug_messenger);
        if result != .SUCCESS {
            log_error("Could not create the debug messenger: %", result);
        }
    }
}

destroy_debug_messenger :: () {
    if !debug_messenger  return;

    vkDestroyDebugUtilsMessengerEXT: PFN_vkDestroyDebugUtilsMessengerEXT =
        xx vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if vkDestroyDebugUtilsMessengerEXT {
        vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, null);
    }
    debug_messenger = null;
}

vulkan_debug_callback :: ( severity: VkDebugUtilsMessageSeverityFlagBitsEXT,
                           types: VkDebugUtilsMessageTypeFlagsEXT,
                           callback_data: *VkDebugUtilsMessengerCallbackDataEXT,
                           user_data: *void
                         ) -> VkBool32 #c_call {

    // Severity
    if      (severity & .ERROR_BIT_EXT)   != 0 { write_strings("[Vulkan ERROR]");   }
    else if (severity & .WARNING_BIT_EXT) != 0 { write_strings("[Vulkan WARNING]"); }
    else if (severity & .INFO_BIT_EXT)    != 0 { write_strings("[Vulkan INFO]");    }
    else                                       { write_strings("[Vulkan VERBOSE]"); }

    // @TODO we just ingore message type here [Validation / Performance / General]

    write_strings("[Vulkan] ", to_string(callback_data.pMessage), "\n");

    return VK_FALSE; // do not abort the call that triggered this message
}

// ----------------------------------------------------------------------------: Surface

surface : VkSurfaceKHR;

create_surface :: () {
    #if OS == .WINDOWS {
        create_info := VkWin32SurfaceCreateInfoKHR.{
            hinstance = xx Win32.GetModuleHandleW(null),
            hwnd      = the_window,
        };
        result := vkCreateWin32SurfaceKHR(instance, *create_info, null, *surface);
        assert(result == .SUCCESS, "vkCreateWin32SurfaceKHR failed: %", result);
    } else {
        // X11 Window
        // Wayland Surface
        #assert false "Surface creation is only implemented for Windows so far.";
    }
}

// ----------------------------------------------------------------------------: Physical device

physical_device            : VkPhysicalDevice;
physical_device_properties : VkPhysicalDeviceProperties2;
graphics_family            : u32 = QUEUE_FAMILY_NONE;

pick_physical_device :: () {
    auto_release_temp();
    push_allocator(temp);

    devices := vkEnumeratePhysicalDevices(instance);
    assert(devices.count > 0, "Could not find a GPU with Vulkan support.");

    best_score := -1;
    for devices {
        score, family := rate_physical_device(it);
        if score > best_score {
            best_score      = score;
            physical_device = it;
            graphics_family = family;
        }
    }
    assert(best_score >= 0, "None of the % available GPUs meet our requirements.", devices.count);

    vkGetPhysicalDeviceProperties2(physical_device, *physical_device_properties);

    api := physical_device_properties.properties.apiVersion;
    log("Using GPU: % (Vulkan %.%.%)", to_string(physical_device_properties.properties.deviceName),
                  VK_API_VERSION_MAJOR(api), VK_API_VERSION_MINOR(api), VK_API_VERSION_PATCH(api));
}

// Returns a negative score for devices we cannot use at all. (负分就是不合格)
rate_physical_device :: (candidate: VkPhysicalDevice) -> score: int, graphics_family: u32 {
    // suffixed with a 2 (Vulkan 就这样，为了修之前的缺陷，如果直接修改原来的就会破坏原 API 兼容性，所示添加了后缀 2)
    properties: VkPhysicalDeviceProperties2;
    vkGetPhysicalDeviceProperties2(candidate, *properties);
    name := to_string(properties.properties.deviceName);

    if properties.properties.apiVersion < VULKAN_API_VERSION {
        log("Skipping %: it only implements Vulkan %.%.", name,
            VK_API_VERSION_MAJOR(properties.properties.apiVersion),
            VK_API_VERSION_MINOR(properties.properties.apiVersion));
        return -1, QUEUE_FAMILY_NONE;
    }

    available_extensions := vkEnumerateDeviceExtensionProperties(candidate, null);
    for REQUIRED_DEVICE_EXTENSIONS {
        if !has_extension(available_extensions, it) {
            log("Skipping %: it does not support '%'.", name, it);
            return -1, QUEUE_FAMILY_NONE;
        }
    }

    vulkan_11: VkPhysicalDeviceVulkan11Features;
    vulkan_13: VkPhysicalDeviceVulkan13Features;
    features: VkPhysicalDeviceFeatures2;

    features.pNext = *vulkan_11;
    vulkan_11.pNext = *vulkan_13;

    vkGetPhysicalDeviceFeatures2(candidate, *features);

    if vulkan_11.shaderDrawParameters == VK_FALSE {
        log("Skipping %: no shaderDrawParameters.", name);
        return -1, QUEUE_FAMILY_NONE;
    }

    if vulkan_13.dynamicRendering == VK_FALSE || vulkan_13.synchronization2 == VK_FALSE {
        log("Skipping %: no dynamic rendering or no synchronization2.", name);
        return -1, QUEUE_FAMILY_NONE;
    }

    family := find_graphics_queue_family(candidate);
    if family == QUEUE_FAMILY_NONE {
        log("Skipping %: no queue family that can both render and present.", name);
        return -1, QUEUE_FAMILY_NONE;
    }

    // A discrete GPU is almost always the one the user wants to render with.
    score: int;
    if properties.properties.deviceType == {
        case .DISCRETE_GPU;   score = 1000;
        case .VIRTUAL_GPU;    score = 100;
        case .INTEGRATED_GPU; score = 10;
        case;                 score = 1;
    }
    return score, family;
}

// @NOTE Commands are not sent to the device directly, they go into a queue, and
// every queue belongs to a family that supports a certain set of operations. 
find_graphics_queue_family :: (candidate: VkPhysicalDevice) -> u32 {
    families := vkGetPhysicalDeviceQueueFamilyProperties(candidate);

    for families {
        supports_graphics := (it.queueFlags & .GRAPHICS_BIT) != 0;
        if !supports_graphics  continue;

        index := it_index.(u32);

        // Being able to present at all is a platform question,
        // being able to present to *our* surface is a driver question.
        // *Both* have to say yes.

        #if OS == .WINDOWS {
            if vkGetPhysicalDeviceWin32PresentationSupportKHR(candidate, index) == VK_FALSE  continue;
        }

        supported: VkBool32;
        vkGetPhysicalDeviceSurfaceSupportKHR(candidate, index, surface, *supported);
        if supported == VK_FALSE  continue;

        return index;
    }
    return QUEUE_FAMILY_NONE;
}

// ----------------------------------------------------------------------------: Logical device

device         : VkDevice;
graphics_queue : VkQueue;

create_device :: () {
    auto_release_temp();
    push_allocator(temp);

    queue_priority : float = 1.0;
    queue_info := VkDeviceQueueCreateInfo.{
        queueFamilyIndex = graphics_family,
        queueCount       = 1,
        pQueuePriorities = *queue_priority,
    };

    // @NOTE maybe create a similar util one day to simply jai code here
    // https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/04_Logical_device_and_queues.html
    /*
        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                           vk::PhysicalDeviceVulkan11Features,
                           vk::PhysicalDeviceVulkan13Features,
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> feature_chain = {
            {.features = {.samplerAnisotropy = true}},
            {.shaderDrawParameters = true},
            {.synchronization2 = true, .dynamicRendering = true},
            {.extendedDynamicState = true},
        };
    */

    vulkan_11 := VkPhysicalDeviceVulkan11Features.{ shaderDrawParameters = VK_TRUE, };
    vulkan_13 := VkPhysicalDeviceVulkan13Features.{ dynamicRendering = VK_TRUE, synchronization2 = VK_TRUE, };
    vulkan_11.pNext = *vulkan_13;

    enabled_extensions: [..] *u8;
    for REQUIRED_DEVICE_EXTENSIONS {
        array_add(*enabled_extensions, it.data);
    }

    create_info := VkDeviceCreateInfo.{
        pNext                   = *vulkan_11,
        queueCreateInfoCount    = 1,
        pQueueCreateInfos       = *queue_info,
        enabledExtensionCount   = enabled_extensions.count.(u32),
        ppEnabledExtensionNames = enabled_extensions.data,
    };

    result := vkCreateDevice(physical_device, *create_info, null, *device);
    assert(result == .SUCCESS, "vkCreateDevice failed: %", result);

    vkGetDeviceQueue(device, graphics_family, 0, *graphics_queue);
}

// ----------------------------------------------------------------------------: Swapchain

swapchain             : VkSwapchainKHR;
swapchain_format      : VkFormat;
swapchain_color_space : VkColorSpaceKHR;
swapchain_extent      : VkExtent2D;
swapchain_images      : [] VkImage;
swapchain_image_views : [] VkImageView;

// Set when the window changed size or the driver told us the swapchain no longer
// matches the surface. The next frame rebuilds it before drawing anything.
swapchain_is_stale := false;

create_swapchain :: () {
    // @NOTE no temporary allocator here.
    // The image and semaphore arrays below have to outlive this call,
    // they will be released in destroy_swapchain_resources().
    caps: VkSurfaceCapabilitiesKHR;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, *caps);

    swapchain_extent = caps.currentExtent;
    if caps.currentExtent.width == 0xFFFF_FFFF {
        swapchain_extent = .{width = window_width, height = window_height};
    }

    // Minimized window
    if swapchain_extent.width == 0 || swapchain_extent.height == 0  return;

    swapchain_format, swapchain_color_space = choose_surface_format();

    old_swapchain := swapchain;
    create_info := VkSwapchainCreateInfoKHR.{
        surface          = surface,
        minImageCount    = caps.minImageCount,
        imageFormat      = swapchain_format,
        imageColorSpace  = swapchain_color_space,
        imageExtent      = swapchain_extent,
        imageArrayLayers = 1,
        imageUsage       = .COLOR_ATTACHMENT_BIT,
        preTransform     = caps.currentTransform,
        compositeAlpha   = .OPAQUE_BIT_KHR,

        // FIFO is vsync, and it is the only mode every driver has to support.
        presentMode      = .FIFO_KHR,
        clipped          = VK_TRUE,

        // Handing the old one over lets the driver reuse its resources.
        oldSwapchain     = old_swapchain,
    };

    result := vkCreateSwapchainKHR(device, *create_info, null, *swapchain);
    assert(result == .SUCCESS, "vkCreateSwapchainKHR failed: %", result);

    if old_swapchain  vkDestroySwapchainKHR(device, old_swapchain, null);

    // Images & ImageViews

    images, images_result := vkGetSwapchainImagesKHR(device, swapchain);
    assert(images_result == .SUCCESS, "vkGetSwapchainImagesKHR failed: %", images_result);

    swapchain_images      = images;
    swapchain_image_views = NewArray(swapchain_images.count, VkImageView);

    for swapchain_images {
        view_info := VkImageViewCreateInfo.{
            image    = it,
            viewType = ._2D,
            format   = swapchain_format,
            subresourceRange = .{aspectMask = .COLOR_BIT, levelCount = 1, layerCount = 1},
        };
        view_result := vkCreateImageView(device, *view_info, null, *swapchain_image_views[it_index]);
        assert(view_result == .SUCCESS, "vkCreateImageView failed: %", view_result);
    }

    // One semaphore per image rather than per frame
    render_finished_semaphores = NewArray(swapchain_images.count, VkSemaphore);
    for * render_finished_semaphores {
        semaphore_info: VkSemaphoreCreateInfo;
        semaphore_result := vkCreateSemaphore(device, *semaphore_info, null, it);
        assert(semaphore_result == .SUCCESS, "vkCreateSemaphore failed: %", semaphore_result);
    }

    swapchain_is_stale = false;
}

// Prefer an sRGB swapchain format. We do rendering calculations in linear space,
// and an sRGB color attachment encodes the final output for presentation.
choose_surface_format :: () -> VkFormat, VkColorSpaceKHR {
    auto_release_temp();
    push_allocator(temp);

    formats := vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface);
    assert(formats.count > 0, "The surface does not support any format at all.");

    for formats {
        if it.format == .B8G8R8A8_SRGB && it.colorSpace == .COLOR_SPACE_SRGB_NONLINEAR_KHR {
            return it.format, it.colorSpace;
        }
    }
    return formats[0].format, formats[0].colorSpace;
}

// Everything here is recreated whenever the window changes size. The swapchain
// itself is not destroyed: it is passed to the new one as `oldSwapchain`.
destroy_swapchain_resources :: () {
    for render_finished_semaphores  vkDestroySemaphore(device, it, null);
    for swapchain_image_views       vkDestroyImageView(device, it, null);

    array_free(render_finished_semaphores);
    array_free(swapchain_image_views);
    array_free(swapchain_images);

    render_finished_semaphores = .[];
    swapchain_image_views      = .[];
    swapchain_images           = .[];
}

recreate_swapchain :: () {
    // The old views and semaphores may still be referenced by frames in flight.
    vkDeviceWaitIdle(device);

    destroy_swapchain_resources();
    create_swapchain();
}

// ----------------------------------------------------------------------------: Commands and synchronization

MAX_FRAMES_IN_FLIGHT :: 2;

command_pool    : VkCommandPool;
command_buffers : [MAX_FRAMES_IN_FLIGHT] VkCommandBuffer;

// Fences synchronize the GPU with us,
// Semaphores synchronize GPU work with itself.
in_flight_fences           : [MAX_FRAMES_IN_FLIGHT] VkFence;
image_available_semaphores : [MAX_FRAMES_IN_FLIGHT] VkSemaphore;
render_finished_semaphores : [] VkSemaphore; // One per swapchain image, see create_swapchain().

frame_index : u32;

// We never call the GPU directly.
// We record commands into a command buffer,
// which is allocated out of a pool that belongs to one queue family.
create_command_buffers :: () {
    // Command Pool
    pool_info := VkCommandPoolCreateInfo.{
        flags            = .RESET_COMMAND_BUFFER_BIT,
        queueFamilyIndex = graphics_family,
    };
    result := vkCreateCommandPool(device, *pool_info, null, *command_pool);
    assert(result == .SUCCESS, "vkCreateCommandPool failed: %", result);

    // Command Buffer
    alloc_info := VkCommandBufferAllocateInfo.{
        commandPool        = command_pool,
        level              = .PRIMARY,
        commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };
    result = vkAllocateCommandBuffers(device, *alloc_info, command_buffers.data);
    assert(result == .SUCCESS, "vkAllocateCommandBuffers failed: %", result);
}

create_sync_objects :: () {
    for 0..MAX_FRAMES_IN_FLIGHT-1 {
        fence_info := VkFenceCreateInfo.{flags = .SIGNALED_BIT}; // signaled by default, otherwise wait forever
        fence_result := vkCreateFence(device, *fence_info, null, *in_flight_fences[it]);
        assert(fence_result == .SUCCESS, "vkCreateFence failed: %", fence_result);

        semaphore_info: VkSemaphoreCreateInfo;
        semaphore_result := vkCreateSemaphore(device, *semaphore_info, null, *image_available_semaphores[it]);
        assert(semaphore_result == .SUCCESS, "vkCreateSemaphore failed: %", semaphore_result);
    }
}

// ----------------------------------------------------------------------------: Pipeline

SHADER_PATH :: "assets/shaders/triangle.spv";

pipeline        : VkPipeline;
pipeline_layout : VkPipelineLayout;

create_pipeline :: () {
    shader_module := load_shader_module(SHADER_PATH);
    defer vkDestroyShaderModule(device, shader_module, null); // only needed while creating the pipeline.

    // Both entry points come out of the same SPIR-V module, they just differ in stage.
    stages := VkPipelineShaderStageCreateInfo.[
        .{stage = .VERTEX_BIT,   module = shader_module, pName = "vertex_main".data},
        .{stage = .FRAGMENT_BIT, module = shader_module, pName = "fragment_main".data},
    ];

    // Just a triangle in this example, so vertices are contsnats insider the shader, nothing to describe here.
    vertex_input: VkPipelineVertexInputStateCreateInfo;

    input_assembly := VkPipelineInputAssemblyStateCreateInfo.{topology = .TRIANGLE_LIST};

    // Viewport and scissor are set while recording instead of being baked in,
    // so a window resize does not force us to build a new pipeline.
    dynamic_states := VkDynamicState.[.VIEWPORT, .SCISSOR];
    dynamic_state := VkPipelineDynamicStateCreateInfo.{
        dynamicStateCount = dynamic_states.count.(u32),
        pDynamicStates    = dynamic_states.data,
    };

    // how many viewprot? how many scissor?
    viewport_state := VkPipelineViewportStateCreateInfo.{viewportCount = 1, scissorCount = 1};

    rasterization := VkPipelineRasterizationStateCreateInfo.{
        polygonMode = .FILL,
        cullMode    = .NONE, // With one triangle we do not care which way it faces.
        frontFace   = .COUNTER_CLOCKWISE,
        lineWidth   = 1.0,
    };

    multisample := VkPipelineMultisampleStateCreateInfo.{rasterizationSamples = ._1_BIT}; // no MSAA

    // No blending, but we still have to say which channels may be written at all.
    blend_attachment := VkPipelineColorBlendAttachmentState.{colorWriteMask = xx 0xF}; // RGBA
    color_blend := VkPipelineColorBlendStateCreateInfo.{
        attachmentCount = 1,
        pAttachments    = *blend_attachment,
    };

    // The pipeline has no descriptor sets and no push constants:
    // the shader reads nothing from the outside world.
    layout_info: VkPipelineLayoutCreateInfo;
    layout_result := vkCreatePipelineLayout(device, *layout_info, null, *pipeline_layout);
    assert(layout_result == .SUCCESS, "vkCreatePipelineLayout failed: %", layout_result);

    // With dynamic rendering there is no VkRenderPass to be compatible with, so the
    // pipeline is told directly which attachment formats it will be used against.
    rendering_info := VkPipelineRenderingCreateInfo.{
        colorAttachmentCount    = 1,
        pColorAttachmentFormats = *swapchain_format,
    };

    pipeline_info := VkGraphicsPipelineCreateInfo.{
        pNext               = *rendering_info,
        stageCount          = stages.count.(u32),
        pStages             = stages.data,
        pVertexInputState   = *vertex_input,
        pInputAssemblyState = *input_assembly,
        pViewportState      = *viewport_state,
        pRasterizationState = *rasterization,
        pMultisampleState   = *multisample,
        pColorBlendState    = *color_blend,
        pDynamicState       = *dynamic_state,
        layout              = pipeline_layout,
    };

    result := vkCreateGraphicsPipelines(device, null, 1, *pipeline_info, null, *pipeline);
    assert(result == .SUCCESS, "vkCreateGraphicsPipelines failed: %", result);
}

// The build step (see first.jai) compiles the .slang source into this .spv file.
load_shader_module :: (path: string) -> VkShaderModule {
    code, ok := read_entire_file(path);
    assert(ok, "Could not read '%'. Run 'jai first.jai' from the repository root so that the shader gets compiled.", path);
    defer free(code);

    // SPIR-V is a stream of 32 bit words, but the size is counted in bytes.
    info := VkShaderModuleCreateInfo.{
        codeSize = code.count.(u64),
        pCode    = code.data.(*u32),
    };

    shader_module: VkShaderModule;
    result := vkCreateShaderModule(device, *info, null, *shader_module);
    assert(result == .SUCCESS, "vkCreateShaderModule failed for '%': %", path, result);
    return shader_module;
}

// ----------------------------------------------------------------------------: Drawing

draw_one_frame :: () {
    // Check swapchain
    if swapchain_is_stale  recreate_swapchain();
    if swapchain_is_stale  return; // Still stale means the window is minimized.

    vkWaitForFences(device, 1, *in_flight_fences[frame_index], VK_TRUE, TIMEOUT_NEVER);

    result : VkResult;

    image_index: u32;
    result = vkAcquireNextImageKHR(device, swapchain, TIMEOUT_NEVER, image_available_semaphores[frame_index], null, *image_index);
    if result == .ERROR_OUT_OF_DATE_KHR {
        swapchain_is_stale = true;
        return;
    }
    assert(result == .SUCCESS || result == .SUBOPTIMAL_KHR, "vkAcquireNextImageKHR failed: %", result);

    vkResetFences(device, 1, *in_flight_fences[frame_index]);

    command_buffer := command_buffers[frame_index];
    vkResetCommandBuffer(command_buffer, xx 0);
    record_command_buffer(command_buffer, image_index);

    // The GPU must not write to the image before the presentation engine is done reading it
    wait_info := VkSemaphoreSubmitInfo.{
        semaphore = image_available_semaphores[frame_index],
        stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    command_buffer_info := VkCommandBufferSubmitInfo.{commandBuffer = command_buffer};
    signal_info := VkSemaphoreSubmitInfo.{
        semaphore = render_finished_semaphores[image_index],
        stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    submit_info := VkSubmitInfo2.{
        waitSemaphoreInfoCount   = 1,
        pWaitSemaphoreInfos      = *wait_info,
        commandBufferInfoCount   = 1,
        pCommandBufferInfos      = *command_buffer_info,
        signalSemaphoreInfoCount = 1,
        pSignalSemaphoreInfos    = *signal_info,
    };
    result = vkQueueSubmit2(graphics_queue, 1, *submit_info, in_flight_fences[frame_index]);
    assert(result == .SUCCESS, "vkQueueSubmit2 failed: %", result);

    present_info := VkPresentInfoKHR.{
        waitSemaphoreCount = 1,
        pWaitSemaphores    = *render_finished_semaphores[image_index],
        swapchainCount     = 1,
        pSwapchains        = *swapchain,
        pImageIndices      = *image_index,
    };
    result = vkQueuePresentKHR(graphics_queue, *present_info);
    if result == .ERROR_OUT_OF_DATE_KHR || result == .SUBOPTIMAL_KHR {
        swapchain_is_stale = true;
    } else {
        assert(result == .SUCCESS, "vkQueuePresentKHR failed: %", result);
    }

    frame_index = (frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

record_command_buffer :: (command_buffer: VkCommandBuffer, image_index: u32) {
    begin_info := VkCommandBufferBeginInfo.{flags = .ONE_TIME_SUBMIT_BIT};
    chk(vkBeginCommandBuffer(command_buffer, *begin_info));

    // Change swapchain image layout: undefined -> attachment optimal
    //
    // @NOTE the FlagBits2 values are 64 bit constants rather than a Jai enum, so
    // they have to be spelled out in full instead of using the leading-dot shorthand.
    to_attachment := VkImageMemoryBarrier2.{
        srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        srcAccessMask = 0,
        dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        oldLayout     = .UNDEFINED,
        newLayout     = .ATTACHMENT_OPTIMAL,
        image         = swapchain_images[image_index],
        subresourceRange = .{aspectMask = .COLOR_BIT, levelCount = 1, layerCount = 1},
    };
    to_attachment_dependency := VkDependencyInfo.{
        imageMemoryBarrierCount = 1,
        pImageMemoryBarriers    = *to_attachment,
    };
    vkCmdPipelineBarrier2(command_buffer, *to_attachment_dependency);

    clear_value: VkClearValue;
    clear_value.color._float32 = .[0.02, 0.02, 0.05, 1.0];

    color_attachment := VkRenderingAttachmentInfo.{
        imageView   = swapchain_image_views[image_index],
        imageLayout = .ATTACHMENT_OPTIMAL,
        loadOp      = .CLEAR, // Wipe the image at the start of the pass...
        storeOp     = .STORE, // ...and keep what we drew when it ends.
        clearValue  = clear_value,
    };

    // Dynamic rendering: we describe the attachments right here
    rendering_info := VkRenderingInfo.{
        renderArea           = .{extent = swapchain_extent},
        layerCount           = 1,
        colorAttachmentCount = 1,
        pColorAttachments    = *color_attachment,
    };
    vkCmdBeginRendering(command_buffer, *rendering_info);

    viewport := VkViewport.{
        width    = swapchain_extent.width.(float),
        height   = swapchain_extent.height.(float),
        minDepth = 0.0,
        maxDepth = 1.0,
    };
    scissor := VkRect2D.{extent = swapchain_extent};

    vkCmdSetViewport(command_buffer, 0, 1, *viewport);
    vkCmdSetScissor(command_buffer, 0, 1, *scissor);
    vkCmdBindPipeline(command_buffer, .GRAPHICS, pipeline);

    // Three vertices, one instance. The vertex shader turns the vertex index into
    // a position and a color all by itself.
    vkCmdDraw(command_buffer, 3, 1, 0, 0);

    vkCmdEndRendering(command_buffer);

    // Change layout again: Attachment optimal -> Present SRC(source)
    to_present := VkImageMemoryBarrier2.{
        srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        dstAccessMask = 0,
        oldLayout     = .ATTACHMENT_OPTIMAL,
        newLayout     = .PRESENT_SRC_KHR,
        image         = swapchain_images[image_index],
        subresourceRange = .{aspectMask = .COLOR_BIT, levelCount = 1, layerCount = 1},
    };
    to_present_dependency := VkDependencyInfo.{
        imageMemoryBarrierCount = 1,
        pImageMemoryBarriers    = *to_present,
    };
    vkCmdPipelineBarrier2(command_buffer, *to_present_dependency);

    chk(vkEndCommandBuffer(command_buffer));
}

// ----------------------------------------------------------------------------: Clean up

// Vulkan objects have to go in the reverse order of their creation
clean_up :: () {
    vkDestroyPipeline(device, pipeline, null);
    vkDestroyPipelineLayout(device, pipeline_layout, null);

    for in_flight_fences            vkDestroyFence(device, it, null);
    for image_available_semaphores  vkDestroySemaphore(device, it, null);

    vkDestroyCommandPool(device, command_pool, null);

    destroy_swapchain_resources();
    vkDestroySwapchainKHR(device, swapchain, null);

    vkDestroyDevice(device, null);
    vkDestroySurfaceKHR(instance, surface, null);
    destroy_debug_messenger();
    vkDestroyInstance(instance, null);
}

// ----------------------------------------------------------------------------: utils

// @TODO
chk :: (call: Code) #expand {
    result := #insert call;
    assert(result == .SUCCESS, "Vulkan call failed: , result\n", result);
}
