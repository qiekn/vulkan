# Dev Environment

## Vulkan SDK

Download and install [Vulkan SDK 1.4+](https://vulkan.lunarg.com/sdk/home). Make sure the `VULKAN_SDK` environment variable is set — CMake uses `find_package(Vulkan)` to locate it.

## Toolchain (MSYS2 UCRT64)

Install [MSYS2](https://www.msys2.org/), then in the UCRT64 terminal:

```bash
pacman -S mingw-w64-ucrt-x86_64-clang \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja \
          mingw-w64-ucrt-x86_64-clang-tools-extra
```

## Clone & Build

```bash
git clone --recurse-submodules <repo-url>
cd vulkan

# if already cloned without submodules:
git submodule update --init --recursive
```

```bash
cmake -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
```

This generates `compile_commands.json` for clangd.

```bash
cmake --build build -j$(nproc)
./build/vulkan
```

Or use the shortcut:

```bash
./run.sh          # build + run
./run.sh debug    # build + gdb
```

## Editor

Any editor with clangd support works (VS Code, Neovim, etc.).
The build generates `build/compile_commands.json` automatically.

Formatting and linting are enforced by `.clang-format` and `.clang-tidy`.
