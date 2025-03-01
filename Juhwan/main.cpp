#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <filesystem>
#include <tuple>
#include <bitset>
#include <span>
#include "shader_module.h"

typedef unsigned int uint;

const uint32_t WIDTH = 1200;
const uint32_t HEIGHT = 800;
const uint32_t SHADER_GROUP_HANDLE_SIZE = 32;

#ifdef NDEBUG
    const bool ON_DEBUG = false;
#else
    const bool ON_DEBUG = true;
#endif


struct Global {
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rtProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue graphicsQueue; // assume allowing graphics and present
    uint queueFamilyIndex;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    // std::vector<VkImageView> swapChainImageViews;
    const VkFormat swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;    // intentionally chosen to match a specific format
    const VkExtent2D swapChainImageExtent = { .width = WIDTH, .height = HEIGHT };

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence fence0;

    ////////////////////////////////////////////////////////////////////

    VkBuffer blasBuffer;                // BLAS Buffer
    VkDeviceMemory blasBufferMem;
    VkAccelerationStructureKHR blas;
    VkDeviceAddress blasAddress;

    VkBuffer tlasBuffer;                // TLAS Buffer                                              // Binding 0
    VkDeviceMemory tlasBufferMem;                     
    VkAccelerationStructureKHR tlas;

    VkImage outImage;                   // Ray Tracing Pipeline 에서 Render Target 이 되는 이미지    // Binding 1
    VkDeviceMemory outImageMem;                                                                 
    VkImageView outImageView;

    VkBuffer uniformBuffer;                                                                         // Binding 2
    VkDeviceMemory uniformBufferMem;

    VkDescriptorSetLayout descriptorSetLayout;  // 총 1 개의 Discriptor Set Layout 이 있다. ( TLAS, outImage, Uniform Buffer )
    VkPipelineLayout pipelineLayout;                // 총 3 개의 Binding Layout 이 있다. ( TLAS, outImage, Uniform Buffer )
    VkPipeline pipeline;

    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;              // Discriptor Set 은 총 1 개이다.

    VkBuffer sbtBuffer;                         // Shader Binding Table ( Staging Buffer 가 아닌, 실제로 Shader 에서 사용되는 Buffer )
    VkDeviceMemory sbtBufferMem;
    VkStridedDeviceAddressRegionKHR rgenSbt{};
    VkStridedDeviceAddressRegionKHR missSbt{};
    VkStridedDeviceAddressRegionKHR hitgSbt{};

    ////////////////////////////////////////////////////////////////////
    
    ~Global() {
        vkDestroyBuffer(device, tlasBuffer, nullptr);
        vkFreeMemory(device, tlasBufferMem, nullptr);
        vkDestroyAccelerationStructureKHR(device, tlas, nullptr);

        vkDestroyBuffer(device, blasBuffer, nullptr);
        vkFreeMemory(device, blasBufferMem, nullptr);
        vkDestroyAccelerationStructureKHR(device, blas, nullptr);

        vkDestroyImageView(device, outImageView, nullptr);
        vkDestroyImage(device, outImage, nullptr);
        vkFreeMemory(device, outImageMem, nullptr);

        vkDestroyBuffer(device, uniformBuffer, nullptr);
        vkFreeMemory(device, uniformBufferMem, nullptr);

        vkDestroyBuffer(device, sbtBuffer, nullptr);
        vkFreeMemory(device, sbtBufferMem, nullptr);

        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);


        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroyFence(device, fence0, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);

        // for (auto imageView : swapChainImageViews) {
        //     vkDestroyImageView(device, imageView, nullptr);
        // }
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroyDevice(device, nullptr);
        if (ON_DEBUG) {
            ((PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk.instance, "vkDestroyDebugUtilsMessengerEXT"))
                (vk.instance, vk.debugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
} vk;

void loadDeviceExtensionFunctions(VkDevice device)
{
    vk.vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
    vk.vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
    vk.vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
    vk.vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
    vk.vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
    vk.vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
	vk.vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
                                                                            // Ray Tracing Pipeline 을 만드는 데에 사용하는 함수
	vk.vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
    vk.vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
                                                                            // GPU 가 Ray 를 추적하도록 명령하는 함수

    VkPhysicalDeviceProperties2 deviceProperties2{                  // Ray Tracing 과 관련된 Vulkan 드라이버의 Property 들이 정의되어 있는 함수들이 있는데
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,    //  이 함수들을 얻어오는 과정 ( Ray Tracing Pipeline 과 관련된 property 들을 vk.rtProperties 에 넣는다. )
        .pNext = &vk.rtProperties,
    };
	vkGetPhysicalDeviceProperties2(vk.physicalDevice, &deviceProperties2);

    if (vk.rtProperties.shaderGroupHandleSize != SHADER_GROUP_HANDLE_SIZE) {        // shaderGroupHandleSize property 는 무조건 32 이어야 한다.
        throw std::runtime_error("shaderGroupHandleSize must be 32 mentioned in the vulakn spec (Table 69. Required Limits)!");
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    const char* severity;
    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severity = "[Verbose]"; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severity = "[Warning]"; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: severity = "[Error]"; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: severity = "[Info]"; break;
    default: severity = "[Unknown]";
    }

    const char* types;
    switch (messageType) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: types = "[General]"; break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: types = "[Performance]"; break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: types = "[Validation]"; break;
    default: types = "[Unknown]";
    }

    std::cout << "[Debug]" << severity << types << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

bool checkValidationLayerSupport(std::vector<const char*>& reqestNames) 
{
    uint32_t count;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> availables(count);
    vkEnumerateInstanceLayerProperties(&count, availables.data());

    for (const char* reqestName : reqestNames) {
        bool found = false;

        for (const auto& available : availables) {
            if (strcmp(reqestName, available.layerName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*>& reqestNames) 
{
    uint32_t count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> availables(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, availables.data());

    for (const char* reqestName : reqestNames) {
        bool found = false;

        for (const auto& available : availables) {
            if (strcmp(reqestName, available.extensionName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

GLFWwindow* createWindow()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    return glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void createVkInstance(GLFWwindow* window)
{
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .apiVersion = VK_API_VERSION_1_3
    };

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (ON_DEBUG) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    std::vector<const char*> validationLayers;
    if (ON_DEBUG) validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    if (!checkValidationLayerSupport(validationLayers)) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
    };

    VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = ON_DEBUG ? &debugCreateInfo : nullptr,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = (uint)validationLayers.size(),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = (uint)extensions.size(),
        .ppEnabledExtensionNames = extensions.data(),
    };

    if (vkCreateInstance(&createInfo, nullptr, &vk.instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    if (ON_DEBUG) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk.instance, "vkCreateDebugUtilsMessengerEXT");
        if (!func || func(vk.instance, &debugCreateInfo, nullptr, &vk.debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    if (glfwCreateWindowSurface(vk.instance, window, nullptr, &vk.surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void createVkDevice()
{
    vk.physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vk.instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vk.instance, &deviceCount, devices.data());

    std::vector<const char*> extentions = { 
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                                        // Acceleration Structure 을 사용하려면
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,                    //  VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME 과
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, // not used     //  VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME 과
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, // not used          //  VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME 을 추가해주어야 한다.
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,   // VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME 자체가 계층구조로 이루어져 있어서 위 3 개의 extension 이 필요하다. (사용하지 않더라도)

        VK_KHR_SPIRV_1_4_EXTENSION_NAME, // not used
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,     // VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME 또한 계층구조로 이루어져 있어서 VK_KHR_SPIRV_1_4_EXTENSION_NAME 가 필요하다.
    };

    ////////////////////////////////////////////

    /*for (const auto& device : devices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if (checkDeviceExtensionSupport(device, extentions))
        {
            std::cout << deviceProperties.deviceName << std::endl;
            vk.physicalDevice = device;
        }
    }*/

    ////////////////////////////////////////////

    for (const auto& device : devices)
    {
        if (checkDeviceExtensionSupport(device, extentions))
        {
            vk.physicalDevice = device;
            break;
        }
    }

    if (vk.physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk.physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vk.physicalDevice, &queueFamilyCount, queueFamilies.data());

    vk.queueFamilyIndex = 0;
    {
        for (; vk.queueFamilyIndex < queueFamilyCount; ++vk.queueFamilyIndex)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(vk.physicalDevice, vk.queueFamilyIndex, vk.surface, &presentSupport);

            if (queueFamilies[vk.queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport)
                break;
        }

        if (vk.queueFamilyIndex >= queueFamilyCount)
            throw std::runtime_error("failed to find a graphics & present queue!");
    }
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vk.queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = (uint)extentions.size(),
        .ppEnabledExtensionNames = extentions.data(),
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures f1{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .bufferDeviceAddress = VK_TRUE,
    };

	VkPhysicalDeviceAccelerationStructureFeaturesKHR f2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .accelerationStructure = VK_TRUE,
    };
	
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR f3{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .rayTracingPipeline = VK_TRUE,
    };

    createInfo.pNext = &f1;
    f1.pNext = &f2;
    f2.pNext = &f3;

    if (vkCreateDevice(vk.physicalDevice, &createInfo, nullptr, &vk.device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(vk.device, vk.queueFamilyIndex, 0, &vk.graphicsQueue);
}

void createSwapChain()
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physicalDevice, vk.surface, &capabilities);

    const VkColorSpaceKHR defaultSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    {
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, vk.surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, vk.surface, &formatCount, formats.data());

        bool found = false;
        for (auto format : formats) {
            if (format.format == vk.swapChainImageFormat && format.colorSpace == defaultSpace) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("");
        }
    }

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    {
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, vk.surface, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, vk.surface, &presentModeCount, presentModes.data());

        for (auto mode : presentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }

    uint imageCount = capabilities.minImageCount + 1;
    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk.surface,
        .minImageCount = imageCount,
        .imageFormat = vk.swapChainImageFormat,
        .imageColorSpace = defaultSpace,
        .imageExtent = {.width = WIDTH , .height = HEIGHT },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT, // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                            // 나중에 outImage 에서 Swap Chain Image 로 copy 를 하기 때문에
                                            //  VK_IMAGE_USAGE_TRANSFER_DST_BIT flag 로 copy 의 destination 으로 설정해주어야 한다.
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
    };

    if (vkCreateSwapchainKHR(vk.device, &createInfo, nullptr, &vk.swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(vk.device, vk.swapChain, &imageCount, nullptr);
    vk.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vk.device, vk.swapChain, &imageCount, vk.swapChainImages.data());

    // Swapchain 에서 나오는 이미지를 텍스쳐로 이용하지 않고, Graphics Pipeline 의 Render Target 으로도 사용하지 않을 것이다.
    // 즉, View 를 만들 필요가 없다.

    //for (const auto& image : vk.swapChainImages) {        // Swapchain 에서 나오는 이미지를 텍스쳐로 이용하지 않고, Graphics Pipeline 의 Render Target 으로도 사용하지 않을 것이다.
    //    VkImageViewCreateInfo createInfo{                   //  즉, View 를 만들 필요가 없다.
    //        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    //        .image = image,
    //        .viewType = VK_IMAGE_VIEW_TYPE_2D,
    //        .format = vk.swapChainImageFormat,
    //        .subresourceRange = {
    //            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    //            .levelCount = 1,
    //            .layerCount = 1,
    //        },
    //    };
        // VkImageView imageView;
        // if (vkCreateImageView(vk.device, &createInfo, nullptr, &imageView) != VK_SUCCESS) {
        //     throw std::runtime_error("failed to create image views!");
        // }
        // vk.swapChainImageViews.push_back(imageView);
    //}
}

void createCommandCenter()
{
    VkCommandPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vk.queueFamilyIndex,
    };

    if (vkCreateCommandPool(vk.device, &poolInfo, nullptr, &vk.commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }

    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk.commandPool,
        .commandBufferCount = 1,
    };

    if (vkAllocateCommandBuffers(vk.device, &allocInfo, &vk.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    if (vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &vk.imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &vk.renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(vk.device, &fenceInfo, nullptr, &vk.fence0) != VK_SUCCESS) {
        throw std::runtime_error("failed to create synchronization objects for a frame!");
    }

}


uint findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags reqMemProps)
{
    uint memTypeIndex = 0;
    std::bitset<32> isSuppoted(memoryTypeBits);

    VkPhysicalDeviceMemoryProperties spec;
    vkGetPhysicalDeviceMemoryProperties(vk.physicalDevice, &spec);

    for (auto& [props, _] : std::span<VkMemoryType>(spec.memoryTypes, spec.memoryTypeCount)) {
        if (isSuppoted[memTypeIndex] && (props & reqMemProps) == reqMemProps) {
            break;
        }
        ++memTypeIndex;
    }
    return memTypeIndex;
}

std::tuple<VkBuffer, VkDeviceMemory> createBuffer(
    VkDeviceSize size, 
    VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags reqMemProps)
{
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;

    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
    };
    if (vkCreateBuffer(vk.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vk.device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, reqMemProps),
    };

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkMemoryAllocateFlagsInfo flagsInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
            .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
        };
        allocInfo.pNext = &flagsInfo;
    }

    if (vkAllocateMemory(vk.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(vk.device, buffer, bufferMemory, 0);

    return { buffer, bufferMemory };
}

std::tuple<VkImage, VkDeviceMemory> createImage(
    VkExtent2D extent,
    VkFormat format,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags reqMemProps)
{
    VkImage image;
    VkDeviceMemory imageMemory;

    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { extent.width, extent.height, 1 },
        .mipLevels = 1,                                 // 출력에 사용되는 Image 이기 때문에 Mipmap 은 필요하지 않다.
        .arrayLayers = 1,                               // Cube Map 과 같은 특수한 상황에 사용되는 Image 가 아니기 때문에 필요하지 않다.
        .samples = VK_SAMPLE_COUNT_1_BIT,               // Multisampling 을 하지 않는다.
        .tiling = VK_IMAGE_TILING_OPTIMAL,              // 특수한 상황에서 사용되는 VK_IMAGE_TILING_LINEAR 도 있지만, 보통 VK_IMAGE_TILING_OPTIMAL 을 사용한다.
        .usage = usage,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,     // 특수한 상황에서 사용되는 VK_IMAGE_LAYOUT_PREINITIALIZED 도 있지만, 보통 VK_IMAGE_LAYOUT_UNDEFINED 을 사용한다.
    };
    if (vkCreateImage(vk.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;                               // 이후의 작업은 Buffer 을 생성하는 작업과 유사하다.
    vkGetImageMemoryRequirements(vk.device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, reqMemProps),
    };

    if (vkAllocateMemory(vk.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) { // Image Memory 할당
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(vk.device, image, imageMemory, 0);

    return { image, imageMemory };  // Tuple 로 Return
}

void setImageLayout(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, // 동기화 부분으로 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT 로 커버가 가능하지만, 최적화가 되어 있기 않아 그리 좋은 선택지는 아니다.
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
{
    VkImageMemoryBarrier barrier{                           // Image Memory Barrier 는 총 3 가지의 용도가 있다.
                                                            //  1. 동기화를 도와주는 용도로 사용 가능 ( 이를 위해 srcAccessMask 와 dstAccessMask, srcStageMask, dstStageMask 가 있다. )
                                                            //  2. 여러 개의 Queue Family (여러 개의 Queue 들이 병행하여 돌아간다는 의미) 를 사용할 때, Recource 에 대한 Race Condition 발생하지 않도록 제어해주는 용도로 사용 가능
                                                            //  3. Layout 전환 용도 ( 현재 이 Barrier 을 사용하는 목적 )
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldImageLayout,        // 현재 Layout 으로 지정
        .newLayout = newImageLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,     // Queue Family 를 1 개만 사용하므로 VK_QUEUE_FAMILY_IGNORED 사용
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresourceRange,       // 기존의 subresourceRange 전체를 사용할 것이다.
    };

                                                // VkImageMemoryBarrier 의 구성요소에는 VkBufferMemoryBarrier 에는 없는 Layout 관련 요소들이 존재한다. ( oldLayout, newLayout )
                                                // Buffer 의 쓰임이 기존의 용도에서 다른 용도로 전환되도록 할 때, 개발자는 직접 그 두 용도들에 맞는 Memory Layout 구조를 만들어야 한다.
                                                // 그래도 이러한 작업은 상대적으로 간단하다.
                                                // 하지만, Image 는 Buffer 에 비하면 수많은 Layout 이 존재한다. 너무 많은 Layout 이 존재하기에 개발자가 일일히 관리하기 너무 힘들다.
                                                // 따라서 Image Layout 전환과 관련해서는 Vulkan API 자체에서 관리를 해준다.

    if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }

    if (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }

    vkCmdPipelineBarrier(
        cmdbuffer, srcStageMask, dstStageMask, 0,
        0, nullptr, 0, nullptr, 1, &barrier);   // Image Memory Barrier 만 사용하기에 1 개의 barrier 만 넘긴다.
}

inline VkDeviceAddress getDeviceAddressOf(VkBuffer buffer)
{
    VkBufferDeviceAddressInfoKHR info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer,
    };
    return vk.vkGetBufferDeviceAddressKHR(vk.device, &info);
}

inline VkDeviceAddress getDeviceAddressOf(VkAccelerationStructureKHR as)
{
    VkAccelerationStructureDeviceAddressInfoKHR info{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = as,
    };
    return vk.vkGetAccelerationStructureDeviceAddressKHR(vk.device, &info);
}

void createBLAS()
{
    float vertices[][3] = {
        { -1.0f, -1.0f, 0.0f },
        {  1.0f, -1.0f, 0.0f },
        {  1.0f,  1.0f, 0.0f },
        { -1.0f,  1.0f, 0.0f },
    };
    uint32_t indices[] = { 0, 1, 3, 1, 2, 3 };

    VkTransformMatrixKHR geoTransforms[] = {
        {
            1.0f, 0.0f, 0.0f, -2.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        }, 
        {
            1.0f, 0.0f, 0.0f, 2.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        },
    };

    auto [vertexBuffer, vertexBufferMem] = createBuffer(
        sizeof(vertices), 
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    auto [indexBuffer, indexBufferMem] = createBuffer(
        sizeof(indices), 
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    auto [geoTransformBuffer, geoTransformBufferMem] = createBuffer(
        sizeof(geoTransforms), 
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    void* dst;

    vkMapMemory(vk.device, vertexBufferMem, 0, sizeof(vertices), 0, &dst);
    memcpy(dst, vertices, sizeof(vertices));
    vkUnmapMemory(vk.device, vertexBufferMem);

    vkMapMemory(vk.device, indexBufferMem, 0, sizeof(indices), 0, &dst);
    memcpy(dst, indices, sizeof(indices));
    vkUnmapMemory(vk.device, indexBufferMem);

    vkMapMemory(vk.device, geoTransformBufferMem, 0, sizeof(geoTransforms), 0, &dst);
    memcpy(dst, geoTransforms, sizeof(geoTransforms));
    vkUnmapMemory(vk.device, geoTransformBufferMem);

    VkAccelerationStructureGeometryKHR geometry0{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry = {
            .triangles = {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
                .vertexData = { .deviceAddress = getDeviceAddressOf(vertexBuffer) },
                .vertexStride = sizeof(vertices[0]),
                .maxVertex = sizeof(vertices) / sizeof(vertices[0]) - 1,
                .indexType = VK_INDEX_TYPE_UINT32,
                .indexData = { .deviceAddress = getDeviceAddressOf(indexBuffer) },
                .transformData = { .deviceAddress = getDeviceAddressOf(geoTransformBuffer) },
            },
        },
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };
    VkAccelerationStructureGeometryKHR geometries[] = { geometry0, geometry0 };

    uint32_t triangleCount0 = sizeof(indices) / (sizeof(indices[0]) * 3);
    uint32_t triangleCounts[] = { triangleCount0, triangleCount0 };

    VkAccelerationStructureBuildGeometryInfoKHR buildBlasInfo{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount = sizeof(geometries) / sizeof(geometries[0]),
        .pGeometries = geometries,
    };
    
    VkAccelerationStructureBuildSizesInfoKHR requiredSize{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vk.vkGetAccelerationStructureBuildSizesKHR(
        vk.device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildBlasInfo,
        triangleCounts,
        &requiredSize);

    std::tie(vk.blasBuffer, vk.blasBufferMem) = createBuffer(
        requiredSize.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);           // Device (GPU) 의 주소를 가져오려면 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT 가 꼭 필요하다.

    auto [scratchBuffer, scratchBufferMem] = createBuffer(
        requiredSize.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Generate BLAS handle
    {
        VkAccelerationStructureCreateInfoKHR asCreateInfo{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .buffer = vk.blasBuffer,
            .size = requiredSize.accelerationStructureSize,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        };
        vk.vkCreateAccelerationStructureKHR(vk.device, &asCreateInfo, nullptr, &vk.blas);

        vk.blasAddress = getDeviceAddressOf(vk.blas);
    }

    // Build BLAS using GPU operations
    {
        vkResetCommandBuffer(vk.commandBuffer, 0);
        VkCommandBufferBeginInfo beginInfo {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(vk.commandBuffer, &beginInfo);
        {
            buildBlasInfo.dstAccelerationStructure = vk.blas;
            buildBlasInfo.scratchData.deviceAddress = getDeviceAddressOf(scratchBuffer);
            VkAccelerationStructureBuildRangeInfoKHR buildBlasRangeInfo[] = {
                { 
                    .primitiveCount = triangleCounts[0],
                    .transformOffset = 0,
                },
                { 
                    .primitiveCount = triangleCounts[1],
                    .transformOffset = sizeof(geoTransforms[0]),
                }
            };

            VkAccelerationStructureBuildGeometryInfoKHR buildBlasInfos[] = { buildBlasInfo };
            VkAccelerationStructureBuildRangeInfoKHR* buildBlasRangeInfos[] = { buildBlasRangeInfo };
            vk.vkCmdBuildAccelerationStructuresKHR(vk.commandBuffer, 1, buildBlasInfos, buildBlasRangeInfos);
            // vkCmdBuildAccelerationStructuresKHR(vk.commandBuffer, 1, &buildBlasInfo, 
            // (const VkAccelerationStructureBuildRangeInfoKHR *const *)&buildBlasRangeInfo);
        }
        vkEndCommandBuffer(vk.commandBuffer);

        VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &vk.commandBuffer,
        }; 
        vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(vk.graphicsQueue);
    }

    vkFreeMemory(vk.device, scratchBufferMem, nullptr);
    vkFreeMemory(vk.device, vertexBufferMem, nullptr);
    vkFreeMemory(vk.device, indexBufferMem, nullptr);
    vkFreeMemory(vk.device, geoTransformBufferMem, nullptr);
    vkDestroyBuffer(vk.device, scratchBuffer, nullptr);
    vkDestroyBuffer(vk.device, vertexBuffer, nullptr);
    vkDestroyBuffer(vk.device, indexBuffer, nullptr);
    vkDestroyBuffer(vk.device, geoTransformBuffer, nullptr);
}

void createTLAS()                                   
{
    VkTransformMatrixKHR insTransforms[] = {
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 2.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        }, 
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, -2.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        },
    };

    VkAccelerationStructureInstanceKHR instance0 {
        .instanceCustomIndex = 100,
        .mask = 0xFF,
        .instanceShaderBindingTableRecordOffset = 0,
        .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        .accelerationStructureReference = vk.blasAddress,
    };
    VkAccelerationStructureInstanceKHR instanceData[] = { instance0, instance0 };
    instanceData[0].transform = insTransforms[0];
    instanceData[1].transform = insTransforms[1];
    instanceData[1].instanceShaderBindingTableRecordOffset = 2; // 2 geometry (in instance0) + 2 geometry (in instance1)

    auto [instanceBuffer, instanceBufferMem] = createBuffer(
        sizeof(instanceData), 
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);    

    void* dst;
    vkMapMemory(vk.device, instanceBufferMem, 0, sizeof(instanceData), 0, &dst);
    memcpy(dst, instanceData, sizeof(instanceData));
    vkUnmapMemory(vk.device, instanceBufferMem);

    VkAccelerationStructureGeometryKHR instances{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry = {
            .instances = {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                .data = { .deviceAddress = getDeviceAddressOf(instanceBuffer) },
            },
        },
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };

    uint32_t instanceCount = 2;

    VkAccelerationStructureBuildGeometryInfoKHR buildTlasInfo{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount = 1,     // It must be 1 with .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR as shown in the vulkan spec.
        .pGeometries = &instances,
    };

    VkAccelerationStructureBuildSizesInfoKHR requiredSize{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    vk.vkGetAccelerationStructureBuildSizesKHR(
        vk.device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildTlasInfo,
        &instanceCount,
        &requiredSize);

    std::tie(vk.tlasBuffer, vk.tlasBufferMem) = createBuffer(
        requiredSize.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);       // TLAS 의 Buffer 주소는 사용하지 않아서 예전에 있었던 tlasAddress 를 이제는 사용하지 않는다.

    auto [scratchBuffer, scratchBufferMem] = createBuffer(
        requiredSize.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Generate TLAS handle
    {
        VkAccelerationStructureCreateInfoKHR asCreateInfo{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .buffer = vk.tlasBuffer,
            .size = requiredSize.accelerationStructureSize,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        };
        vk.vkCreateAccelerationStructureKHR(vk.device, &asCreateInfo, nullptr, &vk.tlas);
    }

    // Build TLAS using GPU operations
    {
        vkResetCommandBuffer(vk.commandBuffer, 0);
        VkCommandBufferBeginInfo beginInfo {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(vk.commandBuffer, &beginInfo);
        {
            buildTlasInfo.dstAccelerationStructure = vk.tlas;
            buildTlasInfo.scratchData.deviceAddress = getDeviceAddressOf(scratchBuffer);

            VkAccelerationStructureBuildRangeInfoKHR buildTlasRangeInfo = { .primitiveCount = instanceCount };
            VkAccelerationStructureBuildRangeInfoKHR* buildTlasRangeInfo_[] = { &buildTlasRangeInfo };
            vk.vkCmdBuildAccelerationStructuresKHR(vk.commandBuffer, 1, &buildTlasInfo, buildTlasRangeInfo_);
        }
        vkEndCommandBuffer(vk.commandBuffer);

        VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &vk.commandBuffer,
        }; 
        vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(vk.graphicsQueue);
    }

    vkFreeMemory(vk.device, scratchBufferMem, nullptr);
    vkFreeMemory(vk.device, instanceBufferMem, nullptr);
    vkDestroyBuffer(vk.device, scratchBuffer, nullptr);
    vkDestroyBuffer(vk.device, instanceBuffer, nullptr);
}

void createOutImage()
{                               // UNORM 은 8 bit (0 에서 255) 를 0 에서 1 사이의 실수값으로 바꾼 것.
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM; //VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB(==vk.swapChainImageFormat)
                                                    // Swap Chain 에서는 VK_FORMAT_B8G8R8A8_SRGB format 이 표준적이다.
    std::tie(vk.outImage, vk.outImageMem) = createImage(
        { WIDTH, HEIGHT },                   // Graphics Pipeline 에서는 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT 가 사용되어야 하고,
                                             //  Compute Pipeline 이나 Raytracing Pipeline 에서는 VK_IMAGE_USAGE_STORAGE_BIT 가 추가되어야 한다.
        format,                 // 일반적으로 Image 는 Texture 로 사용되는데, 그러한 용도가 아닌 오직 Raytracing Pipeline 에서의 Write 를 위한 용도로 사용되기에 VK_IMAGE_USAGE_STORAGE_BIT flag 가 추가되어야 한다. 
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,   // 나중에 outImage 에서 Swap Chain Image 로 copy 를 하기 때문에 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);                           //  VK_IMAGE_USAGE_TRANSFER_SRC_BIT flag 를 해야 outImage 를 copy 의 source 로 설정할 수 있다.

    VkImageSubresourceRange subresourceRange{       // Mipmap Level 과 Array Layer Level 을 각각 1 로 설정 (설정해주지 않으면 0 임)
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1,
    };
    
    VkImageViewCreateInfo ci0{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = vk.outImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,                               // 특수한 상황이 아닌 이상, View 와 출력 Image 의 Format 을 일치시켜주면 된다.
        .components = {                     // VK_COMPONENT_SWIZZLE_IDENTITY 는 R 값은 그대로 R 값으로 B 값은 그대로 B 값으로 유지하게 하는 역할을 한다.
            .r = VK_COMPONENT_SWIZZLE_B,    // R 값은 사실상 B 값이 되도록 설정
            .b = VK_COMPONENT_SWIZZLE_R,    // B 값 또한 사실상 R 값임
        },
        .subresourceRange = subresourceRange,
    };
    vkCreateImageView(vk.device, &ci0, nullptr, &vk.outImageView);  // Ray Tracing Shader 의 출력으로 사용될 View 를 만들어서 이 View 에 접근해야 한다.

    vkResetCommandBuffer(vk.commandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(vk.commandBuffer, &beginInfo);
    {
        setImageLayout(                 // Image 는 Buffer 와 다르게 Layout 이 존재한다.
                                        // VK_IMAGE_LAYOUT_UNDEFINED 와 VK_IMAGE_LAYOUT_PREINITIALIZED 이 Initial Layout 으로 사용되는데,
                                        //  Image 를 사용하는 것은 VK_IMAGE_LAYOUT_UNDEFINED 에서는 불가능하기 때문에
                                        //  목적에 맞는 다른 Layout 으로 바꾸어야 한다.
            vk.commandBuffer, 
            vk.outImage, 
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_GENERAL,    // VK_IMAGE_LAYOUT_GENERAL layout 은 다른 다양한 용도들로 사용될 수 있다. (다만, 특수 목적에 맞게 최적화가 되어 있지 않아서 그리 좋은 Layout 은 아니다.)
            subresourceRange);
    }
    vkEndCommandBuffer(vk.commandBuffer);

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &vk.commandBuffer,
    }; 
    vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vk.graphicsQueue);  // 무작정 대기보다는, 비동기적으로 작동하도록 바꿔주면 좋다.
}

void createUniformBuffer()
{
    struct Data{                    // 16 Bytes 단위로 나뉘도록 설정
        float cameraPos[3];
        float yFov_degree;
    } dataSrc; 

    std::tie(vk.uniformBuffer, vk.uniformBufferMem) = createBuffer(
        sizeof(dataSrc), 
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* dst;
    vkMapMemory(vk.device, vk.uniformBufferMem, 0, sizeof(dataSrc), 0, &dst);
    *(Data*) dst = {0, 0, 10, 60};
    vkUnmapMemory(vk.device, vk.uniformBufferMem);
}

const char* raygen_src = R"(
#version 460
#extension GL_EXT_ray_tracing : enable

layout(binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, rgba8) uniform image2D image;
layout(binding = 2) uniform CameraProperties 
{
    vec3 cameraPos;
    float yFov_degree;
} g;

layout(location = 0) rayPayloadEXT vec3 hitValue;

void main() 
{
    const vec3 cameraX = vec3(1, 0, 0);
    const vec3 cameraY = vec3(0, -1, 0);
    const vec3 cameraZ = vec3(0, 0, -1);
    const float aspect_y = tan(radians(g.yFov_degree) * 0.5);
    const float aspect_x = aspect_y * float(gl_LaunchSizeEXT.x) / float(gl_LaunchSizeEXT.y);

    const vec2 screenCoord = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 ndc = screenCoord/vec2(gl_LaunchSizeEXT.xy) * 2.0 - 1.0;
    vec3 rayDir = ndc.x*aspect_x*cameraX + ndc.y*aspect_y*cameraY + cameraZ;

    hitValue = vec3(0.0);

    traceRayEXT(
        topLevelAS,                         // topLevel
        gl_RayFlagsOpaqueEXT, 0xff,         // rayFlags, cullMask
        0, 1, 0,                            // sbtRecordOffset, sbtRecordStride, missIndex
        g.cameraPos, 0.0, rayDir, 100.0,    // origin, tmin, direction, tmax
        0);                                 // payload

    imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(hitValue, 0.0));
})";

const char* miss_src = R"(
#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;

void main()
{
    hitValue = vec3(0.0, 0.0, 0.2);
})";

const char* chit_src = R"(
#version 460
#extension GL_EXT_ray_tracing : enable

layout(shaderRecordEXT) buffer CustomData
{
    vec3 color;
};

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

void main()
{
    if (gl_PrimitiveID == 1 && 
        gl_InstanceID == 1 && 
        gl_InstanceCustomIndexEXT == 100 && 
        gl_GeometryIndexEXT == 1) {
        hitValue = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    }
    else {
        hitValue = color;
    }
})";

void createRayTracingPipeline()
{
    VkDescriptorSetLayoutBinding bindings[] = {     // raygen_src 의 Binding Layout 3 개
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,       // raygen_src Shader 에서만 사용 ( miss_src 와 chit_src 에서는 사용 안 함 )
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
    };

    VkDescriptorSetLayoutCreateInfo ci0{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = sizeof(bindings) / sizeof(bindings[0]),
        .pBindings = bindings,
    };
    vkCreateDescriptorSetLayout(vk.device, &ci0, nullptr, &vk.descriptorSetLayout);

    VkPipelineLayoutCreateInfo ci1{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &vk.descriptorSetLayout,
    };
    vkCreatePipelineLayout(vk.device, &ci1, nullptr, &vk.pipelineLayout);       // Pipeline Layout 만듦

    ShaderModule<VK_SHADER_STAGE_RAYGEN_BIT_KHR> raygenModule(vk.device, raygen_src);
    ShaderModule<VK_SHADER_STAGE_MISS_BIT_KHR> missModule(vk.device, miss_src);
    ShaderModule<VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR> chitModule(vk.device, chit_src);
    VkPipelineShaderStageCreateInfo stages[] = { raygenModule, missModule, chitModule };

    VkRayTracingShaderGroupCreateInfoKHR shaderGroups[] = {
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = 0,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = 1,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = 2,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
    };
    
    VkRayTracingPipelineCreateInfoKHR ci2{
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .stageCount = sizeof(stages) / sizeof(stages[0]),
        .pStages = stages,
        .groupCount = sizeof(shaderGroups) / sizeof(shaderGroups[0]),
        .pGroups = shaderGroups,
        .maxPipelineRayRecursionDepth = 1,
        .layout = vk.pipelineLayout,
    };
    vk.vkCreateRayTracingPipelinesKHR(vk.device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &ci2, nullptr, &vk.pipeline);
}

void createDescriptorSets()     // Descriptor Set Layout 이 설계도이면, 이 설계도대로 실제로 저장되어 있는 것이 Descriptor Set 이다.
{                               //  그리고, Descriptor Set 은 Descriptor Pool 안에 있다.
    VkDescriptorPoolSize poolSizes[] = {    // 따라서 Pool 을 먼저 만들어주어야 한다. ( 우선, Pool 내의 구성요소 각각에 대한 개수 설정 )
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
    };
    VkDescriptorPoolCreateInfo ci0 {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]),
        .pPoolSizes = poolSizes,
    };
    vkCreateDescriptorPool(vk.device, &ci0, nullptr, &vk.descriptorPool);

    VkDescriptorSetAllocateInfo ai0 {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vk.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &vk.descriptorSetLayout,
    };
    vkAllocateDescriptorSets(vk.device, &ai0, &vk.descriptorSet);

    VkWriteDescriptorSet write_temp{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = vk.descriptorSet,
        .descriptorCount = 1,
    };

    // Descriptor(binding = 0), VkAccelerationStructure
    VkWriteDescriptorSetAccelerationStructureKHR desc0{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &vk.tlas,  
    };
    VkWriteDescriptorSet write0 = write_temp;
    write0.pNext = &desc0;
    write0.dstBinding = 0;
    write0.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    
    // Descriptor(binding = 1), VkImage for output
    VkDescriptorImageInfo desc1{
        .imageView = vk.outImageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL, // raygen_src Shader 에서 해당 리소스 ( outImage ) 에 접근하는 순간, 그 Image 가 가지고 있어야 할 Layout 상태
    };
    VkWriteDescriptorSet write1 = write_temp;
    write1.dstBinding = 1;
    write1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write1.pImageInfo = &desc1;
 
    // Descriptor(binding = 2), VkBuffer for uniform
    VkDescriptorBufferInfo desc2{
        .buffer = vk.uniformBuffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    VkWriteDescriptorSet write2 = write_temp;
    write2.dstBinding = 2;
    write2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write2.pBufferInfo = &desc2;

    VkWriteDescriptorSet writeInfos[] = { write0, write1, write2 };
    vkUpdateDescriptorSets(vk.device, sizeof(writeInfos) / sizeof(writeInfos[0]), writeInfos, 0, VK_NULL_HANDLE);
    /*
    [VUID-VkWriteDescriptorSet-descriptorType-00336]
    If descriptorType is VK_DESCRIPTOR_TYPE_STORAGE_IMAGE or VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 
    the imageView member of each element of pImageInfo must have been created with the identity swizzle.
    */
}

struct ShaderGroupHandle {
    uint8_t data[SHADER_GROUP_HANDLE_SIZE];
};

struct HitgCustomData {
    float color[3];
};

/*
In the vulkan spec,
[VUID-vkCmdTraceRaysKHR-stride-03686] pMissShaderBindingTable->stride must be a multiple of VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupHandleAlignment
[VUID-vkCmdTraceRaysKHR-stride-03690] pHitShaderBindingTable->stride must be a multiple of VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupHandleAlignment
[VUID-vkCmdTraceRaysKHR-pRayGenShaderBindingTable-03682] pRayGenShaderBindingTable->deviceAddress must be a multiple of VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupBaseAlignment
[VUID-vkCmdTraceRaysKHR-pMissShaderBindingTable-03685] pMissShaderBindingTable->deviceAddress must be a multiple of VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupBaseAlignment
[VUID-vkCmdTraceRaysKHR-pHitShaderBindingTable-03689] pHitShaderBindingTable->deviceAddress must be a multiple of VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupBaseAlignment

As shown in the vulkan spec 40.3.1. Indexing Rules,  
    pHitShaderBindingTable->deviceAddress + pHitShaderBindingTable->stride × (
    instanceShaderBindingTableRecordOffset + geometryIndex × sbtRecordStride + sbtRecordOffset )
*/
void createShaderBindingTable() 
{
    auto alignTo = [](auto value, auto alignment) -> decltype(value) {
        return (value + (decltype(value))alignment - 1) & ~((decltype(value))alignment - 1);
    };
    const uint32_t handleSize = SHADER_GROUP_HANDLE_SIZE;
    const uint32_t groupCount = 3; // 1 raygen, 1 miss, 1 hit group
    std::vector<ShaderGroupHandle> handels(groupCount);
    vk.vkGetRayTracingShaderGroupHandlesKHR(vk.device, vk.pipeline, 0, groupCount, handleSize * groupCount, handels.data());
    ShaderGroupHandle rgenHandle = handels[0];
    ShaderGroupHandle missHandle = handels[1];
    ShaderGroupHandle hitgHandle = handels[2];

    const uint32_t rgenStride = alignTo(handleSize, vk.rtProperties.shaderGroupHandleAlignment);
    vk.rgenSbt = { 0, rgenStride, rgenStride };
    
    const uint64_t missOffset = alignTo(vk.rgenSbt.size, vk.rtProperties.shaderGroupBaseAlignment);
    const uint32_t missStride = alignTo(handleSize, vk.rtProperties.shaderGroupHandleAlignment);
    vk.missSbt = { 0, missStride, missStride };

    const uint32_t hitgCustomDataSize = sizeof(HitgCustomData);
    const uint32_t geometryCount = 4;
    const uint64_t hitgOffset = alignTo(missOffset + vk.missSbt.size, vk.rtProperties.shaderGroupBaseAlignment);
    const uint32_t hitgStride = alignTo(handleSize + hitgCustomDataSize, vk.rtProperties.shaderGroupHandleAlignment);
    vk.hitgSbt = { 0, hitgStride, hitgStride * geometryCount };

    const uint64_t sbtSize = hitgOffset + vk.hitgSbt.size;
    std::tie(vk.sbtBuffer, vk.sbtBufferMem) = createBuffer(
        sbtSize,
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    auto sbtAddress = getDeviceAddressOf(vk.sbtBuffer);
    if (sbtAddress != alignTo(sbtAddress, vk.rtProperties.shaderGroupBaseAlignment)) {
        throw std::runtime_error("It will not be happened!");
    }
    vk.rgenSbt.deviceAddress = sbtAddress;
    vk.missSbt.deviceAddress = sbtAddress + missOffset;
    vk.hitgSbt.deviceAddress = sbtAddress + hitgOffset;

    uint8_t* dst;
    vkMapMemory(vk.device, vk.sbtBufferMem, 0, sbtSize, 0, (void**)&dst);
    {
        *(ShaderGroupHandle*)dst = rgenHandle; 
        *(ShaderGroupHandle*)(dst + missOffset) = missHandle;

        *(ShaderGroupHandle*)(dst + hitgOffset + 0 * hitgStride             ) = hitgHandle;
        *(HitgCustomData*   )(dst + hitgOffset + 0 * hitgStride + handleSize) = {0.6f, 0.1f, 0.2f}; // Deep Red Wine
        *(ShaderGroupHandle*)(dst + hitgOffset + 1 * hitgStride             ) = hitgHandle;
        *(HitgCustomData*   )(dst + hitgOffset + 1 * hitgStride + handleSize) = {0.1f, 0.8f, 0.4f}; // Emerald Green
        *(ShaderGroupHandle*)(dst + hitgOffset + 2 * hitgStride             ) = hitgHandle;
        *(HitgCustomData*   )(dst + hitgOffset + 2 * hitgStride + handleSize) = {0.9f, 0.7f, 0.1f}; // Golden Yellow
        *(ShaderGroupHandle*)(dst + hitgOffset + 3 * hitgStride             ) = hitgHandle;
        *(HitgCustomData*   )(dst + hitgOffset + 3 * hitgStride + handleSize) = {0.3f, 0.6f, 0.9f}; // Dawn Sky Blue
    }
    vkUnmapMemory(vk.device, vk.sbtBufferMem);
}

void render()
{
    static const VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    static const VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    static const VkImageCopy copyRegion = {
        .srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .extent = { WIDTH, HEIGHT, 1 },
    };
    static const VkStridedDeviceAddressRegionKHR callSbt{};

    vkWaitForFences(vk.device, 1, &vk.fence0, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &vk.fence0);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(vk.device, vk.swapChain, UINT64_MAX, vk.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(vk.commandBuffer, 0);
    if (vkBeginCommandBuffer(vk.commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    {
        vkCmdBindPipeline(vk.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vk.pipeline);
        vkCmdBindDescriptorSets(
            vk.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 
            vk.pipelineLayout, 0, 1, &vk.descriptorSet, 0, 0);

        vk.vkCmdTraceRaysKHR(
            vk.commandBuffer,
            &vk.rgenSbt,
            &vk.missSbt,
            &vk.hitgSbt,
            &callSbt,
            WIDTH, HEIGHT, 1);
        
        setImageLayout(
            vk.commandBuffer,
            vk.outImage,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            subresourceRange);
            
        setImageLayout(
            vk.commandBuffer,
            vk.swapChainImages[imageIndex],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            subresourceRange);
        
        vkCmdCopyImage(                 // outImage (Ray tracing 결과) 에서 Swap Chain Image 로 복사
            vk.commandBuffer,
            vk.outImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vk.swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copyRegion);

        setImageLayout(
            vk.commandBuffer,
            vk.outImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL,
            subresourceRange);

        setImageLayout(
            vk.commandBuffer,
            vk.swapChainImages[imageIndex],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            subresourceRange);
    }
    if (vkEndCommandBuffer(vk.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    VkSemaphore waitSemaphores[] = { 
        vk.imageAvailableSemaphore 
    };
    VkPipelineStageFlags waitStages[] = { 
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT 
    };
    
    VkSubmitInfo submitInfo{ 
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = sizeof(waitSemaphores) / sizeof(waitSemaphores[0]),
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &vk.commandBuffer,
    };
    
    if (vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, vk.fence0) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &vk.swapChain,
        .pImageIndices = &imageIndex,
    };
    
    vkQueuePresentKHR(vk.graphicsQueue, &presentInfo);
}

int main()
{
    glfwInit();
    GLFWwindow* window = createWindow();
    createVkInstance(window);
    createVkDevice();
    loadDeviceExtensionFunctions(vk.device);
    createSwapChain();
    createCommandCenter();
    createSyncObjects();

    createBLAS();
    createTLAS();
    createOutImage();               // Raytracing 용 출력 Image 만들기
    createUniformBuffer();
    createRayTracingPipeline();
    createDescriptorSets();
    createShaderBindingTable();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        render();
    }

    vkDeviceWaitIdle(vk.device);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}