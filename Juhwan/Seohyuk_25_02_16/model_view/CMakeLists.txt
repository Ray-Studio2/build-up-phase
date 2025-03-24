cmake_minimum_required(VERSION 3.29)
project(model_view)


#LLVM is strongly recommended!!!


set(CMAKE_CXX_STANDARD 17)

add_executable(model_view main.cpp
        Renderer.hpp)

target_include_directories(model_view PUBLIC $ENV{VULKAN_SDK}/Include)
target_link_directories(model_view PUBLIC $ENV{VULKAN_SDK}/Lib)
target_link_libraries(model_view PUBLIC vulkan-1.lib)

target_sources(
        model_view PUBLIC
        Renderer.cpp
        Utility.hpp
)
