cmake_minimum_required(VERSION 3.28)

project(main)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------: Dependences
set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.341.1")
set(Vulkan_INCLUDE_DIR "${VULKAN_SDK_PATH}/Include")
set(Vulkan_LIBRARY "${VULKAN_SDK_PATH}/Lib/vulkan-1.lib")
find_package(Vulkan REQUIRED)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/glfw)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/glm)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/tinyobjloader)

# -----------------------------------------------------------------------------: Targets
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE MODULE_FILES CONFIGURE_DEPENDS "src/*.cppm")

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})
target_sources(${PROJECT_NAME} PRIVATE FILE_SET CXX_MODULES FILES ${MODULE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE
  Vulkan::Vulkan
  glfw
  glm
  tinyobjloader
)
