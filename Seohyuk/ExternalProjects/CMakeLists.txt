add_custom_target(EXTERNAL_OBJECTS)

macro(registerProject A B)
    foreach (item ${ARGV})
        add_subdirectory(${item})
        add_dependencies(EXTERNAL_OBJECTS ${item})
    endforeach ()
endmacro()



registerProject(glfw tinyobjloader)
