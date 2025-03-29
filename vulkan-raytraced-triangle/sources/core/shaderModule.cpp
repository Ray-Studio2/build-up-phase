#include "core/shaderModule.hpp"
#include "core/app.hpp"

#include <iostream>
#include <fstream>
#include <vector>
using std::cout, std::endl;
using std::ifstream;
using std::vector;

vector<char> readShader(const char* filePath) {
    ifstream reader(filePath, std::ios::binary | std::ios::ate);
    vector<char> fileData;

    if (reader) {
        unsigned long long fileSize = static_cast<unsigned long long>(reader.tellg());
        fileData.resize(fileSize);

        reader.seekg(std::ios::beg);
        reader.read(fileData.data(), fileSize);
        reader.close();
    }

    return fileData;
}

/* -------- Constructors & Destructor -------- */
ShaderModule::ShaderModule() { }
ShaderModule::ShaderModule(VkShaderStageFlagBits stage, const char* filePath)
    : mLogicalDevice{App::device()}, mStage{stage} {
    vector<char> spv = readShader(filePath);

    if (!spv.empty()) {
        VkShaderModuleCreateInfo shaderModuleInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = spv.size(),
            .pCode = reinterpret_cast<const unsigned int*>(spv.data())
        };

        if (vkCreateShaderModule(mLogicalDevice, &shaderModuleInfo, nullptr, &mHandle) != VK_SUCCESS)
            cout << "[ERROR]: vkCreateShaderModule()" << endl;
    }
    else
        cout << "[ERROR]: Failed to open file \"" << filePath << '\"' << endl;
}
ShaderModule::~ShaderModule() noexcept { vkDestroyShaderModule(mLogicalDevice, mHandle, nullptr); }
/* ------------------------------------------- */

/* ----------------- Getters ----------------- */
VkPipelineShaderStageCreateInfo ShaderModule::info() const noexcept {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = mStage,
        .module = mHandle,
        .pName = "main"
    };
}
/* ------------------------------------------- */