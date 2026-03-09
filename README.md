# Vulkan learning notes

跟随 [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/) 学习 Vulkan。

## 环境

- **平台**: Windows (MSYS2 UCRT64)
- **编译器**: Clang 21 (C++23 Modules + libc++)
- **构建**: CMake 4.2+ / Ninja（`import std;` 需要实验性支持，不同 CMake 版本需更新 `CMakeLists.txt` 中的 UUID）
- **Vulkan SDK**: 1.4.341.1

## 依赖

| 库 | 用途 |
|----|------|
| [GLFW](https://github.com/glfw/glfw) | 窗口管理 |
| [GLM](https://github.com/g-truc/glm) | 数学库 |
| [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) | OBJ 模型加载 |

## 构建

需根据本机环境修改 `CMakeLists.txt` 中的以下路径：

- `VULKAN_SDK_PATH` — Vulkan SDK 安装路径（默认 `C:/VulkanSDK/1.4.341.1`）
- `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` — 不同 CMake 版本的 UUID 不同

```bash
cmake -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++

cmake --build build
./build/vulkan
```
