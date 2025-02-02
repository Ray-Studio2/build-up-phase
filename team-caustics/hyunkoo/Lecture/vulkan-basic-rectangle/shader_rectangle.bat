glslc -fshader-stage=vertex vertex_input_vs.glsl -o ../../x64/Debug/vertex_input_vs.spv
glslc -fshader-stage=vertex vertex_input_vs.glsl -o ../../x64/Release/vertex_input_vs.spv

glslc -fshader-stage=fragment vertex_input_fs.glsl -o ../../x64/Debug/vertex_input_fs.spv
glslc -fshader-stage=fragment vertex_input_fs.glsl -o ../../x64/Release/vertex_input_fs.spv