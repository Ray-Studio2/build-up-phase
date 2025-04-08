#!/bin/sh

set -e
cd "$(dirname "$0")"


if command -v cmd > /dev/null; then
    PWD="$(cmd //c cd | sed 's|\\|\\\\\\\\|g' | sed s/\"/\\\"/g)"
else
    PWD="$(pwd | sed 's|\\|\\\\\\\\|g' | sed s/\"/\\\"/g)"
fi

if command -v cmd > /dev/null; then
    INCLUDE_DIR="$VULKAN_SDK\\Include"
else
    INCLUDE_DIR="$VULKAN_SDK/Include"
fi

echo '[
  {
    "directory": "[[WORKSPACE]]",
    "file": "src/lib.cpp",
    "output": "/dev/null",
    "arguments": [
      "clang",
      "src/lib.cpp",
      "-o",
      "/dev/null",
      "-I",
      "submodules/glfw/include",
      "-I",
      "'"$(echo "$INCLUDE_DIR"  | sed 's|\\|\\\\\\\\|g' | sed s/\"/\\\"/g)"'",
      "-std=c++17",
      "-c"
    ]
  }
]
' | sed "s\\[\\[WORKSPACE\\]\\]$PWDg" > compile_commands.json
