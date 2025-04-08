### Project structure definition ###
cmake_minimum_required(VERSION 3.30)
project(RayStudioPhase_1)

###### External project or submodules ######
set(VULKAN_SDK_DIR)
add_subdirectory(GLFW/glfw)

##### Support project for main project ######
# Support project should be independent to other any project exception of external elements.
# Subprojects contain with it's own project, must be can be open as a single project.
add_subdirectory(SupportProjects)

###### Variables ######
set(GLOBAL_INCLUDE_DIR
        ${NEFTY_RENDER_TOOLS}
)
set(GLOBAL_LINK_DIR)
set(GLOBAL_LINK_LIBS)

message(WARNING ${NEFTY_RENDER_TOOLS})

### Project structure definition ended ###


### CrossPlatform compilation
if(APPLE)
    ###### This is why we recommend to install vulkan sdk globally. ######
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
    ###### Unsupported system means it's not a Windows, POSIX or the OS we knows. ######
    message(FATAL_ERROR "Unsupported system: ${CMAKE_SYSTEM_NAME}")
endif()



add_executable(GlobalMainProgram GlobalMainProgram.cpp GlobalMainProgram.h)



#[==[
set(GLOBAL_INCLUDE
        "${VK_DIR}/include\;"
        "${CMAKE_SOURCE_DIR}/SupportProjects/VkNeftyRenderTools/include\;"
        "${CMAKE_SOURCE_DIR}/SupportProjects/VkExtraThinWindows/include\;"
)


add_subdirectory(SupportProjects)
message(WARNING ${GLOBAL_INCLUDE_DIR})
add_subdirectory(MainProject)

]==]