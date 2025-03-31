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

#ifdef NDEBUG
const bool ON_DEBUG = false;
#else
const bool ON_DEBUG = true;
#endif

namespace A3
{
struct Vertex;

class VulkanRendererBackend
{
public:
    VulkanRendererBackend( GLFWwindow* window, std::vector<const char*>& extensions, int32 screenWidth, int32 screenHeight );
    ~VulkanRendererBackend();

    void beginFrame( int32 screenWidth, int32 screenHeight );
    void endFrame();

    void beginRaytracingPipeline();

    // @TODO: Make private
    void createVkInstance( std::vector<const char*>& extensions );
    void createVkSurface( GLFWwindow* window );
    void createVkPhysicalDevice();
    void createVkQueueFamily();
    void createVkDescriptorPools();
    void createSwapChain();
    void createImguiRenderPass( int32 screenWidth, int32 screenHeight );
    void createCommandCenter();

    void rebuildAccelerationStructure();

    //@TODO: Move to renderer
    VkAccelerationStructureKHR createBLAS( const std::vector<Vertex>& vertexData, const std::vector<uint32>& indexData );
    void createTLAS();
    void createOutImage();
    void createUniformBuffer();
    void createRayTracingPipeline();
    void createRayTracingDescriptorSet();
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

    VkAllocationCallbacks* allocator;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue graphicsQueue; // assume allowing graphics and present
    uint32 queueFamilyIndex;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> framebuffers;
    const VkFormat swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;    // intentionally chosen to match a specific format
    const VkExtent2D swapChainImageExtent = { .width = RenderSettings::screenWidth, .height = RenderSettings::screenHeight };

    std::vector<VkCommandPool> commandPools;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> fences;
    uint32 semaphoreIndex;
    uint32 imageIndex;

    VkBuffer blasBuffer;
    VkDeviceMemory blasBufferMem;
    VkAccelerationStructureKHR blas;

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

    friend class Addon_imgui;
};
}