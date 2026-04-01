cmake_minimum_required(VERSION 3.30)

# import std: must be set before project() for toolchain detection
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

include(cmake/EnableCxxImportStd.cmake)
project(vulkan LANGUAGES CXX)

set(CMAKE_CXX_MODULE_STD ON)

# compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------: Config
set(SHADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets/shaders)

# -----------------------------------------------------------------------------: Dependences
find_package(Vulkan REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/deps/glfw)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/glm)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/tinyobjloader)

# -----------------------------------------------------------------------------: Vulkan C++ Module
add_library(VulkanCppModule)
add_library(Vulkan::cppm ALIAS VulkanCppModule)

target_compile_definitions(VulkanCppModule PUBLIC
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
  VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
  VULKAN_HPP_CXX_MODULE_EXPERIMENTAL_WARNING=1
)

target_include_directories(VulkanCppModule PRIVATE "${Vulkan_INCLUDE_DIRS}")
target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)

target_sources(VulkanCppModule
  PUBLIC
  FILE_SET cxx_modules TYPE CXX_MODULES
  BASE_DIRS "${Vulkan_INCLUDE_DIRS}"
  FILES "${Vulkan_INCLUDE_DIRS}/vulkan/vulkan.cppm"
        "${Vulkan_INCLUDE_DIRS}/vulkan/vulkan_video.cppm"
)

# -----------------------------------------------------------------------------: Slang
function (add_slang_shader_target TARGET)
  cmake_parse_arguments ("SHADER" "" "" "SOURCES" ${ARGN})

  set (SHADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/assets/shaders)
  file(MAKE_DIRECTORY ${SHADERS_OUT_DIR})

  set (ENTRY_POINTS -entry vertMain -entry fragMain)
  add_custom_command (
          OUTPUT ${SHADERS_DIR}
          COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
  )
  add_custom_command (
          OUTPUT  ${SHADERS_DIR}/slang.spv
          COMMAND slangc ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o slang.spv
          WORKING_DIRECTORY ${SHADERS_DIR}
          DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
          COMMENT "Compiling Slang Shaders"
          VERBATIM
  )
  add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()

# -----------------------------------------------------------------------------: Targets
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE MODULE_FILES CONFIGURE_DEPENDS "src/*.cppm")
file(GLOB_RECURSE SHADER_SLANG_SOURCES CONFIGURE_DEPENDS "assets/shaders/*.slang")

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})
target_sources(${PROJECT_NAME} PRIVATE FILE_SET CXX_MODULES FILES ${MODULE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE
  Vulkan::cppm
  glfw
  glm
  tinyobjloader
)

add_slang_shader_target(shader SOURCES ${SHADER_SLANG_SOURCES})
add_dependencies(${PROJECT_NAME} shader)
