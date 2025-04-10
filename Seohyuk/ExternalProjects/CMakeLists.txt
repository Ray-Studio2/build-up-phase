add_custom_target(EXTERNAL_OBJECTS)

macro(registerProject A B)
    foreach (item ${ARGV})
        add_subdirectory(${item})

        set_target_properties( ${item}
                PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib"
                ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib"
                RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
        )

        add_dependencies(EXTERNAL_OBJECTS ${item})
    endforeach ()
endmacro()


registerProject(glfw tinyobjloader)
