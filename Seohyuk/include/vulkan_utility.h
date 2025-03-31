

#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <algorithm>
#include <iostream>



namespace vkut {

    namespace display {
        #define WIDTH 800
        #define HEIGHT 600

        #define SWAPCHAIN_IMAGE_COUNT 2
        #define SWAPCHAIN_IMAGE_FORMAT vk::Format::eB8G8R8Srgb
        #define SWAPCHAIN_IMAGE_COLORSPACE vk::ColorSpaceKHR::eSrgbNonlinear
        #define DISPLAY_DIM vk::Extent2D {WIDTH, HEIGHT}


        vk::SwapchainKHR createSwapchainInfo(vk::Device device, vk::SurfaceKHR vkSurface, uint32_t queueFamily) {
            auto a = vk::SwapchainCreateInfoKHR {
                {},
                vkSurface,
                SWAPCHAIN_IMAGE_COUNT,
                SWAPCHAIN_IMAGE_FORMAT,
                SWAPCHAIN_IMAGE_COLORSPACE,
                vk::Extent2D {WIDTH, HEIGHT},
                1,
                vk::ImageUsageFlagBits::eTransferDst,
                vk::SharingMode::eExclusive,
                queueFamily,
                vk::SurfaceTransformFlagBitsKHR::eIdentity, // preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                vk::CompositeAlphaFlagBitsKHR::eOpaque, // compositeAlpha =  VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                vk::PresentModeKHR::eMailbox, // presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
                VK_TRUE // CLIPED
            };


            /* TODO: swapchain
            vk::SwapchainKHR swapchain;

            return PFN_vkGetSwapchainImagesKHR(device, &swapchain, SWAPCHAIN_IMAGE_COUNT, );

            Get swapchain images associated with this swapchain
                uint32_t uint32_t_temp_21(3u);
                GetSwapchainImagesKHR(VkDevice_uid_3, VkSwapchainKHR_uid_13, &uint32_t_temp_21, nullptr);
                std::vector<VkImage> std_vector_of_VkImage_temp_2(uint32_t_temp_21);
                GetSwapchainImagesKHR(VkDevice_uid_3, VkSwapchainKHR_uid_13, &uint32_t_temp_21, std_vector_of_VkImage_temp_2.data());
                VkImage_uid_14 = std_vector_of_VkImage_temp_2[0]; VkImage_uid_15 = std_vector_of_VkImage_temp_2[1]; VkImage_uid_16 = std_vector_of_VkImage_temp_2[2];
            */
        }
    }

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


