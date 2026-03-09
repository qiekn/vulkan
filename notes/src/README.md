# Vulkan Learning Notes

Personal notes from following the [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/).

## Environment

- **Platform**: Windows (MSYS2 UCRT64)
- **Compiler**: Clang 21 (C++23 Modules & libc++)
- **Build system**: CMake 3.30+ / Ninja
- **Vulkan SDK**: 1.4.341.1 (via `find_package(Vulkan)`, using Vulkan-Hpp C++ Module)

## Dependencies

All third-party libraries are managed as git submodules under `deps/`.

| Library | Purpose |
|---------|---------|
| [GLFW](https://github.com/glfw/glfw) | Window & input, Vulkan surface |
| [GLM](https://github.com/g-truc/glm) | Math (vectors, matrices, quaternions) |
| [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) | OBJ model loading |

## Quick Start

Download and install [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) first, then:

```bash
git clone --recurse-submodules https://github.com/qiekn/vulkan
cd vulkan

cmake -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++

cmake --build build
./build/vulkan
```
