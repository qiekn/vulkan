#include <cassert>
#include <cstdlib>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

import vulkan;
import std;

#ifdef NDEBUG
constexpr bool kEnableValidationLayers = false;
#else
constexpr bool kEnableValidationLayers = true;
#endif

constexpr size_t kMAX_FRAMES_IN_FLIGHT = 2;

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
  glm::vec2 tex_coord;

  static vk::VertexInputBindingDescription GetBindingDescription() {
    return {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex,
    };
  }

  static std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescriptions() {
    return {{
        {.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, pos)},
        {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)},
        {.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, tex_coord)},
    }};
  }
};

const std::vector<Vertex> kVertices = {
    {{-0.95f, -0.95f}, {1.0f, 0.0f, 0.0f}, {2.0f, 0.0f}},
    {{0.95f, -0.95f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.95f, 0.95f}, {0.0f, 0.0f, 1.0f}, {0.0f, 2.0f}},
    {{-0.95f, 0.95f}, {1.0f, 1.0f, 1.0f}, {2.0f, 2.0f}},
};

const std::vector<uint16_t> kIndices = {
    0, 1, 2,
    2, 3, 0,
};

struct UniformBufferObject {
  glm::vec2 qiekn; // 8 bytes  [ 8 bytes ]
  alignas(16)glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

class HelloTriangleApplication {
public:
  void Run() {
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();
  }

private:
  GLFWwindow* window_ = nullptr;

  vk::raii::Context context_;
  vk::raii::Instance instance_ = nullptr;
  vk::raii::DebugUtilsMessengerEXT debug_messenger_ = nullptr;
  vk::raii::PhysicalDevice physical_device_ = nullptr;
  vk::raii::Device device_ = nullptr;
  vk::raii::Queue graphics_queue_ = nullptr;
  uint32_t graphics_queue_family_ = 0;
  vk::raii::SurfaceKHR surface_ = nullptr;
  vk::raii::SwapchainKHR swapchain_ = nullptr;
  std::vector<vk::Image> swapchain_images_;
  vk::SurfaceFormatKHR swapchain_format_;
  vk::Extent2D swapchain_extent_;
  std::vector<vk::raii::ImageView> swapchain_image_views_;
  vk::raii::DescriptorSetLayout descriptor_set_layout_ = nullptr;
  vk::raii::PipelineLayout pipeline_layout_ = nullptr;
  vk::raii::Pipeline graphics_pipeline_ = nullptr;

  vk::raii::Buffer vertex_buffer_ = nullptr;
  vk::raii::DeviceMemory vertex_buffer_memory_ = nullptr;

  vk::raii::Buffer index_buffer_ = nullptr;
  vk::raii::DeviceMemory index_buffer_memory_ = nullptr;

  vk::raii::Image texture_image_ = nullptr;
  vk::raii::DeviceMemory texture_image_memory_ = nullptr;
  vk::raii::ImageView texture_image_view_ = nullptr;
  vk::raii::Sampler texture_sampler_ = nullptr;

  std::vector<vk::raii::Buffer> uniform_buffers_;
  std::vector<vk::raii::DeviceMemory> uniform_buffers_memory_;
  std::vector<void*> uniform_buffers_mapped_;
  vk::raii::DescriptorPool descriptor_pool_ = nullptr;
  std::vector<vk::raii::DescriptorSet> descriptor_sets_;

  vk::raii::CommandPool command_pool_ = nullptr;
  std::vector<vk::raii::CommandBuffer> command_buffers_;

  std::vector<vk::raii::Semaphore> present_complete_semaphores_;
  std::vector<vk::raii::Semaphore> render_finished_semaphores_;
  std::vector<vk::raii::Fence> in_flight_fences_;
  uint32_t frame_index_{0};
  bool framebuffer_resized_ = false;

  static constexpr uint32_t kWidth = 1200;
  static constexpr uint32_t kHeight = 900;

  const std::vector<const char*> kValidationLayers = {
      "VK_LAYER_KHRONOS_validation",
  };

  const std::vector<const char*> kRequiredDeviceExtensions = {
      vk::KHRSwapchainExtensionName,
  };

  void InitWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, FramebufferResizeCallback);
  }

  static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebuffer_resized_ = true;
  }

  void InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain();
    CreateImageViews();
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateCommandPool();
    CreateTextureImage();
    CreateTextureImageView();
    CreateTextureSampler();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
    CreateSyncObjects();
  }

  void MainLoop() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
      DrawFrame();
    }
    device_.waitIdle();
  }

  void Cleanup() {
    CleanupSwapchain();
    glfwDestroyWindow(window_);
    glfwTerminate();
  }

  void CleanupSwapchain() {
    swapchain_image_views_.clear();
    swapchain_ = nullptr;
  }

  void RecreateSwapchain() {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(window_, &width, &height);
      glfwWaitEvents();
    }

    device_.waitIdle();

    CleanupSwapchain();
    CreateSwapchain();
    CreateImageViews();
  }

  // ---------------------------------------------------------------------------: Instance

  void CreateInstance() {
    if (kEnableValidationLayers) {
      CheckValidationLayerSupport();
    }

    constexpr vk::ApplicationInfo app_info{
        .pApplicationName = "Hello Triangle",
        .applicationVersion = vk::makeApiVersion(0, 1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = vk::makeApiVersion(0, 1, 0, 0),
        .apiVersion = vk::ApiVersion14,
    };

    auto required_extensions = GetRequiredExtensions();

    CheckExtensionSupport(required_extensions);

    vk::InstanceCreateInfo create_info{
        .pApplicationInfo = &app_info,
        .enabledLayerCount = kEnableValidationLayers ? static_cast<uint32_t>(kValidationLayers.size()) : 0,
        .ppEnabledLayerNames = kEnableValidationLayers ? kValidationLayers.data() : nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data(),
    };

    instance_ = vk::raii::Instance(context_, create_info);
  }

  void CheckValidationLayerSupport() {
    auto available_layers = context_.enumerateInstanceLayerProperties();

    for (const char* layer_name : kValidationLayers) {
      bool found = std::ranges::any_of(available_layers, [layer_name](const auto& prop) {
        return strcmp(prop.layerName, layer_name) == 0;
      });
      if (!found) {
        throw std::runtime_error("Validation layer not available: " + std::string(layer_name));
      }
    }
  }

  std::vector<const char*> GetRequiredExtensions() {
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (kEnableValidationLayers) {
      extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
  }

  void CheckExtensionSupport(const std::vector<const char*>& required_extensions) {
    auto available_extensions = context_.enumerateInstanceExtensionProperties();

    for (const char* ext : required_extensions) {
      bool found = std::ranges::any_of(available_extensions, [ext](const auto& prop) {
        return strcmp(prop.extensionName, ext) == 0;
      });
      if (!found) {
        throw std::runtime_error("Required extension not supported: " + std::string(ext));
      }
    }
  }

  // ---------------------------------------------------------------------------: Debug Messenger

  void SetupDebugMessenger() {
    if (!kEnableValidationLayers) return;

    vk::DebugUtilsMessengerCreateInfoEXT create_info{
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                         | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                         | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                     | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                     | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = &DebugCallback,
    };

    debug_messenger_ = instance_.createDebugUtilsMessengerEXT(create_info);
  }

  static vk::Bool32 DebugCallback(
      vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
      vk::DebugUtilsMessageTypeFlagsEXT type,
      const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
      void* user_data) {
    if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
      std::cerr << "validation layer: " << callback_data->pMessage << std::endl;
    }
    return vk::False;
  }

  // ---------------------------------------------------------------------------: Physical Device

  void PickPhysicalDevice() {
    auto physical_devices = instance_.enumeratePhysicalDevices();
    auto it = std::ranges::find_if(physical_devices, [&](const auto& device) {
      return IsDeviceSuitable(device);
    });
    if (it == physical_devices.end()) {
      throw std::runtime_error("Failed to find a suitable GPU!");
    }
    physical_device_ = *it;
  }

  bool IsDeviceSuitable(const vk::raii::PhysicalDevice& device) {
    bool supports_vulkan_1_3 = device.getProperties().apiVersion >= vk::ApiVersion13;

    auto queue_families = device.getQueueFamilyProperties();
    bool supports_graphics = std::ranges::any_of(queue_families, [](const auto& qfp) {
      return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
    });

    auto available_extensions = device.enumerateDeviceExtensionProperties();
    bool supports_required_extensions = std::ranges::all_of(kRequiredDeviceExtensions, [&](const char* required) {
      return std::ranges::any_of(available_extensions, [required](const auto& ext) {
        return strcmp(ext.extensionName, required) == 0;
      });
    });

    auto features = device.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    bool supports_required_features =
        features.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
        features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
        features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
        features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

    return supports_vulkan_1_3 && supports_graphics && supports_required_extensions && supports_required_features;
  }

  // ---------------------------------------------------------------------------: Logical Device

  uint32_t FindGraphicsQueueFamily() {
    auto queue_families = physical_device_.getQueueFamilyProperties();
    auto indices = std::views::iota(0u, static_cast<uint32_t>(queue_families.size()));
    auto it = std::ranges::find_if(indices, [&](uint32_t i) {
      return !!(queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics)
          && physical_device_.getSurfaceSupportKHR(i, *surface_);
    });
    assert(it != indices.end() && "No queue family with graphics + present support!");
    return *it;
  }

  void CreateLogicalDevice() {
    graphics_queue_family_ = FindGraphicsQueueFamily();

    float queue_priority = 0.5f;

    vk::DeviceQueueCreateInfo queue_create_info{
        .queueFamilyIndex = graphics_queue_family_,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan11Features,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> feature_chain = {
        {.features = {.samplerAnisotropy = true}},
        {.shaderDrawParameters = true},
        {.synchronization2 = true, .dynamicRendering = true},
        {.extendedDynamicState = true},
    };

    vk::DeviceCreateInfo create_info{
        .pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount = static_cast<uint32_t>(kRequiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = kRequiredDeviceExtensions.data(),
    };


    device_ = vk::raii::Device(physical_device_, create_info);
    graphics_queue_ = vk::raii::Queue(device_, graphics_queue_family_, 0);
  }

  // ---------------------------------------------------------------------------: Surface

  void CreateSurface() {
    VkSurfaceKHR raw_surface;
    if (glfwCreateWindowSurface(*instance_, window_, nullptr, &raw_surface) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create window surface!");
    }
    surface_ = vk::raii::SurfaceKHR(instance_, raw_surface);
  }

  // ---------------------------------------------------------------------------: Swap Chain

  vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats) {
    auto it = std::ranges::find_if(available_formats, [](const auto& f) {
      return f.format == vk::Format::eB8G8R8A8Srgb
          && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    return it != available_formats.end() ? *it : available_formats[0];
  }

  vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_modes) {
    // Prefer mailbox (triple buffering), fall back to FIFO (always available)
    bool has_mailbox = std::ranges::any_of(available_modes, [](auto mode) {
      return mode == vk::PresentModeKHR::eMailbox;
    });
    return has_mailbox ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
  }

  vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    // If currentExtent is not the special value 0xFFFFFFFF, the window manager
    // already decided the size for us — just use it.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    }
    // Otherwise (e.g. high-DPI), query actual framebuffer pixel size from GLFW
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };
  }

  uint32_t ChooseSwapImageCount(const vk::SurfaceCapabilitiesKHR& capabilities) {
    // Request at least 3 images (triple buffering), but respect min/max
    uint32_t count = std::max(3u, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0 && count > capabilities.maxImageCount) {
      count = capabilities.maxImageCount;
    }
    return count;
  }

  void CreateSwapchain() {
    auto capabilities = physical_device_.getSurfaceCapabilitiesKHR(*surface_);
    auto available_formats = physical_device_.getSurfaceFormatsKHR(*surface_);
    auto available_modes = physical_device_.getSurfacePresentModesKHR(*surface_);

    swapchain_format_ = ChooseSwapSurfaceFormat(available_formats);
    auto present_mode = ChooseSwapPresentMode(available_modes);
    swapchain_extent_ = ChooseSwapExtent(capabilities);
    uint32_t image_count = ChooseSwapImageCount(capabilities);

    vk::SwapchainCreateInfoKHR create_info{
        .surface = *surface_,
        .minImageCount = image_count,
        .imageFormat = swapchain_format_.format,
        .imageColorSpace = swapchain_format_.colorSpace,
        .imageExtent = swapchain_extent_,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = present_mode,
        .clipped = true,
    };

    swapchain_ = vk::raii::SwapchainKHR(device_, create_info);
    swapchain_images_ = swapchain_.getImages();
  }

  // ---------------------------------------------------------------------------: Image Views

  void CreateImageViews() {
    swapchain_image_views_.reserve(swapchain_images_.size());
    for (auto image : swapchain_images_) {
      swapchain_image_views_.emplace_back(CreateImageView(image, swapchain_format_.format));
    }
  }

  // ----------------------------------------------------------------------------: Uniforms

  void CreateDescriptorSetLayout() {
    std::array bindings = {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr),
    };
    vk::DescriptorSetLayoutCreateInfo layout_info{
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };
    descriptor_set_layout_ = vk::raii::DescriptorSetLayout(device_, layout_info);
  }

  // ----------------------------------------------------------------------------: Graphics Pipeline

  void CreateGraphicsPipeline() {
    auto shader_module = CreateShaderModule(ReadFile("assets/shaders/slang.spv"));

    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        {.stage = vk::ShaderStageFlagBits::eVertex, .module = shader_module, .pName = "vertMain"},
        {.stage = vk::ShaderStageFlagBits::eFragment, .module = shader_module, .pName = "fragMain"},
    };

    // Vertex input: describe how to read Vertex struct from vertex buffer
    auto binding_description = Vertex::GetBindingDescription();
    auto attribute_descriptions = Vertex::GetAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertex_input_info{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_description,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size()),
        .pVertexAttributeDescriptions = attribute_descriptions.data(),
    };

    // Input assembly: draw triangles from every 3 vertices
    vk::PipelineInputAssemblyStateCreateInfo input_assembly{
        .topology = vk::PrimitiveTopology::eTriangleList,
    };

    // Viewport & scissor: count only, actual values set dynamically at draw time
    vk::PipelineViewportStateCreateInfo viewport_state{
        .viewportCount = 1,
        .scissorCount = 1,
    };

    // Rasterizer: fill triangles, cull back faces
    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .lineWidth = 1.0f,
    };

    // Multisampling: disabled (1 sample per pixel)
    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
    };

    // Color blending: no blending, just write RGBA
    vk::PipelineColorBlendAttachmentState color_blend_attachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                        | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo color_blending{
        .logicOpEnable = vk::False,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    // Dynamic state: viewport and scissor set at draw time
    std::vector dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo dynamic_state{
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    // Pipeline layout: no uniforms or push constants yet
    vk::PipelineLayoutCreateInfo pipeline_layout_info{
        .setLayoutCount = 1,
        .pSetLayouts = &*descriptor_set_layout_,
        .pushConstantRangeCount = 0
    };
    pipeline_layout_ = vk::raii::PipelineLayout(device_, pipeline_layout_info);

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
        // GraphicsPipelineCreateInfo
        {.stageCount = 2,
         .pStages = shader_stages,
         .pVertexInputState = &vertex_input_info,
         .pInputAssemblyState = &input_assembly,
         .pViewportState = &viewport_state,
         .pRasterizationState = &rasterizer,
         .pMultisampleState = &multisampling,
         .pColorBlendState = &color_blending,
         .pDynamicState = &dynamic_state,
         .layout = pipeline_layout_,
         .renderPass = nullptr},
        // PipelineRenderingCreateInfo
        {.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapchain_format_.format}};

    graphics_pipeline_ = vk::raii::Pipeline(device_, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
  }

  // ---------------------------------------------------------------------------: Command Pool & Buffer

  void CreateCommandPool() {
    vk::CommandPoolCreateInfo pool_info{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphics_queue_family_,
    };
    command_pool_ = vk::raii::CommandPool(device_, pool_info);
  }

  void CreateCommandBuffers() {
    command_buffers_.clear();
    vk::CommandBufferAllocateInfo alloc_info{
        .commandPool = command_pool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = kMAX_FRAMES_IN_FLIGHT,
    };
    command_buffers_ = vk::raii::CommandBuffers(device_, alloc_info);
  }

  // ---------------------------------------------------------------------------: Synchronization

  void CreateSyncObjects() {
    assert(present_complete_semaphores_.empty() && render_finished_semaphores_.empty() && in_flight_fences_.empty());

    for (auto _ : std::views::iota(0uz, swapchain_images_.size())) {
      render_finished_semaphores_.emplace_back(device_, vk::SemaphoreCreateInfo());
    }

    for (auto _ : std::views::iota(0uz, kMAX_FRAMES_IN_FLIGHT)) {
      present_complete_semaphores_.emplace_back(device_, vk::SemaphoreCreateInfo());
      in_flight_fences_.emplace_back(device_, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
  }

  // ---------------------------------------------------------------------------: Buffer Helpers

  uint32_t FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties) {
    auto mem_properties = physical_device_.getMemoryProperties();
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
      if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
      }
    }
    throw std::runtime_error("Failed to find suitable memory type!");
  }

  std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(vk::DeviceSize size,
                                                                   vk::BufferUsageFlags usage,
                                                                   vk::MemoryPropertyFlags properties) {
    vk::raii::Buffer buffer(device_, vk::BufferCreateInfo{
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
    });

    auto mem_requirements = buffer.getMemoryRequirements();
    vk::raii::DeviceMemory memory(device_, vk::MemoryAllocateInfo{
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, properties),
    });

    buffer.bindMemory(*memory, 0);
    return {std::move(buffer), std::move(memory)};
  }

  vk::raii::CommandBuffer BeginSingleTimeCommands() {
    vk::raii::CommandBuffer command_buffer = std::move(vk::raii::CommandBuffers(device_, vk::CommandBufferAllocateInfo{
        .commandPool = command_pool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    }).front());

    command_buffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    return command_buffer;
  }

  void EndSingleTimeCommands(vk::raii::CommandBuffer&& command_buffer) {
    command_buffer.end();

    vk::SubmitInfo submit_info{
        .commandBufferCount = 1,
        .pCommandBuffers = &*command_buffer,
    };
    graphics_queue_.submit(submit_info);
    graphics_queue_.waitIdle();
  }

  std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(uint32_t width,
                                                                 uint32_t height,
                                                                 vk::Format format,
                                                                 vk::ImageTiling tiling,
                                                                 vk::ImageUsageFlags usage,
                                                                 vk::MemoryPropertyFlags properties) {
    vk::raii::Image image(device_, vk::ImageCreateInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    });

    auto mem_requirements = image.getMemoryRequirements();
    vk::raii::DeviceMemory memory(device_, vk::MemoryAllocateInfo{
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, properties),
    });

    image.bindMemory(*memory, 0);
    return {std::move(image), std::move(memory)};
  }

  // ---------------------------------------------------------------------------: Texture

  void TransitionImageLayout(vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout) {
    auto command_buffer = BeginSingleTimeCommands();

    vk::AccessFlags src_access_mask{};
    vk::AccessFlags dst_access_mask{};
    vk::PipelineStageFlags src_stage{};
    vk::PipelineStageFlags dst_stage{};

    if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
      src_access_mask = {};
      dst_access_mask = vk::AccessFlagBits::eTransferWrite;
      src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
      dst_stage = vk::PipelineStageFlagBits::eTransfer;
    } else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
      src_access_mask = vk::AccessFlagBits::eTransferWrite;
      dst_access_mask = vk::AccessFlagBits::eShaderRead;
      src_stage = vk::PipelineStageFlagBits::eTransfer;
      dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
      throw std::invalid_argument("Unsupported layout transition!");
    }

    command_buffer.pipelineBarrier(
        src_stage,
        dst_stage,
        {},
        nullptr,
        nullptr,
        vk::ImageMemoryBarrier{
            .srcAccessMask = src_access_mask,
            .dstAccessMask = dst_access_mask,
            .oldLayout = old_layout,
            .newLayout = new_layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        });

    EndSingleTimeCommands(std::move(command_buffer));
  }

  void CopyBufferToImage(vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height) {
    auto command_buffer = BeginSingleTimeCommands();

    command_buffer.copyBufferToImage(
        *buffer,
        *image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::BufferImageCopy{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, 1},
        });

    EndSingleTimeCommands(std::move(command_buffer));
  }

  void CopyBuffer(vk::raii::Buffer& src, vk::raii::Buffer& dst, vk::DeviceSize size) {
    auto command_buffer = BeginSingleTimeCommands();
    command_buffer.copyBuffer(*src, *dst, vk::BufferCopy{.size = size});
    EndSingleTimeCommands(std::move(command_buffer));
  }

  void CreateTextureImage() {
    // CPU-size load image using stb_image lib
    int tex_width = 0;
    int tex_height = 0;
    int tex_channels = 0;
    stbi_uc* pixels = stbi_load("assets/textures/cat.jpg", &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
      throw std::runtime_error("Failed to load texture image!");
    }

    // Staging buffer
    vk::DeviceSize image_size = static_cast<vk::DeviceSize>(tex_width) * tex_height * 4;

    auto [staging_buffer, staging_memory] = CreateBuffer(
        image_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = staging_memory.mapMemory(0, image_size);
    memcpy(data, pixels, static_cast<size_t>(image_size));
    staging_memory.unmapMemory();

    stbi_image_free(pixels);

    // Texture image & layout transition & copy buffer to image
    std::tie(texture_image_, texture_image_memory_) = CreateImage(
        static_cast<uint32_t>(tex_width),
        static_cast<uint32_t>(tex_height),
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    // Undefined --(barrier)--> TransferDstOptimal
    //                            |
    //                            | CopyBufferToImage (staging → image)
    //                            v
    //                        TransferDstOptimal --(barrier)--> ShaderReadOnlyOptimal
    TransitionImageLayout(*texture_image_, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    CopyBufferToImage(staging_buffer, texture_image_, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height));
    TransitionImageLayout(*texture_image_, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
  }

  vk::raii::ImageView CreateImageView(vk::Image image, vk::Format format) {
    vk::ImageViewCreateInfo view_info{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
    };
    return vk::raii::ImageView(device_, view_info);
  }

  void CreateTextureImageView() {
    texture_image_view_ = CreateImageView(*texture_image_, vk::Format::eR8G8B8A8Srgb);
  }

  void CreateTextureSampler() {
    auto properties = physical_device_.getProperties();
    vk::SamplerCreateInfo sampler_info{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = vk::False,
    };
    texture_sampler_ = vk::raii::Sampler(device_, sampler_info);
  }

  // ---------------------------------------------------------------------------: Vertex Buffer

  void CreateVertexBuffer() {
    vk::DeviceSize buffer_size = sizeof(kVertices[0]) * kVertices.size();

    // Staging buffer: CPU-visible, used as transfer source
    auto [staging_buffer, staging_memory] = CreateBuffer(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = staging_memory.mapMemory(0, buffer_size);
    memcpy(data, kVertices.data(), buffer_size);
    staging_memory.unmapMemory();

    // Device-local buffer: GPU-only high-speed memory, used as transfer destination + vertex buffer
    std::tie(vertex_buffer_, vertex_buffer_memory_) = CreateBuffer(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    CopyBuffer(staging_buffer, vertex_buffer_, buffer_size);
  }

  // ---------------------------------------------------------------------------: Index Buffer

  void CreateIndexBuffer() {
    vk::DeviceSize buffer_size = sizeof(kIndices[0]) * kIndices.size();

    auto [staging_buffer, staging_memory] = CreateBuffer(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = staging_memory.mapMemory(0, buffer_size);
    memcpy(data, kIndices.data(), buffer_size);
    staging_memory.unmapMemory();

    std::tie(index_buffer_, index_buffer_memory_) = CreateBuffer(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    CopyBuffer(staging_buffer, index_buffer_, buffer_size);
  }

  // ----------------------------------------------------------------------------: Uniform Buffer

  void CreateUniformBuffers() {
    uniform_buffers_.clear();
    uniform_buffers_memory_.clear();
    uniform_buffers_mapped_.clear();

    for (size_t i = 0; i < kMAX_FRAMES_IN_FLIGHT; i++) {
      vk::DeviceSize buffer_size = sizeof(UniformBufferObject);

      auto [buffer, memory] = CreateBuffer(
          buffer_size,
          vk::BufferUsageFlagBits::eUniformBuffer,
          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

      uniform_buffers_.emplace_back(std::move(buffer));
      uniform_buffers_memory_.emplace_back(std::move(memory));
      uniform_buffers_mapped_.emplace_back(uniform_buffers_memory_[i].mapMemory(0, buffer_size));
    }
  }

  void CreateDescriptorPool() {
    std::array pool_sizes = {
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = static_cast<uint32_t>(kMAX_FRAMES_IN_FLIGHT),
        },
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = static_cast<uint32_t>(kMAX_FRAMES_IN_FLIGHT),
        },
    };
    vk::DescriptorPoolCreateInfo pool_info{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = static_cast<uint32_t>(kMAX_FRAMES_IN_FLIGHT),
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };
    descriptor_pool_ = vk::raii::DescriptorPool(device_, pool_info);
  }

  void CreateDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(kMAX_FRAMES_IN_FLIGHT, *descriptor_set_layout_);
    vk::DescriptorSetAllocateInfo alloc_info{
        .descriptorPool = *descriptor_pool_,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
    };
    descriptor_sets_ = vk::raii::DescriptorSets(device_, alloc_info);

    for (size_t i = 0; i < kMAX_FRAMES_IN_FLIGHT; i++) {
      vk::DescriptorBufferInfo buffer_info{
          .buffer = *uniform_buffers_[i],
          .offset = 0,
          .range = sizeof(UniformBufferObject),
      };
      vk::DescriptorImageInfo image_info{
          .sampler = *texture_sampler_,
          .imageView = *texture_image_view_,
          .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
      };
      std::array descriptor_writes = {
          vk::WriteDescriptorSet{
              .dstSet = *descriptor_sets_[i],
              .dstBinding = 0,
              .dstArrayElement = 0,
              .descriptorCount = 1,
              .descriptorType = vk::DescriptorType::eUniformBuffer,
              .pBufferInfo = &buffer_info,
          },
          vk::WriteDescriptorSet{
              .dstSet = *descriptor_sets_[i],
              .dstBinding = 1,
              .dstArrayElement = 0,
              .descriptorCount = 1,
              .descriptorType = vk::DescriptorType::eCombinedImageSampler,
              .pImageInfo = &image_info,
          },
      };
      device_.updateDescriptorSets(descriptor_writes, nullptr);
    }
  }

  // ----------------------------------------------------------------------------: Updating

  void UpdateUniformBuffers(uint32_t image_index) {
    static auto s_start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - s_start_time).count();

    UniformBufferObject ubo;
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f),
                                static_cast<float>(swapchain_extent_.width) / static_cast<float>(swapchain_extent_.height),
                                0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    memcpy(uniform_buffers_mapped_[image_index], &ubo, sizeof(ubo));
  }

  // ---------------------------------------------------------------------------: Drawing

  void RecordCommandBuffer(uint32_t image_index) {
    command_buffers_[frame_index_].begin({});

    TransitionImageLayout(
        image_index,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput);

    vk::ClearValue clear_color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo color_attachment{
        .imageView = swapchain_image_views_[image_index],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clear_color,
    };
    vk::RenderingInfo rendering_info{
        .renderArea = {.offset = {0, 0}, .extent = swapchain_extent_},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
    };

    command_buffers_[frame_index_].beginRendering(rendering_info);
    command_buffers_[frame_index_].bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline_);
    command_buffers_[frame_index_].bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *pipeline_layout_,
        0,
        *descriptor_sets_[frame_index_],
        nullptr);
    command_buffers_[frame_index_].bindVertexBuffers(0, *vertex_buffer_, {0});
    command_buffers_[frame_index_].bindIndexBuffer(*index_buffer_, 0, vk::IndexType::eUint16);
    command_buffers_[frame_index_].setViewport(0, vk::Viewport(0.0f, 0.0f,
        static_cast<float>(swapchain_extent_.width),
        static_cast<float>(swapchain_extent_.height), 0.0f, 1.0f));
    command_buffers_[frame_index_].setScissor(0, vk::Rect2D({0, 0}, swapchain_extent_));
    command_buffers_[frame_index_].drawIndexed(static_cast<uint32_t>(kIndices.size()), 1, 0, 0, 0);
    command_buffers_[frame_index_].endRendering();

    TransitionImageLayout(
        image_index,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe);

    command_buffers_[frame_index_].end();
  }

  void TransitionImageLayout(
      uint32_t image_index,
      vk::ImageLayout old_layout,
      vk::ImageLayout new_layout,
      vk::AccessFlags2 src_access,
      vk::AccessFlags2 dst_access,
      vk::PipelineStageFlags2 src_stage,
      vk::PipelineStageFlags2 dst_stage) {
    vk::ImageMemoryBarrier2 barrier{
        .srcStageMask = src_stage,
        .srcAccessMask = src_access,
        .dstStageMask = dst_stage,
        .dstAccessMask = dst_access,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain_images_[image_index],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    command_buffers_[frame_index_].pipelineBarrier2(vk::DependencyInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    });
  }

  void DrawFrame() {
    auto fence_result = device_.waitForFences(*in_flight_fences_[frame_index_], vk::True, UINT64_MAX);
    if (fence_result != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to wait for draw fence!");
    }

    uint32_t image_index = 0;
    vk::Result result = vk::Result::eSuccess;
    try {
      std::tie(result, image_index) = swapchain_.acquireNextImage(UINT64_MAX, *present_complete_semaphores_[frame_index_], nullptr);
      if (result == vk::Result::eSuboptimalKHR) {
        framebuffer_resized_ = false;
        RecreateSwapchain();
        return;
      }
      if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to acquire swapchain image!");
      }
    } catch (const vk::OutOfDateKHRError&) {
      RecreateSwapchain();
      return;
    }

    device_.resetFences(*in_flight_fences_[frame_index_]);
    command_buffers_[frame_index_].reset();
    RecordCommandBuffer(image_index);

    UpdateUniformBuffers(frame_index_);

    vk::PipelineStageFlags wait_stage(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submit_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*present_complete_semaphores_[frame_index_],
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &*command_buffers_[frame_index_],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*render_finished_semaphores_[image_index],
    };
    graphics_queue_.submit(submit_info, *in_flight_fences_[frame_index_]);

    vk::PresentInfoKHR present_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*render_finished_semaphores_[image_index],
        .swapchainCount = 1,
        .pSwapchains = &*swapchain_,
        .pImageIndices = &image_index,
    };
    try {
      auto result = graphics_queue_.presentKHR(present_info);
      if (result == vk::Result::eSuboptimalKHR || framebuffer_resized_) {
        framebuffer_resized_ = false;
        RecreateSwapchain();
        return;
      }
      if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to present swapchain image!");
      }
    } catch (const vk::OutOfDateKHRError&) {
      framebuffer_resized_ = false;
      RecreateSwapchain();
      return;
    }

    frame_index_ = (frame_index_ + 1) % kMAX_FRAMES_IN_FLIGHT;
  }

  // ---------------------------------------------------------------------------: Utilities

  [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo create_info{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    vk::raii::ShaderModule shaderModule{device_, create_info};

    return shaderModule;
  }

  static std::vector<char> ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("failed to open file!");
    }
    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();
    return buffer;
  }

};

int main() {
  try {
    HelloTriangleApplication app;
    app.Run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
