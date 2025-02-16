//
// Created by anisa on 2025-01-30.
//

#include <vulkan/vulkan.hpp>
#include "Renderer.hpp"
#include "Utility.hpp"
#include <algorithm>
#include <iostream>
#include <vector>


namespace renderer {
    NeftyRenderingEngine::NeftyRenderingEngine_(const std::string &applicationName, const uint32_t appVersion, const bool offscreen) {

        { /* in the constructor, we will let the caller select physical device. so we need to prepare. */
            const auto appInfo = vk::ApplicationInfo()
                                                .setPApplicationName(applicationName.c_str())
                                                .setApplicationVersion(appVersion)
                                                .setApiVersion(VK_API_VERSION_1_1)
                                                .setEngineVersion(0);
            char const * instanceLayerNames[] = {"VK_LAYER_KHRONOS_validation"};

            std::vector<const char *> vecInstanceExtensionNames = {};

            if (!offscreen) {
                vecInstanceExtensionNames.push_back("VK_KHR_surface");
                vecInstanceExtensionNames.push_back("VK_KHR_win32_surface");
            }

            vecInstanceExtensionNames.push_back("VK_EXT_debug_report");
            const auto instanceCreateInfo = vk::InstanceCreateInfo()
                                                .setPApplicationInfo(&appInfo)
                                                .setPEnabledLayerNames(instanceLayerNames)
                                                .setPEnabledExtensionNames(*vecInstanceExtensionNames.data());
            this->instance = vk::createInstanceUnique(instanceCreateInfo);
        }

        { /* See how meny device is available in the computer. and move them to array with size of maximum number to support. */
            int count = 0;
            for(const auto &physicalDevice: this->instance->enumeratePhysicalDevices()) {
                this->physicalDevices[count] = physicalDevice;

                count += 1;
                if (count >= MAX_DEVICES) {
                    break;
                }
            }
        }

    }

    NeftyRenderingEngine_ *NeftyRenderingEngine::renderOnWindow() { return this; }

    vk::PhysicalDevice NeftyRenderingEngine_::getPhysicalDevice(const uint32_t selection) const {
        return this->physicalDevices[selection];
    }

    /**
     * Selection of physical device. by default is 0
     *
     * @param selection 0 ~ 8
     * @return this
     */
    NeftyRenderingEngine_ * NeftyRenderingEngine::selectPhysicalDeviceByNumber(const uint32_t selection) {
        this->physicalDeviceSelection = selection;
        return this;
    }

    NeftyRenderingEngine_ * NeftyRenderingEngine::prepareRendering() {
    {
        auto physicalDevice = this->physicalDevices[physicalDeviceSelection];

        //auto queueCreateInfo = vk::QueueCreateInfo();
        //auto deviceCreateInfo = vk::DeviceCreateInfo().setQueueCreateInfos();

    }


        return this;
    }



    NeftyRenderingEngine::~NeftyRenderingEngine_() = default;


}
