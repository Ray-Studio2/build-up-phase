#pragma once
#include <vector>
#include <iostream>
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>

#pragma comment(lib, "glslang.lib")
#pragma comment(lib, "glslang-default-resource-limits.lib")
#pragma comment(lib, "SPIRV-Tools.lib")
#pragma comment(lib, "SPIRV-Tools-opt.lib")


std::vector<uint32_t> glsl2spv(glslang_stage_t stage, const char* shaderSource) {
    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_2,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_5,
        .code = shaderSource,
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslang_default_resource(),
    };

    glslang_shader_t* shader = glslang_shader_create(&input);

    if (!glslang_shader_preprocess(shader, &input)) {
        printf("GLSL preprocessing failed (%d)\n", stage);
        printf("%s\n", glslang_shader_get_info_log(shader));
        printf("%s\n", glslang_shader_get_info_debug_log(shader));
        printf("%s\n", input.code);
        glslang_shader_delete(shader);
        return {};
    }

    if (!glslang_shader_parse(shader, &input)) {
        printf("GLSL parsing failed (%d)\n", stage);
        printf("%s\n", glslang_shader_get_info_log(shader));
        printf("%s\n", glslang_shader_get_info_debug_log(shader));
        printf("%s\n", glslang_shader_get_preprocessed_code(shader));
        glslang_shader_delete(shader);
        return {};
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        printf("GLSL linking failed (%d)\n", stage);
        printf("%s\n", glslang_program_get_info_log(program));
        printf("%s\n", glslang_program_get_info_debug_log(program));
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        return {};
    }

    glslang_program_SPIRV_generate(program, stage);

    size_t size = glslang_program_SPIRV_get_size(program);
    std::vector<uint32_t> spvBirary(size);
    glslang_program_SPIRV_get(program, spvBirary.data());

    const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
    if (spirv_messages)
        printf("(%d) %s\b", stage, spirv_messages);

    glslang_program_delete(program);
    glslang_shader_delete(shader);

    return spvBirary;
}




#include <vulkan/vulkan_core.h>
VkShaderModule example(VkDevice device)
{
    const char* vs_src = R"(
        #version 450
        layout(location = 0) in vec3 inPosition;
        void main() {
            gl_Position = vec4(inPosition, 1.0);s
        }
    )";

    auto vs_spv = glsl2spv(GLSLANG_STAGE_VERTEX, vs_src);

    VkShaderModuleCreateInfo vsModuleInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = vs_spv.size(),
        .pCode = vs_spv.data(),
    };

    VkShaderModule vsModule;
    if (vkCreateShaderModule(device, &vsModuleInfo, nullptr, &vsModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return vsModule;
}