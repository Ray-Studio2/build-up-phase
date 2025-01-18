#version 450

//layout(location = 0) in dvec3 inPosition;
layout(location = 2) in vec3 inNromal;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(inNromal, 1.0);
}
