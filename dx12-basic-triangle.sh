#!/bin/sh

set -e
cd "$(dirname "$0")"


mkdir -p builddirs/out
cmake -B builddirs/dx12-basic-triangle -DCMAKE_INSTALL_PREFIX=builddirs/out/dx12-basic-triangle dx12-basic-triangle
cmake --build builddirs/dx12-basic-triangle --config Debug
cmake --install builddirs/dx12-basic-triangle --config Debug

builddirs/out/dx12-basic-triangle/bin/demo
