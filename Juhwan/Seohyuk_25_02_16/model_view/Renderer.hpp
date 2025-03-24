//
// Created by anisa on 2025-02-01.
//

#ifndef RENDERER_CPP_H
#define RENDERER_CPP_H

#include <vulkan/vulkan.hpp>

#define MAX_DEVICES 8

#define ENGINE_NAME "nefty_renderer"
#define API_VERSION VK_VERSION_1_2

namespace renderer {

    /**
     * The rendering engine will take the data and rendering callback.
     * also it initialize vulkan for the rendering.
     * ALL members are public. don't have to worry about using it.
     */
    typedef struct NeftyRenderingEngine_ {
        /**
         *
         * there will be
         *
         * instance
         * device
         * queue
         * commandPool
         *
         * and the draw method to draw.
         *
         * a single pipline will be presented as default.
         *
         */
        vk::UniqueInstance instance;

        uint32_t physicalDeviceSelection{};
        vk::PhysicalDevice physicalDevices[MAX_DEVICES];

        vk::UniqueDevice device;
        vk::Queue supermanQueue;


        NeftyRenderingEngine_(const std::string& applicationName, uint32_t appVersion, bool offscreen = false);

        NeftyRenderingEngine_ * renderOnWindow();
        [[nodiscard]] vk::PhysicalDevice getPhysicalDevice(uint32_t selection) const;
        NeftyRenderingEngine_ * selectPhysicalDeviceByNumber(uint32_t selection);
        NeftyRenderingEngine_ * prepareRendering();

        ~NeftyRenderingEngine_();

    } NeftyRenderingEngine;
}

#endif //RENDERER_CPP_H
