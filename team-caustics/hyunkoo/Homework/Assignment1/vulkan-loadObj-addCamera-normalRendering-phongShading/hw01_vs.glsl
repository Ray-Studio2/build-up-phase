#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragNormal;  // vec4 -> vec3로 수정
layout(location = 1) out vec3 fragPos;

layout(set = 0, binding = 0) uniform MVP_Projecttion {
    mat4 model;
    mat4 view;
    mat4 proj;
    int isNormalPresent;
    vec3 padding;  // 16바이트 정렬 맞추기
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragPos = vec3(ubo.model * vec4(inPosition, 1.0));
    fragNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
}
