### Project structure definition ###
cmake_minimum_required(VERSION 3.30)
project(RayStudioPhase_1)

###### This directories will connect every thing together. ######
file(MAKE_DIRECTORY include lib bin)

###### Functions ######
#TODO Copy compiled result to global directory

###### External project or submodules ######
add_subdirectory(ExternalProjects)


##### Support project for main project ######
# Support project should be independent to other any project exception of external elements.
# Subprojects contain with it's own project, must be can be open as a single project.
add_subdirectory(SupportProjects)


###### Installing files to source directory ######




###### Variables ######
set(GLOBAL_INCLUDE_DIR include)
set(GLOBAL_LINK_DIR lib)
set(GLOBAL_LINK_LIBS)




### Project structure definition ended ###


### CrossPlatform compilation
if(APPLE)
    ###### This is why we recommend to install vulkan sdk globally. ######
    file(GLOB VK_DIR "$ENV{HOME}/VulkanSDK/1.*/macOS")
    set(VULKAN_SDK_DIR VK_DIR)
    list(APPEND GLOBAL_LINK_LIBS vulkan)
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


add_custom_target(SETUPS)
add_dependencies(SETUPS EXTERNAL_OBJECTS SUPPORT_OBJECTS)

add_subdirectory(MainProject)

