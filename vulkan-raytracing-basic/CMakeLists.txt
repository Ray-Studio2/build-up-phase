cmake_minimum_required(VERSION 3.10)

project(hello_triangle)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})
set(GLFW_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/glfw-3.4.bin.WIN64/include)
set(GLFW_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/glfw-3.4.bin.WIN64/lib-vc2022)

add_executable(${PROJECT_NAME} main.cpp)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
    set(GLSLANG_LIB_NAMES 
        glslang 
        glslang-default-resource-limits 
        SPIRV-Tools 
        SPIRV-Tools-opt)
else()
    set(GLSLANG_LIB_NAMES 
        glslangd 
        glslang-default-resource-limitsd 
        SPIRV-Toolsd 
        SPIRV-Tools-optd)
endif()


include_directories(
    ${GLFW_INCLUDE_DIR}
    $ENV{VULKAN_SDK}/Include
)


find_library(GLFW_LIB glfw3 PATHS ${GLFW_LIBRARY_DIR})
find_library(VULKAN_LIB vulkan-1 PATHS $ENV{VULKAN_SDK}/Lib)

set(GLSLANG_LIBS "")
foreach(LIB_NAME ${GLSLANG_LIB_NAMES})
    find_library(LIB_${LIB_NAME} ${LIB_NAME} PATHS $ENV{VULKAN_SDK}/Lib)
    if(LIB_${LIB_NAME})
        list(APPEND GLSLANG_LIBS ${LIB_${LIB_NAME}})
    else()
        message(FATAL_ERROR "Failed to find library: ${LIB_NAME}")
    endif()
endforeach()

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GLFW_LIB}
    ${VULKAN_LIB}
    ${GLSLANG_LIBS}
)