#!/bin/sh

set -e
cd "$(dirname "$0")"

git submodule update --init

cmake -DCMAKE_INSTALL_PREFIX=builddirs/out/glfw -B builddirs/glfw submodules/glfw
cmake --build builddirs/glfw --config Release
cmake --install builddirs/glfw --config Release
