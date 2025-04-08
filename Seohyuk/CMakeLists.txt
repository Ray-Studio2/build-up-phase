cmake_minimum_required(VERSION 3.30)

project(RayStudioPhase_1)

set(VULKAN_SDK_DIR)



if(APPLE)
    file(GLOB VK_DIR "$ENV{HOME}/VulkanSDK/1.*/macOS")

    set(VULKAN_SDK_DIR VK_DIR)
elseif(BSD)
# FreeBSD, NetBSD, OpenBSD, etc.
elseif(LINUX)
# Linux
elseif(UNIX)
# Linux, BSD, etc. (including macOS if not trapped above)
elseif(WIN32)
# Windows
else()
message(FATAL_ERROR "Unsupported system: ${CMAKE_SYSTEM_NAME}")
endif()

set(GLOBAL_INCLUDE
        "${VK_DIR}/include"
        "${CMAKE_SOURCE_DIR}/SupportProjects/VkNeftyRenderTools/include"
        "${CMAKE_SOURCE_DIR}/SupportProjects/VkExtraThinWindows/include"
)

add_subdirectory(MainProject)
add_subdirectory(SupportProjects/NeftyThinWindows)
add_subdirectory(SupportProjects/VkNeftyRenderTools)

