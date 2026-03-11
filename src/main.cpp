#include <cassert>
#include <cstdlib>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

import vulkan;
import std;

#ifdef NDEBUG
constexpr bool kEnableValidationLayers = false;
#else
constexpr bool kEnableValidationLayers = true;
#endif

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
  vk::raii::SurfaceKHR surface_ = nullptr;
  vk::raii::SwapchainKHR swapchain_ = nullptr;
  std::vector<vk::Image> swapchain_images_;
  vk::SurfaceFormatKHR swapchain_format_;
  vk::Extent2D swapchain_extent_;
  std::vector<vk::raii::ImageView> swapchain_image_views_;

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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
  }

  void InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain();
    CreateImageViews();
  }

  void MainLoop() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
    }
  }

  void Cleanup() {
    glfwDestroyWindow(window_);
    glfwTerminate();
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
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    bool supports_required_features =
        features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
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
    uint32_t graphics_family = FindGraphicsQueueFamily();

    float queue_priority = 0.5f;

    vk::DeviceQueueCreateInfo queue_create_info{
        .queueFamilyIndex = graphics_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> feature_chain = {
        {},
        {.dynamicRendering = true},
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
    graphics_queue_ = vk::raii::Queue(device_, graphics_family, 0);
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
    vk::ImageViewCreateInfo create_info{
        .viewType = vk::ImageViewType::e2D,
        .format = swapchain_format_.format,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
    };
    for (auto image : swapchain_images_) {
      create_info.image = image;
      swapchain_image_views_.emplace_back(device_, create_info);
    }
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
