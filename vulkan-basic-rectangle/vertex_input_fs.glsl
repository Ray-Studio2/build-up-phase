#version 450

#pragma shader_stage(fragment)

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    vec2 foo;
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    vec3 color;
    if(ubo.foo.x > 0)
    {
        color = fragColor;
    }
    else
    {
        float dotValue = dot(fragColor, vec3(0, 1, 0));
        color = vec3(dotValue);
    }

    outColor = vec4(color, 1.0);
}
