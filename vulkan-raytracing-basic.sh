#!/bin/sh

set -e
cd "$(dirname "$0")"

sh glfw.sh


mkdir -p builddirs/out
cmake -B builddirs/vulkan-raytracing-basic -DCMAKE_INSTALL_PREFIX=builddirs/out/vulkan-raytracing-basic vulkan-raytracing-basic
cmake --build builddirs/vulkan-raytracing-basic --config Debug
cmake --install builddirs/vulkan-raytracing-basic --config Debug

builddirs/out/vulkan-raytracing-basic/bin/hello_triangle
