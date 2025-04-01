#pragma once

#include "core/types.hpp"

namespace Settings {
    struct App {
        /*
         * Vulkan SDK 1.4.304.0
         *     #define VK_API_VERSION_1_3 VK_MAKE_API_VERSION(0, 1, 3, 0)// Patch version should always be set to 0
         *     #define VK_MAKE_API_VERSION(variant, major, minor, patch) \
         *             ((((uint32_t)(variant)) << 29U) | (((uint32_t)(major)) << 22U) | (((uint32_t)(minor)) << 12U) | ((uint32_t)(patch)))
         */
        static constexpr       uint32 VK_API_VER_1_3 = 4'206'592U;    // (1U << 22U) | (3U << 12U)
        static constexpr const char*  VK_APP_NAME    = "RayStudio";
    };

    struct Extension {
        static constexpr const char* DEVICE[] = {
            "VK_KHR_swapchain"               ,      // VK_KHR_SWAPCHAIN_EXTENSION_NAME

            "VK_EXT_descriptor_indexing"     ,      // VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
            "VK_KHR_buffer_device_address"   ,      // VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
            "VK_KHR_deferred_host_operations",      // VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
            "VK_KHR_acceleration_structure"  ,      // VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME

            "VK_KHR_spirv_1_4"               ,      // VK_KHR_SPIRV_1_4_EXTENSION_NAME
            "VK_KHR_ray_tracing_pipeline"           // VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
        };
    };
}