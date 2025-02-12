#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform UBO {
    float dx;
    float dy;
} g;

void main() {
    gl_Position = vec4(inPosition.x + g.dx, inPosition.y + g.dy, 0.0, 1.0);
    fragColor = inColor;
}