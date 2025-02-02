#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

//layout(binding = 1) uniform bool isNormalMode;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 FragPos;
layout(location = 1) out vec3 Normal;
//layout(location = 2) out bool isNormalMode;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    //gl_Position = vec4(inPosition, 1.0);

    FragPos = vec3(ubo.model * vec4(inPosition, 1.0));
    Normal = mat3(transpose(inverse(ubo.model))) * inNormal;
}

//#version 450
//
//layout(location = 0) in vec2 inPosition;
//layout(location = 1) in vec3 inColor;
//
//layout(location = 0) out vec3 fragColor;
//
//layout(set = 0, binding = 0) uniform UBO {
//    float dx;
//    float dy;
//} g;
//
//void main() {
//    gl_Position = vec4(inPosition.x + g.dx, inPosition.y + g.dy, 0.0, 1.0);
//    fragColor = inColor;
//}