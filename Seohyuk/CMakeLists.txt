cmake_minimum_required(VERSION 3.30)
project(Seohyuk)

set(CMAKE_CXX_STANDARD 20)

#
# 실행 파일을 추가할 때는 add_executable 을 사용합니다.
#

message("Detected System type: ${CMAKE_SYSTEM_NAME}")

if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
    message("Preparing for windows")

    set(VULKAN_INCLUDE "$ENV{VULKAN_SDK}/Include")
    set(VULKAN_LIB "$ENV{VULKAN_SDK}/Lib")
    set(VULKAN_LIBRARIES vulkan-1

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

set( GLFW_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/external/include" ) # 외부 인클루드
set( GLFW_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/lib" ) # 외부 라이브러리
set( INCLUDES ${VULKAN_INCLUDE} ${GLFW_INCLUDE} ${EXTRA_INCLUDE} "${CMAKE_SOURCE_DIR}/include")
set( LIB_DIR ${VULKAN_LIB} ${GLFW_LIB_DIR} )
set( LIBS ${VULKAN_LIBRARIES} glfw3 )

# 이 아래에서 작업 시작

add_executable(CMDUtility CMDUtility.cpp
        include/vulkan_utility.h) # 커멘드라인 도구
target_include_directories( CMDUtility PUBLIC ${INCLUDES})
target_link_directories( CMDUtility PUBLIC ${LIB_DIR} )
target_link_libraries(CMDUtility PUBLIC ${LIBS} )

add_executable(Triangle triangle.cpp
        include/seohyuk.h
        include/vulkan_utility.h) # 삼각형만 출력
target_include_directories( Triangle PUBLIC ${INCLUDES} seohyuk.h )
target_link_directories( Triangle PUBLIC ${LIB_DIR} )
target_link_libraries(Triangle PUBLIC ${LIBS} )


#[==[ 여러 줄 주석은 이렇게 합니다!

add_executable( teapot_OBJ_with_normal_rendering teapot_OBJ_with_normal_rendering.cpp )
target_include_directories( teapot_OBJ_with_normal_rendering PUBLIC ${VULKAN_INCLUDE} ${GLFW_INCLUDE} ${EXTRA_INCLUDE})
target_link_directories( teapot_OBJ_with_normal_rendering PUBLIC ${VULKAN_LIB} ${GLFW_LIB_DIR} )
target_link_libraries(teapot_OBJ_with_normal_rendering PUBLIC ${VULKAN_LIBRARIES} glfw3)

]==]

add_subdirectory(TeapotBasic)

target_include_directories( TeapotBasic PUBLIC ${INCLUDES} )
target_link_directories( TeapotBasic PUBLIC ${LIB_DIR} )
target_link_libraries(TeapotBasic PUBLIC ${LIBS} )
target_sources(TeapotBasic PUBLIC include/vk_nutshell.h include/vulkan_utility.h)


#[==[
이하 테스트 유닛.
]==]

add_executable(TestVulkanUtility test/TestVulkanUtility.cpp include/vulkan_utility.h)
target_include_directories( TestVulkanUtility PUBLIC ${INCLUDES} )
target_link_directories( TestVulkanUtility PUBLIC ${LIB_DIR} )
target_link_libraries(TestVulkanUtility LINK_PUBLIC ${LIBS} )

#[==[
add_executable(TestVectorTools test/TestVectorTools.cpp include/vector_tools.h)
target_include_directories( TestVectorTools PUBLIC ${INCLUDES} )
target_link_directories( TestVectorTools PUBLIC ${LIB_DIR} )
target_link_libraries(TestVectorTools PUBLIC ${LIBS} )
]==]
