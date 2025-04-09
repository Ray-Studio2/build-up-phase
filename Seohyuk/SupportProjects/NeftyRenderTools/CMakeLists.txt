cmake_minimum_required(VERSION 3.30)

project(SupporterNeftyRenderTools CXX)

set(NEFTY_RENDER_TOOLS "include/vk_nutshell.h" "include/vulkan_utility.h")

add_executable(NeftyRenderToolsTest Test/TestCompilable.cpp ${NEFTY_RENDER_TOOLS})

if(APPLE)
    ###### This is why we recommend to install vulkan sdk globally. ######
    file(GLOB VK_DIR "$ENV{HOME}/VulkanSDK/1.*/macOS")
    set(LIBS glfw3 vulkan "-framework Cocoa" "-framework IOKit" "-framework QuartzCore" "-framework IOSurface" )
else ()
    set(LIBS glfw3 vulkan)
endif ()

target_include_directories(NeftyRenderToolsTest PUBLIC "${PROJECT_SOURCE_DIR}/include" "${VK_DIR}/include")
target_link_directories(NeftyRenderToolsTest PUBLIC "${VK_DIR}/lib" "${CMAKE_BINARY_DIR}/ExternalProjects/glfw/src/debug")
target_link_libraries(NeftyRenderToolsTest ${LIBS})

