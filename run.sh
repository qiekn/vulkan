#!/bin/bash

BUILD="build"
DEBUGGER="gdb"
TARGET="vulkan"

# 1. Configure (first time or CMakeLists.txt changed)
if [ ! -d "${BUILD}" ]; then
  echo "Configuring project..."
  cmake -B ${BUILD} -G Ninja \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ && echo "" || exit 1
fi

# 2. Build
echo "Building project..."
cmake --build ${BUILD} -j$(nproc) || exit 1

# 3. cd build (assets/shaders are relative to build/)
cd ${BUILD} || exit 1

# 4. Run or Debug
if [ "$1" = "debug" ]; then
  ${DEBUGGER} ./${TARGET}
else
  ./${TARGET}
fi

# vim: ft=sh ts=2 sw=2 et
