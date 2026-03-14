# Vulkan learning notes

跟随 [Vulkan Tutorial](https://docs.vulkan.org/tutorial/latest/) 学习 Vulkan。

- [个人 Notion 随笔](https://qiekn.notion.site/vulkan)
- [在线 MdBook 笔记](https://qiekn.github.io/vulkan)（主要根据 Claude Code 对话的上下文总结）

## 开发环境

- **平台**: Windows (MSYS2 UCRT64)
- **编译器**: Clang 21 (C++23 Modules & LLVM libc++)
- **构建**: CMake 3.30+ / Ninja（CMake `import std;` 需要实验性支持，不同 CMake 版本需更新 `CMakeLists.txt` 中的 UUID）
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

需根据本机环境修改 `CMakeLists.txt` 中的 `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` UUID（不同 CMake 版本不同）。

需自行下载安装 [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)，确保 `VULKAN_SDK` 环境变量已设置。

CMake 中自定义命令使用了 `slangc.exe`，需要把 VulkanSDK 的 Bin 添加到环境变量中，保证 slangc.exe，在终端下可用。可以参考下面我的 MSYS2 ZSH 配置

```bash
export PATH=$PATH:"/c/VulkanSDK/1.4.341.1/Bin"
```

然后可以按照下面使用 CMake 编译运行，或者直接运行 `./run.sh`

```bash
cmake -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++

cmake --build build
cd build
./vulkan
```

注意，`assets/shaders/shader.slang` 被编译为 bytecode 后放在了 `build/assets/shaders/shader.spv`，所以我们需要再 `build` 目录下执行 `./vulkan.exe`，在项目根目录下 `./build/vulkan.exe` 会由于找不到 `shader.spv` 而报错。

