<<<<<<< HEAD
cmake_minimum_required(VERSION 3.30.0)
=======
cmake_minimum_required(VERSION 3.10)

>>>>>>> 181fa97c47d4811c8d711648c18d3879dc58d842
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
    message("For macOS")
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

add_executable(TeapotBasic TeapotBasic/TeapotBasic.cpp include/vk_nutshell.h)

#[==[ 여러 줄 주석은 이렇게 합니다!
add_executable( teapot_OBJ_with_normal_rendering teapot_OBJ_with_normal_rendering.cpp )
]==]

#[==[
이하 테스트 유닛.
]==]



# 여기에 execuable을 등록만 하면 자동으로 링크된다!
set(    EXES
        #[==[============ 이하 executable의 목록 ============]==]
        #CMDUtility
        #Triangle
        TeapotBasic
)

message(STATUS "Prepareing project for the Executables ${EXES}")

set(    DEF_INCLUDES
        "${CMAKE_CURRENT_SOURCE_DIR}/sub_repo/glfw/include"
        "include"
)
set(    DEF_LIB_DIR
        "${CMAKE_BINARY_DIR}/sub_repo/glfw/src"
        "${CMAKE_BINARY_DIR}/sub_repo/glfw/include"
)
set(    DEF_LIBS
         # 앞에 lib가 붙어도 gcc와 llvm은 자동으로 생략합니다.
)

foreach (EXE IN LISTS EXES)
    if (${CMAKE_SYSTEM_NAME} STREQUAL Darwin) # macOS
        

        set(INCLUDES ${DEF_INCLUDES} "/Users/$ENV{USER}/VulkanSDK/1.4.309.0/macOS/include")

        set(LIB_DIRS
                "/Users/$ENV{USER}/VulkanSDK/1.4.309.0/macOS/lib"
        )

        list(APPEND LIB_DIRS )

        if(${CMAKE_GENERATOR} STREQUAL "Xcode")
            list(APPEND LIB_DIRS "${DEF_LIB_DIR}/Debug")
        else ()
            list(APPEND LIB_DIRS "${DEF_LIB_DIR}")
        endif ()

        list(APPEND DEF_LIBS MoltenVK glfw) # MoltenVK

        set(LIBS ${DEF_LIBS} # 이런 경우에 list를 set대신 쓸 수 있습니다. https://cmake.org/cmake/help/latest/command/list.html
                "-framework Cocoa"
                "-framework IOKit"
                "-framework IOSurface"
                "-framework CoreFoundation"
                "-framework QuartzCore"
                "-framework Metal"
                #[==[
                glfw 의 설정을 크게 건들지 않아 임베딩이 되지 않습니다. 이 경우 프레임워크를 최종 컴파일 시점에서 구현체를 정해줘야합니다.

                glfw는 다음 라이브러리들이 필수적으로 요구됩니다.

                코코아: 시간 측정과 모니터 조이스틱 등을 컨트롤 합니다
                IOKit: 네이티브하게 윈도우를 만들 때 사용됩니다
                CoreFoundation: 애플 플렛폼의 프로그래밍 전반에 사용됩니다
                QuartzCore: 다양한 유틸리티와 최종 윈도우 컨트롤에 필요합니다

                라이브러리 목록을 보고싶을 경우 Xcode에서 프로젝트를 수정하는 창에서 라이브러리 추가 화면을 통해 볼 수 있습니다.
                ]==]
        )



    elseif (${CMAKE_SYSTEM_NAME} STREQUAL Windows) #Windows

        set(INCLUDES  ${DEF_INCLUDES} "$ENV{VK_SDK_PATH}/Include")
        list(APPEND LIB_DIRS ${DEF_LIB_DIR} "$ENV{VK_SDK_PATH}/Lib" )
        set(LIBS ${DEF_LIBS} vulkan-1 glfw3)
        #[==[
        add_custom_command(TARGET ${EXE}
                COMMAND cp
                "${CMAKE_CURRENT_BINARY_DIR}/sub_repo/glfw/src/Debug/glfw3.dll"
                "${CMAKE_CURRENT_BINARY_DIR}/Debug/"
                POST_BUILD
        )
        ]==]

    else () #Unknown
        message("Not supported OS")
    endif ()

    # global linking process

    target_include_directories(${EXE} PUBLIC ${INCLUDES})
    target_link_directories( ${EXE} PUBLIC ${LIB_DIRS} )
    target_link_libraries( ${EXE} PUBLIC ${LIBS} )






endforeach ()