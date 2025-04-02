
#[==[
it's time to ditch GLFW or STL or else for more debuging support, platform support, light programing.
EXTREMELY Simple cross platform gui tool for vulkan.

only supports stadard FHD ~ 8K displays and single main screen.

mouse and keyboard is needed. joysticks or any touchscreen is not supported.

using native gui in Win32, Cocoa, X11


]==]

cmake_minimum_required(VERSION 3.30)

project(simple_platform C)

set(${CMAKE_C_STANDARD} 17)


add_library(SimplePlatform platform.c platform.h)
target_include_directories(SimplePlatform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    target_compile_definitions(SimplePlatform PUBLIC "_Cocoa")
    #enable_language(OBJC)
    target_sources(SimplePlatform PUBLIC
            MacOSPlatform/CocoaPlatform.h
            MacOSPlatform/CocoaPlatform.m
    )
endif ()


add_executable(Test test/test.c platform.c)
target_link_directories(Test PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(Test SimplePlatform)
