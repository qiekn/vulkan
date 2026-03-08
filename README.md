# Vulkan learning notes

跟随 [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/) 学习 Vulkan。

## 环境

- **平台**: Windows (MSYS2 UCRT64)
- **编译器**: Clang 21 (C++20 Modules)
- **构建**: CMake + Ninja
- **Vulkan SDK**: 1.4.341.1

## 依赖

| 库 | 用途 |
|----|------|
| [GLFW](https://github.com/glfw/glfw) | 窗口管理 |
| [GLM](https://github.com/g-truc/glm) | 数学库 |
| [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) | OBJ 模型加载 |

## 构建

```bash
cmake -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++

cmake --build build
./build/main
```
