#pragma once
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
#include <vulkan/vulkan_core.h>


std::vector<uint32_t> glsl2spv(glslang_stage_t stage, const char* shaderSource) {
    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_3,
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


template <VkShaderStageFlagBits>
struct glslang_stage_for;

#define GLSLANG_STAGE_MAPPING(vkStage, glslangStage) \
template <> \
struct glslang_stage_for<vkStage> { \
    static constexpr glslang_stage_t value = glslangStage; \
};

GLSLANG_STAGE_MAPPING(VK_SHADER_STAGE_VERTEX_BIT, GLSLANG_STAGE_VERTEX);
GLSLANG_STAGE_MAPPING(VK_SHADER_STAGE_FRAGMENT_BIT, GLSLANG_STAGE_FRAGMENT);
GLSLANG_STAGE_MAPPING(VK_SHADER_STAGE_COMPUTE_BIT, GLSLANG_STAGE_COMPUTE);
GLSLANG_STAGE_MAPPING(VK_SHADER_STAGE_RAYGEN_BIT_KHR, GLSLANG_STAGE_RAYGEN);
GLSLANG_STAGE_MAPPING(VK_SHADER_STAGE_ANY_HIT_BIT_KHR, GLSLANG_STAGE_ANYHIT);
GLSLANG_STAGE_MAPPING(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, GLSLANG_STAGE_CLOSESTHIT);
GLSLANG_STAGE_MAPPING(VK_SHADER_STAGE_MISS_BIT_KHR, GLSLANG_STAGE_MISS);


template <VkShaderStageFlagBits VkStage>
struct ShaderModule {
private:
    VkShaderModule module;
    VkDevice device;

    static std::vector<char> readFile(const std::string& filename, bool binary=false) {
        std::ifstream file(filename, std::ios::ate | (binary ? std::ios::binary : 0));
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize + (binary? 0 : 1));
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), fileSize);
        file.close();
        if(!binary)
            buffer[fileSize] = 0;
        return buffer;
    }

    void build(const std::vector<uint32_t>& spv_blob) {
        VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = spv_blob.size() * 4,
            .pCode = spv_blob.data(),
        };

        if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
    }

public:
    VkShaderModule get() {
        return module;
    }
    
    /* By default, glslang appears to only support "main" as the entry point name.
    VkPipelineShaderStageCreateInfo operator|(const char* entry) {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VkStage,
            .module = module,
            .pName = entry,
        };
    }*/

    operator VkPipelineShaderStageCreateInfo() {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VkStage,
            .module = module,
            .pName = "main",
        };
    }

    ShaderModule(VkDevice device, const std::vector<uint32_t>& spv_blob) :device(device) {
        build(spv_blob);
    }

    ShaderModule(VkDevice device, const char* code) :device(device) {
        build(glsl2spv(glslang_stage_for<VkStage>::value, code));
    }

    ShaderModule(VkDevice device, std::filesystem::path filename) :device(device) {
        if (filename.extension() == ".spv") {
            auto spv_blob_u8 = readFile(filename.string(), true);
            std::vector<uint32_t> spv_blob(spv_blob_u8.size() / 4);
            memcpy(spv_blob.data(), spv_blob_u8.data(), spv_blob_u8.size());
            build(spv_blob);
        }
        else
            build(glsl2spv(glslang_stage_for<VkStage>::value, readFile(filename.string()).data()));
    }

    ~ShaderModule() {
        vkDestroyShaderModule(device, module, nullptr);
    }
};

