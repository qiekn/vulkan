#!/bin/bash

BUILD="build"
DEBUGGER="gdb"
TARGET="vulkan"

# 1. Compile
echo "Building project..."
cmake --build ${BUILD} -j$(nproc)

# 2. Check Compile
if [ $? -ne 0 ]; then
  echo "Build failed! Aborting."
  exit 1
fi

# 3. cd build
cd ${BUILD} || exit 1

# 4. Run or Debug
if [ "$1" = "debug" ]; then
  ${DEBUGGER} ./${TARGET}
else
  ./${TARGET}
fi

# vim: ft=sh ts=2 sw=2 et
