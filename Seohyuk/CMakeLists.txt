### Project structure definition ###
cmake_minimum_required(VERSION 3.30)
project(RayStudioPhase_1)

###### External project or submodules ######
add_subdirectory(ExternalProjects)
###### Variables ######
set(EXTERNAL_PROJECTS_INCLUDE
        "${CMAKE_SOURCE_DIR}/ExternalProjects/glfw/include"
        "${CMAKE_SOURCE_DIR}/ExternalProjects/tinyobjloader"
)
set(VULKAN_SDK_DIR)
### Project structure definition ended ###


### CrossPlatform compilation
if(APPLE)
    ###### This is why we recommend to install vulkan sdk globally. ######
    file(GLOB VK_DIR "$ENV{HOME}/VulkanSDK/1.*/macOS")
    set(VULKAN_SDK_DIR VK_DIR)
    list(APPEND glfw3 vulkan)
elseif(BSD)
# FreeBSD, NetBSD, OpenBSD, etc.
elseif(LINUX)
# Linux
elseif(UNIX)
# Linux, BSD, etc. (including macOS if not trapped above)
elseif(WIN32)
    set(VULKAN_SDK_DIR $ENV{VK_SDK_PATH})
else()
    ###### Unsupported system means it's not a Windows, POSIX or the OS we knows. ######
    message(FATAL_ERROR "Unsupported system: ${CMAKE_SYSTEM_NAME}")
endif()

if (NOT ${VULKAN_SDK_DIR} )
    message(FATAL_ERROR "Unsupported system: ${CMAKE_SYSTEM_NAME}")
endif ()

##### Support project for main project ######
# Support project should be independent to other any project exception of external elements.
# Subprojects contain with it's own project, must be can be open as a single project.
# add_subdirectory(SupportProjects)

#[==[
set(SUPPORT_PROJECTS_INCLUDE
        "${PROJECT_SOURCE_DIR}/SupportProjects/NeftyRenderTools/include"
        #SupportProjects/NeftyThinWindows
)
]==]


add_custom_target(SETUPS)
add_dependencies(SETUPS EXTERNAL_OBJECTS #[==[SUPPORT_OBJECTS]==])

add_subdirectory(MainProject)

