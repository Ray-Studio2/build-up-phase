#!/bin/sh

set -e
cd "$(dirname "$0")"

sh glfw.sh


mkdir -p builddirs/out
cmake -B builddirs/vulkan-raytraced-triangle -DCMAKE_INSTALL_PREFIX=builddirs/out/vulkan-raytraced-triangle vulkan-raytraced-triangle
cmake --build builddirs/vulkan-raytraced-triangle --config Debug
cmake --install builddirs/vulkan-raytraced-triangle --config Debug

builddirs/out/vulkan-raytraced-triangle/bin/hello_triangle
