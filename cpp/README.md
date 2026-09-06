# Vulkan learning notes

跟随 [Khronos Vulkan® Tutorial](https://docs.vulkan.org/tutorial/latest/) 学习 Vulkan。

- [TEASER.md (Screenshots & GIFs)](./TEASER.md)
- [Notion 笔记](https://qiekn.notion.site/vulkan)
- [Github Page 笔记](https://qiekn.github.io/vulkan)（AI SLOP）
- [糟糕的 Bilibili 视频](https://www.bilibili.com/video/BV1zhPUzcEtb)


## 开发环境

- Windows (MSYS2 UCRT64)
- Clang 21 (C++23 Modules & LLVM libc++)
- CMake 3.30+ / Ninja
- Vulkan SDK 1.4.341.1

## 库依赖

| 第三方库                                                        | 用途         |
| ----                                                            | ------       |
| [GLFW](https://github.com/glfw/glfw)                            | 窗口         |
| [GLM](https://github.com/g-truc/glm)                            | 数学库       |
| [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) | OBJ 模型加载 |
| [stb](https://github.com/nothings/stb) (`stb_image.h`)          | 图像加载     |

## 构建运行

下载安装 [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)，确保 `VULKAN_SDK` 环境变量已设置。

CMake 中自定义命令使用了 `slangc.exe`，需要把 VulkanSDK 的 Bin 添加到环境变量中，保证 slangc.exe，在终端下可用。

```bash
export PATH=$PATH:"/c/VulkanSDK/1.4.341.1/Bin"
```

然后可以使用下列命令编译运行

```bash
cmake -B build -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++

cmake --build build
cd build
./vulkan
```

## 额外说明

### cmake import std

关于 `import std;`: 这是 CMake 的实验性功能，不同 CMake 版本需要在 `CMakeLists.txt` 中指定对应的 UUID，我们添加了 cmake 脚本自动设置 UUID，见 [cmake/EnableCxxImportStd.cmake](./cmake/EnableCxxImportStd.cmake) 。

### libc++

CMake 里指定了 `CMAKE_CXX_FLAGS` ，使用了 LLVM 的标准库实现，即 `-stdlib=libc++`。

```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
```

### slangc

Shader 的编译用到 `slangc`，CMakeList.txt 中设置了自定义命令来编译 slang shader，要求 slangc 在环境变量中。

```bash
export PATH=$PATH:"/c/VulkanSDK/1.4.341.1/Bin"
```

```bash
$ which slangc
/c/VulkanSDK/1.4.341.1/Bin/slangc
```
### assets folder

CMake 会自动同步 `assets/` 到 `build/assets/`，并将 `assets/shaders/shader.slang` 编译到 `build/assets/shaders/slang.spv`

```
cd build
./vulkan.exe
```
