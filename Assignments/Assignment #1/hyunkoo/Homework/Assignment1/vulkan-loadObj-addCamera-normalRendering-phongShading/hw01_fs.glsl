#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform MVP_Projecttion {
    mat4 model;
    mat4 view;
    mat4 proj;
    int isNormalPresent;
    vec3 padding;  // 16바이트 정렬 맞추기
} ubo;

const vec3 lightPos = vec3(10.0, 10.0, 10.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const vec3 objectColor = vec3(0.5, 0.5, 0.5);

void main() {
    if (ubo.isNormalPresent == 1) {
        // 월드 노멀 렌더링 (색상으로 노멀을 직접 표현)
        vec3 normalColor = normalize(fragNormal) * 0.5 + 0.5;
        outColor = vec4(normalColor, 1.0);
    }
    else {
        // Phong Shading 적용
        vec3 norm = normalize(fragNormal);
        vec3 lightDir = normalize(lightPos - fragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor * objectColor;

        vec3 viewDir = normalize(-fragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = spec * lightColor;

        outColor = vec4(diffuse + specular, 1.0);
    }
}
