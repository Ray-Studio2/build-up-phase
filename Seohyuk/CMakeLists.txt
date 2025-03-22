cmake_minimum_required(VERSION 3.30)
project(Seohyuk)

set(CMAKE_CXX_STANDARD 20)

#
# 실해파일을 추가할 때는 add_executable을 사용합니다.
#


add_executable(Seohyuk main.cpp) # Hello, world 출력
set(VULKAN_INCLUDE "$ENV{VULKAN_SDK}/Include")
set(VULKAN_LIB "$ENV{VULKAN_SDK}/Lib")
set(GLFW3_INCLUDE "$ENV{CMAKE_CURRENT_SOURCE_DIR}/external/GLFW3/glfw-3.4/include") #GLFW include 폴더
set(GLFW3_LIB "$ENV{CMAKE_CURRENT_SOURCE_DIR}/external/GLFW3/glfw-3.4/lib") #GLFW 폴더의 .lib 파일과 dll 파일

# 이 아래에서 작업 시작

add_executable(Triangle triangle.cpp) # 삼각형만 출력
target_include_directories( Triangle PUBLIC ${VULKAN_INCLUDE} ${GLFW3_INCLUDE} )
target_link_directories( Triangle PUBLIC ${VULKAN_DIR} ${GLFW3_INCLUDE} )
