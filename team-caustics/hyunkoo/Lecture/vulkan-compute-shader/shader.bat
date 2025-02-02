glslc -fshader-stage=vertex simple_vs.glsl -o ../../x64/Debug/simple_vs.spv
glslc -fshader-stage=vertex simple_vs.glsl -o ../../x64/Release/simple_vs.spv

glslc -fshader-stage=fragment simple_fs.glsl -o ../../x64/Debug/simple_fs.spv
glslc -fshader-stage=fragment simple_fs.glsl -o ../../x64/Release/simple_fs.spv