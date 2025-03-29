#pragma once

#include "vulkan/vulkan.h"

class ShaderModule {
    ShaderModule(const ShaderModule&) = delete;
    ShaderModule(ShaderModule&&) noexcept = delete;

    ShaderModule& operator=(const ShaderModule&) = delete;
    ShaderModule& operator=(ShaderModule&&) noexcept = delete;

    public:
        ShaderModule();
        ShaderModule(VkShaderStageFlagBits stage, const char* filePath);      // (shader stage, shader file path)
        ~ShaderModule() noexcept;

        VkPipelineShaderStageCreateInfo info() const noexcept;

    private:
        bool mIsCreated{ };

        VkShaderStageFlagBits mStage{ };

        VkDevice mLogicalDevice{ };
        VkShaderModule mHandle{ };
};