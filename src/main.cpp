#include <GLFW/glfw3.h>
#include <cstdlib>

import vulkan;
import std;

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

  static constexpr uint32_t kWidth = 1200;
  static constexpr uint32_t kHeight = 900;

  void InitWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
  }

  void InitVulkan() { CreateInstance(); }

  void MainLoop() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
    }
  }

  void Cleanup() {
    glfwDestroyWindow(window_);
    glfwTerminate();
  }

  void CreateInstance() {
    constexpr vk::ApplicationInfo app_info{
        .pApplicationName = "Hello Triangle",
        .applicationVersion = vk::makeApiVersion(0, 1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = vk::makeApiVersion(0, 1, 0, 0),
        .apiVersion = vk::ApiVersion14};

    uint32_t glfw_extension_count = 0;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    auto extension_properties = context_.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < glfw_extension_count; ++i) {
      if (std::ranges::none_of(extension_properties,
                               [ext = glfw_extensions[i]](const auto& prop) {
                                 return strcmp(prop.extensionName, ext) == 0;
                               })) {
        throw std::runtime_error("Required GLFW extension not supported: " +
                                 std::string(glfw_extensions[i]));
      }
    }

    vk::InstanceCreateInfo create_info{.pApplicationInfo = &app_info,
                                       .enabledExtensionCount = glfw_extension_count,
                                       .ppEnabledExtensionNames = glfw_extensions};
    instance_ = vk::raii::Instance(context_, create_info);
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
