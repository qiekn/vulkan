#include <GLFW/glfw3.h>
#include <cstdlib>

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

  static constexpr uint32_t kWidth = 1200;
  static constexpr uint32_t kHeight = 900;

private:
  const std::vector<const char*> kValidationLayers = {
      "VK_LAYER_KHRONOS_validation",
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
