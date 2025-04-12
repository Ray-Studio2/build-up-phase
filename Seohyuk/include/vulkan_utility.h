

#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <algorithm>
#include <iostream>



namespace vkut {

    namespace display {
#define WIDTH 800
#define HEIGHT 600

#define SWAPCHAIN_IMAGE_COUNT 2
#define DISPLAY_DIM vk::Extent2D {WIDTH, HEIGHT}

        /*
        inline VkSwapchainCreateInfoKHR createSwapchainInfo(VkPhysicalDevice physical_device, VkSurfaceKHR vkSurface, uint32_t queueFamily) {

            VkPhysicalDeviceSurfaceInfo2KHR physicalDeviceSurfaceInfo2KHR;
            uint32_t surfaceFormatCount = 0;
            std::vector<VkSurfaceFormat2KHR> surfaceFormats{};
            vkGetPhysicalDeviceSurfaceFormats2KHR(physical_device, &physicalDeviceSurfaceInfo2KHR, &surfaceFormatCount, nullptr);
            vkGetPhysicalDeviceSurfaceFormats2KHR(physical_device, &physicalDeviceSurfaceInfo2KHR, &surfaceFormatCount, surfaceFormats.data());

            auto surfaceCreateInfo = VkDisplaySurfaceCreateInfoKHR{
            .sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR,}

            return nullptr;
        }
    }*/

        // not the best way but it will work and comfortable to read. vk::enumerateInstanceLayerProperties function will not take that much reasource.
        // check if given instance name is supported by the system. if it is supported, the function will stop and return true.
        /* useless for now
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
        */

        // the instance layers that can be used and selected at the same time.
        /* usless for now
        inline void filterSupportedInstanceLayers(std::vector<const char *> &layersRequested) {
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
        */
    }
}


