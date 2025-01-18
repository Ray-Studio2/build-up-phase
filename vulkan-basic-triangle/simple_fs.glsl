#version 450

layout(location = 0) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

float oneOvertwo = 1.0 / 2.0;

void main() {
    vec3 color = inNormal + vec3(1, 1, 1);
    color *= oneOvertwo;
    outColor = vec4(color, 1.0);
}
