glslc -fshader-stage=vertex uniform_buffer_vs.glsl -o ../../x64/Debug/uniform_buffer_vs.spv
glslc -fshader-stage=vertex uniform_buffer_vs.glsl -o ../../x64/Release/uniform_buffer_vs.spv

glslc -fshader-stage=fragment uniform_buffer_fs.glsl -o ../../x64/Debug/uniform_buffer_fs.spv
glslc -fshader-stage=fragment uniform_buffer_fs.glsl -o ../../x64/Release/uniform_buffer_fs.spv