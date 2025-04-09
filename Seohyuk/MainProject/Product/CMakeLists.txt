add_executable(TeapotBasic TeapotBasic/TeapotBasic.cpp)
add_executable(Triangle Triangle/triangle.cpp)

ADD_CUSTOM_TARGET(EXES
        Triangle
        TeapotBasic
)

add_dependencies(EXES SETUPS)

foreach (EXE ${EXES})
    target_include_directories(${EXE} PUBLIC ${INCLUDES} ${GLOBAL_INCLUDE})
    target_link_directories(${EXE} PUBLIC ${LIB_DIRS} ${GLOBAL_LINK_DIR})
    target_link_libraries(${EXE} PUBLIC ${LIBS} ${GLOBAL_LINK_LIBS})

endforeach ()