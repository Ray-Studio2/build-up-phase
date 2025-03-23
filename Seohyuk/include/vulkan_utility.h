

#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <algorithm>

namespace vkut {
    bool isInstanceExtensionSupported(const std::string &extension) {
        for (const auto element: vk::enumerateInstanceLayerProperties()) {
            auto text = std::string(element.layerName);

            if (text.compare(extension)) {
                return true;
            }
        }

        return false;
    }
}
