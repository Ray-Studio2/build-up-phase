# glfw: 3.4.0
# vulkan: 1.4.304.0

cmake_minimum_required(VERSION 3.11)

set(MINGW_PATH "C:/Program Files/MinGW-w64/bin")
set(VULKAN_PATH "C:/VulkanSDK/1.4.304.0")

set(CMAKE_CXX_COMPILER ${MINGW_PATH}/g++.exe)
set(CMAKE_MAKE_PROGRAM ${MINGW_PATH}/mingw32-make.exe)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

project(vulkan_rt)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/includes
    ${VULKAN_PATH}/Include
)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp)

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${SRC_FILES}
)
target_compile_definitions(
    ${PROJECT_NAME}

    PRIVATE
        DEBUG
)

target_link_libraries(
    ${PROJECT_NAME}

    PRIVATE
        ${VULKAN_PATH}/Lib/vulkan-1.lib
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/binaries/glfw/libglfw3dll.a
)