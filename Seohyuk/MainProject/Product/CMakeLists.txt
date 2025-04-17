add_executable(TeapotBasic TeapotBasic/TeapotBasic.cpp
)
add_executable(Triangle Triangle/triangle.cpp)

set(EXES TeapotBasic Triangle)

foreach (EXE IN ITEMS ${EXES})
    add_dependencies(${EXE} SETUPS)

    target_include_directories(${EXE} PUBLIC
            ${VULKAN_INCLUDE}
            #${SUPPORT_PROJECTS_INCLUDE}
            ${EXTERNAL_PROJECTS_INCLUDE}
            ${CMAKE_SOURCE_DIR}/include
    )
    target_link_directories(${EXE} PUBLIC ${LIB_DIRS})
    target_link_libraries(${EXE} PUBLIC ${LIBS})

endforeach ()