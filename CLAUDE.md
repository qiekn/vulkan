# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Vulkan 学习项目，使用 C++23 + libc++，基于 MSYS2 UCRT64 + Clang 工具链（Windows）。

目前在学习 这个文档 https://docs.vulkan.org/tutorial/latest/

## Build Commands

CMake 配置（首次或 CMakeLists.txt 变更后执行）：

```bash
"C:/msys64/ucrt64/bin/cmake.exe" -B build -G Ninja \
  -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/clang.exe" \
  -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/clang++.exe"
```

编译并运行：

```bash
"C:/msys64/ucrt64/bin/cmake.exe" --build build -j$(nproc) && ./build/vulkan
```

也可用 `./run.sh` 快捷构建运行，`./run.sh debug` 进入 GDB 调试。

## Architecture

- **构建系统**: CMake (Ninja)，C++23 标准 + Modules + libc++，生成 `compile_commands.json` 供 clangd 使用
- **可执行目标**: `vulkan`，源码通过 `GLOB_RECURSE` 收集 `src/*.cpp`（普通源码）和 `src/*.cppm`（module 文件）
- **依赖管理**: Git submodule（所有第三方库统一使用 submodule）

| 依赖 | 类型 | 位置 | 用途 |
|------|------|------|------|
| Vulkan 1.4 | find_package (系统 SDK) | `C:/VulkanSDK/1.4.341.1` | 图形 API |
| GLFW 3.4 | submodule | `deps/glfw/` | 窗口、输入、Vulkan surface |
| GLM | submodule | `deps/glm/` | 数学库 (向量/矩阵/四元数) |
| tinyobjloader | submodule | `deps/tinyobjloader/` | OBJ 模型加载 |

## Code Style

- Google C++ Style，通过 `.clang-format` 和 `.clang-tidy` 强制执行
- 列宽限制 120 字符，缩进 2 空格
- 类成员变量后缀 `_`（如 `member_`），命名空间/变量 `lower_case`，类/结构体 `CamelCase`
- 常量/枚举值使用 `k` 前缀 + `CamelCase`（如 `kMaxSize`）
- `.clang-tidy` 警告不提升为错误

## C++23 Modules

项目使用 C++23 Modules 替代传统头文件：

- Module 接口文件使用 `.cppm` 后缀，放在 `src/` 目录
- 普通源文件（`.cpp`）通过 `import` 使用 module
- 使用 `import std;` 导入标准库，不再需要 `#include` STL 头文件
- 需要 `#include` 非标准库头文件（如 Vulkan、GLFW）时，放在 module 文件顶部的 `global module fragment`（`module;` 和 `export module xxx;` 之间）
- 必须使用 Ninja 生成器（MinGW Makefiles 不支持 modules）
