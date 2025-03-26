cmake_minimum_required(VERSION 3.30)
project(Seohyuk)

set(CMAKE_CXX_STANDARD 20)




#[==[

본 파일은 모던CMake 와 최대한 피해야할 방법들을 피해가며 만들어낸 CMakeList.txt입니다.

다음과 같은 과정으로 제너레이터를 만들어냅니다. 명령줄에서 cmake -G를 통해 다양한 제너레이터를 사용할 수 있습니다.

0. Ninja
1. Xcode
2. Visual Studio 20**
4  Unix Make

이 프로젝트는 다음과 같은 순서로 작업을 수행합니다.

0. 간단한 OS 검사
1. executable을 추가
2. 시스템 사양에 따른 환경 변수와 디렉터리 세팅
3. OS무관한 링크 과정 수행

현재 테스트된 시스템은 다음과 같습니다

{ 제너레이터: Xcode,  IDE: Clion }


]==]


#
# 실행 파일을 추가할 때는 add_executable 을 사용
#
# 외부 프로젝트가 추가되는 장소. 리파지토리 링크로만 사용
#
# set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/external") 이제부터 sub_repo를 사용
#

message("Detected System type: ${CMAKE_SYSTEM_NAME}")

if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
    message("Preparing for windows")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
    message("For MacOS")
else ()
    message("Not supported OS")
endif ()

# 외부 라이브러리 링크하기!
add_subdirectory(sub_repo)


# 여기서부터는 executable 추가
add_executable(CMDUtility CMDUtility.cpp)
add_executable(Triangle triangle.cpp
        include/seohyuk.h
        include/vulkan_utility.h)
add_subdirectory(TeapotBasic)

#[==[ 여러 줄 주석은 이렇게 합니다!
add_executable( teapot_OBJ_with_normal_rendering teapot_OBJ_with_normal_rendering.cpp )
]==]

#[==[
이하 테스트 유닛.
]==]



# 여기에 execuable을 등록만 하면 자동으로 링크된다!
set(    EXES
        #[==[============ 이하 executable의 목록 ============]==]
        CMDUtility
        Triangle
        TeapotBasic
)

message(STATUS "Prepareing project for the Executables ${EXES}")

foreach (EXE IN LISTS EXES)
    set(    INCLUDES
            "${CMAKE_CURRENT_SOURCE_DIR}/sub_repo/glfw/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    set(    LIB_DIR
            "${CMAKE_SOURCE_DIR}/sub_repo/glfw/CMake/modules"
    )

    set(    LIBS
            glfw3
    )
    
    target_sources(${EXE} PUBLIC "./include/vk_nutshell.h" "./include/vulkan_utility.h")

    if (${CMAKE_SYSTEM_NAME} STREQUAL Darwin) # macOS

        set(LIBRARIES ${LIBRARIES} vulkan.1 glfw3 CoreFoundation)
        set(LIB_DIR ${LIB_DIR} "/Users/$ENV{USER}/VulkanSDK/1.4.309.0/macOS/lib")
        set(INCLUDES ${INCLUDES} "/Users/$ENV{USER}/VulkanSDK/1.4.309.0/macOS/include")

    elseif (${CMAKE_SYSTEM_NAME} STREQUAL Windows) #Windows

        set(VULKAN_INCLUDE "$ENV{VULKAN_SDK}/Include")
        set(VULKAN_LIB "$ENV{VULKAN_SDK}/Lib")
        set(VULKAN_LIBRARIES vulkan-1)

    else () #Unknown
        message("Not supported OS")
    endif ()

    # global linking process

    target_include_directories(${EXE} PUBLIC ${INCLUDES})
    target_link_libraries( ${EXE} PUBLIC ${LIBRARIES} )
    target_link_directories( ${EXE} PUBLIC ${LIB_DIR} "${CMAKE_BINARY_DIR}/sub_repo/glfw/src/Debug" )

endforeach ()