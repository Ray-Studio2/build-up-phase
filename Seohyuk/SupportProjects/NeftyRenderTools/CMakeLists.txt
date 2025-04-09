cmake_minimum_required(VERSION 3.30)

project(SupporterNeftyRenderTools CXX)

set(CMAKE_CXX_STANDARD 20)

set(NEFTY_RENDER_TOOLS "include/vk_nutshell.h" "include/vulkan_utility.h")
set(VULKAN)

add_library(NeftyRenderTools NeftyRenderTools.cpp)
add_executable(NeftyRenderToolsTest Test/TestCompilable.cpp ${NEFTY_RENDER_TOOLS})

if(APPLE)
    ###### This is why we recommend to install vulkan sdk globally. ######
    file(GLOB VULKAN "$ENV{HOME}//VulkanSDK/1.*/macOS")
    set(LIBS glfw vulkan "-framework Cocoa" "-framework IOKit" "-framework QuartzCore" "-framework IOSurface" )
elseif (WIN32)
    set(VULKAN $ENV{VK_SDK_PATH})
    set(LIBS glfw vulkan-1)
endif ()



target_include_directories(NeftyRenderTools PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
        "${VULKAN}/include"
        ${EXTERNAL_PROJECTS_INCLUDE}
)
target_link_directories(NeftyRenderTools PUBLIC
        "${VULKAN}/lib"
        "${CMAKE_BINARY_DIR}/ExternalProjects/glfw/src/debug")
target_link_libraries(NeftyRenderTools ${LIBS})




target_include_directories(NeftyRenderToolsTest PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
        "${VULKAN}/include"
        ${EXTERNAL_PROJECTS_INCLUDE}
)
target_link_directories(NeftyRenderToolsTest PUBLIC
        "${VULKAN}/lib"
        "${CMAKE_BINARY_DIR}/ExternalProjects/glfw/src/debug"
)
target_link_libraries(NeftyRenderToolsTest ${LIBS} NeftyRenderTools)


