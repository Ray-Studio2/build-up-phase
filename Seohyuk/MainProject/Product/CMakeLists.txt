add_executable(TeapotBasic TeapotBasic/TeapotBasic.cpp)
add_executable(Triangle Triangle/triangle.cpp)

set(EXES
        Triangle
        TeapotBasic
)

foreach (EXE ${EXES})

    target_include_directories(${EXE} PUBLIC ${INCLUDES})
    target_link_directories(${EXE} PUBLIC ${LIB_DIRS})
    target_link_libraries(${EXE} PUBLIC ${LIBS})

endforeach ()