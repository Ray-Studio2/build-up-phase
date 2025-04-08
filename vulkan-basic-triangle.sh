#!/bin/sh

set -e
cd "$(dirname "$0")"

sh glfw.sh


mkdir -p builddirs/out
cmake -B builddirs/vulkan-basic-triangle -DCMAKE_INSTALL_PREFIX=builddirs/out/vulkan-basic-triangle vulkan-basic-triangle
cmake --build builddirs/vulkan-basic-triangle --config Debug
cmake --install builddirs/vulkan-basic-triangle --config Debug

(cd vulkan-basic-triangle && ../builddirs/out/vulkan-basic-triangle/bin/hello_triangle)
