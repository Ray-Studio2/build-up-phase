

#pragma once

#include <vulkan/vulkan.h>

namespace vkut {
    VkBool32 checkValidationLayerSupport();
    void showInstanceLayers();
    void showInstanceExtensions();
}