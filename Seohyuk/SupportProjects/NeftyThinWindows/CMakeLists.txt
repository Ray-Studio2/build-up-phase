
#[==[
it's time to ditch GLFW or STL or else for more debuging support, platform support, light programing.
EXTREMELY Simple cross platform gui tool for vulkan.

only supports stadard FHD ~ 8K displays and single main screen.

mouse and keyboard is needed. joysticks or any touchscreen is not supported.

using native gui in Win32, Cocoa, X11
]==]

cmake_minimum_required(VERSION 3.30)

project(simple_platform C)

set(${CMAKE_C_STANDARD} 23)

add_subdirectory(src)
