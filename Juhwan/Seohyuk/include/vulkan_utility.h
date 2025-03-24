

#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <algorithm>
#include <iostream>


namespace vkut {

    // not the best way but it will work and comfortable to read. vk::enumerateInstanceLayerProperties function will not take that much reasource.
    // check if given instance name is supported by the system. if it is supported, the function will stop and return true.
    inline bool isLayerSupported(const std::string &requested) {
        const std::vector<vk::LayerProperties> supportedInstanceLayers = vk::enumerateInstanceLayerProperties();
        for (
            auto supported_instance_layer: supportedInstanceLayers) {
                char * charArr = supported_instance_layer.layerName;

                auto str1 = std::string(charArr);

                if ( requested.compare(charArr) == 0) {
                    return true;
                }
            }

        return false;
    }

    // the instance layers that can be used and selected at the same time.
    inline void filterSupportedInstanceLayers(std::vector<std::string> &layersRequested) {
        if (layersRequested.empty()) {
            return;
        }
        layersRequested.erase(
            std::ranges::remove_if(
                layersRequested,
                [](const std::string &layer) {
                        return !isLayerSupported(layer);
                    }
                )
            .begin(),
            layersRequested.end()
        );
    }
}


