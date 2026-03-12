# Vulkan learning notes

跟随 [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/) 学习 Vulkan。

## 环境

- **平台**: Windows (MSYS2 UCRT64)
- **编译器**: Clang 21 (C++23 Modules & libc++)
- **构建**: CMake 3.30+ / Ninja（`import std;` 需要实验性支持，不同 CMake 版本需更新 `CMakeLists.txt` 中的 UUID）
- **Vulkan SDK**: 1.4.341.1（通过 `find_package(Vulkan)` 查找，使用 Vulkan-Hpp C++ Module）

## 依赖

| 库 | 用途 |
|----|------|
| [GLFW](https://github.com/glfw/glfw) | 窗口管理 |
| [GLM](https://github.com/g-truc/glm) | 数学库 |
| [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) | OBJ 模型加载 |

## 构建

需根据本机环境修改 `CMakeLists.txt` 中的 `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` UUID（不同 CMake 版本不同）。

需自行下载安装 [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)，确保 `VULKAN_SDK` 环境变量已设置。

把 VulkanSDK 的 Bin 添加到环境变量中，目前主要是使用了 slangc.exe，下面是我的 MSYS2 配置

```bash
export PATH=$PATH:"/c/VulkanSDK/1.4.341.1/Bin"
```

```bash
cmake -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++

cmake --build build
./build/vulkan
```
