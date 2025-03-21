#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <filesystem>
#include <tuple>
#include <bitset>
#include <span>
#include "EngineTypes.h"
#include "RenderSettings.h"

namespace A3
{
#ifdef NDEBUG
const bool ON_DEBUG = false;
#else
const bool ON_DEBUG = true;
#endif

class VulkanRendererBackend
{
public:
    VulkanRendererBackend( GLFWwindow* window );
    ~VulkanRendererBackend();

    void beginFrame();
    void endFrame();

    void beginRaytracingPipeline();

    void initImgui( GLFWwindow* window );

    //@TODO: Move to renderer
    void createBLAS();
    void createTLAS();
    void createOutImage();
    void createUniformBuffer();
    void createRayTracingPipeline();
    void createDescriptorSets();
    void createShaderBindingTable();
    //////////////////////////

private:
    void loadDeviceExtensionFunctions( VkDevice device );

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData );

    bool checkValidationLayerSupport( std::vector<const char*>& reqestNames );

    bool checkDeviceExtensionSupport( VkPhysicalDevice device, std::vector<const char*>& reqestNames );

    void createVkInstance();

    void createVkSurface( GLFWwindow* window );

    void createVkDevice();

    void createSwapChain();

    void createCommandCenter();

    void createSyncObjects();

    uint32 findMemoryType( uint32_t memoryTypeBits, VkMemoryPropertyFlags reqMemProps );

    std::tuple<VkBuffer, VkDeviceMemory> createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags reqMemProps );

    std::tuple<VkImage, VkDeviceMemory> createImage(
        VkExtent2D extent,
        VkFormat format,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags reqMemProps );

    void setImageLayout(
        VkCommandBuffer cmdbuffer,
        VkImage image,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT );

    VkDeviceAddress getDeviceAddressOf( VkBuffer buffer );

    VkDeviceAddress getDeviceAddressOf( VkAccelerationStructureKHR as );

private:
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rtProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue graphicsQueue; // assume allowing graphics and present
    uint32 queueFamilyIndex;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    // std::vector<VkImageView> swapChainImageViews;
    const VkFormat swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;    // intentionally chosen to match a specific format
    const VkExtent2D swapChainImageExtent = { .width = RenderSettings::screenWidth, .height = RenderSettings::screenHeight };

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    uint32 imageIndex;
    VkFence fence0;

    VkBuffer blasBuffer;
    VkDeviceMemory blasBufferMem;
    VkAccelerationStructureKHR blas;
    VkDeviceAddress blasAddress;

    VkBuffer tlasBuffer;
    VkDeviceMemory tlasBufferMem;
    VkAccelerationStructureKHR tlas;

    VkImage outImage;
    VkDeviceMemory outImageMem;
    VkImageView outImageView;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMem;

    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    VkBuffer sbtBuffer;
    VkDeviceMemory sbtBufferMem;
    VkStridedDeviceAddressRegionKHR rgenSbt{};
    VkStridedDeviceAddressRegionKHR missSbt{};
    VkStridedDeviceAddressRegionKHR hitgSbt{};
    VkStridedDeviceAddressRegionKHR callSbt{};

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    VkImageCopy copyRegion = {
        .srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .extent = { RenderSettings::screenWidth, RenderSettings::screenHeight, 1 },
    };

    VkRenderPass imguiRenderPass;
    VkDescriptorPool imguiDescriptorPool;

    friend class Addon_imgui;
};
}