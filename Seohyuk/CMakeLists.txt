cmake_minimum_required(VERSION 3.30)
project(Seohyuk)

set(CMAKE_CXX_STANDARD 20)

#
# 실행 파일을 추가할 때는 add_executable 을 사용합니다.
#


add_executable(Seohyuk main.cpp
        seokyuk.h) # Hello, world 출력

if (${CMAKE_SYSTEM_NAME} STREQUAL WIN32)
    message(For Win32)

    set(VULKAN_INCLUDE "$ENV{VULKAN_SDK}/Include")
    set(VULKAN_LIB "$ENV{VULKAN_SDK}/Lib")

    set(VULKAN_LIBRARIES vulkan-1
            glslangd
            glslang-default-resource-limitsd
            SPIRV-Toolsd
            SPIRV-Tools-optd
    )

elseif(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
    message("For MacOS")

    set(VULKAN_INCLUDE "/Users/$ENV{USER}/VulkanSDK/1.4.309.0/macOS/include")
    set(VULKAN_LIB "/Users/$ENV{USER}/VulkanSDK/1.4.309.0/macOS/lib")

    set(VULKAN_LIBRARIES vulkan.1)
else ()
    message("Not supported OS")
endif ()



#만약 무언가를 출력하고싶다면 이렇게! message(STATUS "${VULKAN_LIB}")
#message(STATUS "${VULKAN_LIB}")
#message(STATUS "$ENV{USER}")

set(GLFW_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/external/include") #GLFW include 폴더
set(GLFW_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external") #GLFW 폴더의 .lib 파일과 dll 파일
set(EXTRA_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/external")


# 이 아래에서 작업 시작

add_executable(Triangle triangle.cpp
        seokyuk.h) # 삼각형만 출력
target_include_directories( Triangle PUBLIC ${VULKAN_INCLUDE} ${GLFW_INCLUDE} ${EXTRA_INCLUDE})
target_link_directories( Triangle PUBLIC ${VULKAN_LIB} ${GLFW_LIB_DIR} )
target_link_libraries(Triangle PUBLIC ${VULKAN_LIBRARIES} glfw3)


add_executable( teapot_OBJ_with_normal_rendering teapot_OBJ_with_normal_rendering.cpp )
target_include_directories( teapot_OBJ_with_normal_rendering PUBLIC ${VULKAN_INCLUDE} ${GLFW_INCLUDE} ${EXTRA_INCLUDE})
target_link_directories( teapot_OBJ_with_normal_rendering PUBLIC ${VULKAN_LIB} ${GLFW_LIB_DIR} )
target_link_libraries(teapot_OBJ_with_normal_rendering PUBLIC ${VULKAN_LIBRARIES} glfw3)

