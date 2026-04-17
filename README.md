# Vulkan learning notes

跟随 [Khronos Vulkan® Tutorial](https://docs.vulkan.org/tutorial/latest/) 学习 Vulkan。

- [个人 Notion 随笔](https://qiekn.notion.site/vulkan)
- [在线 MdBook 笔记](https://qiekn.github.io/vulkan)（主要根据 Claude Code 对话的上下文总结）
- [Bilibili 全过程视频记录](https://www.bilibili.com/video/BV1zhPUzcEtb)


## 开发环境

- **平台**: Windows (MSYS2 UCRT64)
- **编译器**: Clang 21 (C++23 Modules & LLVM libc++)
- **构建**: CMake 3.30+ / Ninja
- **Vulkan SDK**: 1.4.341.1

## 三方库依赖

| 第三方库 (已通过 Git Submodule 引入)                                 | 用途         |
| ----                                                            | ------       |
| [GLFW](https://github.com/glfw/glfw)                            | 窗口管理     |
| [GLM](https://github.com/g-truc/glm)                            | 数学库       |
| [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) | OBJ 模型加载 |

## 构建运行

```bash
git clone --recursive https://github.com/qiekn/vulkan.git
```

下载安装 [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)，确保 `VULKAN_SDK` 环境变量已设置。

CMake 中自定义命令使用了 `slangc.exe`，需要把 VulkanSDK 的 Bin 添加到环境变量中，保证 slangc.exe，在终端下可用。可以参考下面我的 MSYS2 ZSH 配置

```bash
export PATH=$PATH:"/c/VulkanSDK/1.4.341.1/Bin"
```

然后可以按照下面使用 CMake 编译运行，或者直接运行 `./run.sh`

```bash
cmake -B build -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++

cmake --build build
cd build
./vulkan
```

## 额外说明

### cmake import std

关于 `import std;`: 这是 CMake 的实验性功能，不同 CMake 版本需要在 `CMakeLists.txt` 中指定对应的 UUID，不过我已经添加了 cmake 脚本自动设置 UUID，见 [cmake/EnableCxxImportStd.cmake](./cmake/EnableCxxImportStd.cmake) 。

所以这里你并不需要做什么。

### libc++

我 CMake 里指定了 `CMAKE_CXX_FLAGS` ，使用了 LLVM 的标准库实现，即 `-stdlib=libc++`。这其实没什么道理，只是我已经用了 clang，平时很多时候用 macOS，另外考虑到只有它让我在 c++23 中使用 `std::println`，gnu / msvc 不让用。所有就干脆用了 `libc++`，如果你没有安装 `libc++` 而 CMake 编译报错，可以去掉下面一行。我代码中并没有使用 `std::println`

```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
```

### slangc

Shader 的编译用到 `slangc`，我在 CMakeList.txt 中设置了自定义命令来编译 slang shader，需要保证 slangc 在环境变量中。

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
