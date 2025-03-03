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
const uint32_t SHADER_GROUP_HANDLE_SIZE = 32;   // 무조건 32 이어야 한다.

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
    VkDeviceMemory sbtBufferMem;                //  Discriptor Set 을 통하지 않는 Shader 에서 접근 가능한 특별한 Buffer 
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
        0, nullptr, 0, nullptr, 1, &barrier);   // Image Memory Barrier 만 사용하기에 마지막 1 개의 barrier 만 넘긴다.
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
        .instanceCustomIndex = 100,                     // Instance ID 는 Instance Buffer 에 Instance 를 적재한 순서대로 지정되지만, 이 또한 마음대로 지정해줄 수 있다.
        .mask = 0xFF,                                   //  현재 이 Instance 의 Custom Index 는 100 이다.
        .instanceShaderBindingTableRecordOffset = 0,                // 첫 번째 Instance 는 instanceShaderBindingTableRecordOffset 를 0 으로 설정.
        .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        .accelerationStructureReference = vk.blasAddress,
    };
    VkAccelerationStructureInstanceKHR instanceData[] = { instance0, instance0 };
    instanceData[0].transform = insTransforms[0];
    instanceData[1].transform = insTransforms[1];
    instanceData[1].instanceShaderBindingTableRecordOffset = 2; // 2 geometry (in instance0) + 2 geometry (in instance1)
                                                                    // 두 번째 Instance 는 instanceShaderBindingTableRecordOffset 를 2 로 설정.

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
                                                // Vulkan 의 쉐이더 파일은 spv 이고, 어셈블리 언어의 형식을 가지고 있다.
                                                // glsl 은 하나의 Shader 파일에 1 가지 종류의 Shader 1 개만 넣을 수 있지만,
                                                //  hlsl 은 하나의 Shader 파일에 여러 가지 종류의 Shader 여러 개를 넣을 수 있다.
                                                // 그래서 glsl 에는 main 함수 (진입점 함수 이름이 고정됨) 가 무조건 1 개씩은 있게 해서 진입점을 명시하지만,
                                                //  hlsl 은 여러 Shader 들이 있으므로 집입점을 DirectX 에서 명시하여 이를 해결한다.
                                                // Vulkan 의 spv 는 glsl 과 hlsl 둘 다 호환이 되어야 하는데, 이를 위해
                                                //  하나의 파일 안에 여러 개의 Shader 를 포함할 수 있고 진입점 또한 main 이 아닌 다른 이름을 사용할 수 있도록 만들어졌다.
                                                // 대신, Vulkan 에서 진입점의 이름을 명시해주어야 한다.

const char* raygen_src = R"(            // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#traceray-control-flow 참고
#version 460
#extension GL_EXT_ray_tracing : enable          // gl_LaunchSizeEXT 와 같은 변수를 사용하기 위해 필요한 작업

layout(binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, rgba8) uniform image2D image;
layout(binding = 2) uniform CameraProperties 
{
    vec3 cameraPos;                             // 카메라 위치만 있지 카메라 회전 정보는 없어서 카메라는 정면만 바라보고 있다.
    float yFov_degree;
} g;

layout(location = 0) rayPayloadEXT vec3 hitValue;   // Raytracing flow 상에서 shader 들끼리 서로 데이터를 주고받는 것이 rayPayloadEXT 인데, 이는 마음껏 커스터마이징이 가능하다.
                                                        // location 은 0 인데, 이는 0 번째 패킷형식을 의미한다.

void main() 
{
    const vec3 cameraX = vec3(1, 0, 0);
    const vec3 cameraY = vec3(0, -1, 0);
    const vec3 cameraZ = vec3(0, 0, -1);                            // 카메라가 오브젝트를 바라보게 하기 위해 -z 방향을 바라보도록 함
    const float aspect_y = tan(radians(g.yFov_degree) * 0.5);
    const float aspect_x = aspect_y * float(gl_LaunchSizeEXT.x) / float(gl_LaunchSizeEXT.y);
                                                                        // WIDTH 와 HEIGHT 를 곱한 개수의 core 들 각각 raygen Shader 가 실행되는데,
                                                                        //  이 core 들을 각각 구별해줘야하기 때문에 gl_LaunchIDEXT 변수를 제공받아 사용한다.

    const vec2 screenCoord = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 ndc = screenCoord/vec2(gl_LaunchSizeEXT.xy) * 2.0 - 1.0;
    vec3 rayDir = ndc.x*aspect_x*cameraX + ndc.y*aspect_y*cameraY + cameraZ;

    hitValue = vec3(0.0);

    traceRayEXT(
        topLevelAS,                         // topLevel ( BVH (Bounded Volume Hierarchy) 로 구성된 Scene (자료구조) 에서
                                            //  travelse 하며 후보들을 찾는데, 이 travelse 할 TLAS 인 topLevelAS 를 지정한다.
        gl_RayFlagsOpaqueEXT, 0xff,         // rayFlags, cullMask   ( 거의 이렇게 사용된다. )
                                                // 충돌한 삼각형 (부분) 이 opaque (불투명한) 인 경우 어떻게 처리할 지에 대한 flag 를 gl_RayFlagsOpaqueEXT 로 설정하였고,
                                                //  불투명한 경우 tmax 가 업데이트 된다.
        0, 1, 0,                            // sbtRecordOffset, sbtRecordStride, missIndex
                                                // sbtRecordStride 는 필요한 geometry 의 record 를 찾을 때 몇 칸을 이동해야 하는 지 결정하는 데에 사용된다. 
                                                // missIndex 는 어떠한 miss Shader 을 호출할 지 결정할 때 참고함다.
        g.cameraPos, 0.0, rayDir, 100.0,    // origin, tmin, direction, tmax    
                                                // origin 은 카메라의 위치 ( ray 의 시작점 )
                                                //  origin 으로부터 direction 방향으로 tmin 부터 tmax 거리까지 ray 를 설정 ( rayDir 크기의 100 배까지로 설정됨 )
                                                // 매번, ray 와 삼각형의 발견된 교차점이 tamx 보다 가까우면 (그리고 opaque 하면) tmax 는 업데이트된다.
        0);                                 // payload
                                                // rayPayloadEXT 와 관련해 주고받는 데이터들이 하나의 패킷으로 만들어져 Shader 들끼리 통신하는데, 이러한 패킷들은 여러 개일 수 있다.
                                                // 이 payload 는 그 중 0 번째 패킷형식을 의미한다.

    imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(hitValue, 0.0));   // 출력으로 사용하는 image 에 RGB 값인 hitValue 를 gl_LaunchIDEXT.xy 픽셀 위치에 작성한다.
                                                                            // hitValue 는 처음에는 (0, 0, 0) 로 초기화되었지만, traceRayEXT 함수 호출 시 Shader 끼리의 통신을 통해 값이 업데이트 되었다.
})";

const char* miss_src = R"(
#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;

void main()
{
    hitValue = vec3(0.0, 0.0, 0.2);     // ray 가 충돌이 없으면 해당 값으로 결정
})";

const char* chit_src = R"(
#version 460
#extension GL_EXT_ray_tracing : enable

layout(shaderRecordEXT) buffer CustomData
{
    vec3 color;
};

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;   // 삼각형 내부의 점은 삼각형의 3 개의 vertices 에 대한 가충치로 표현이 가능한데,
                                //  이 3 개의 가중치들의 합은 항상 1 이라서 vec2 로도 충분하다.

void main()
{
    if (gl_PrimitiveID == 1 &&          // gl_PrimitiveID 는 Shader 에서 제공하는 전역변수인데,
                                        //  BLAS 생성할 때 삼각형 Index 들을 넣을 때 Index 3 개마다 Primitive (삼각형 ID) 가 자동으로 추가된다.
        gl_InstanceID == 1 &&           // TLAS 생성에서 2 개의 Instance 를 만들었는데, 각 Instance 들은 각각 동일한 BLAS 를 포함하고 있다.
                                        //  그리고 이 Instance 들 각각은 자동으로 순서대로 배정된 Instance ID 를 가지고 있다.
        gl_InstanceCustomIndexEXT == 100 &&     // Instance 생성에서 Instance 에 Custom Index 설정이 가능한데, 해당 Instance ID 는 100 이다.
        gl_GeometryIndexEXT == 1) {     // BLAS 생성에서 하나의 BLAS 밑에 2 개의 Geometry 를 추가했는데, 추가한 순서대로 Geometry ID 가 자동으로 붙는다.
        hitValue = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);    // 삼각형의 v0, v1, v2 각각에 대한 가중치들 
    }                                                                               // ray 와 삼각형의 교차점에 대한 가중치들을 알 수 있다.
    else {
        hitValue = color;
    }
})";

//      결과는
//              | Instance 0 |          | Instance 0 |
//              | Geometry 0 |          | Geometry 1 |
//                                                              이렇게 출력이 된다.
//              | Instance 1 |          | Instance 1 |
//              | Geometry 0 |          | Geometry 1 |
//
//      그리고 우하단의 Geometry 에서 2 번째 Primitive 에 해당하는 삼각형은 색상 Interpolation 이 적용되어 있다.


void createRayTracingPipeline()         // RayTracing Pipeline 에는 단 1 개의 raygeneration Shader 만 들어갈 수 있고,
                                        //  miss Shader 와 anyhit Shader, closesthit Shader 는 여러 개가 들어갈 수 있다.
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
    vkCreatePipelineLayout(vk.device, &ci1, nullptr, &vk.pipelineLayout);       // 여기까지가 Pipeline Layout 만들기

    ShaderModule<VK_SHADER_STAGE_RAYGEN_BIT_KHR> raygenModule(vk.device, raygen_src);
    ShaderModule<VK_SHADER_STAGE_MISS_BIT_KHR> missModule(vk.device, miss_src);
    ShaderModule<VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR> chitModule(vk.device, chit_src);
    VkPipelineShaderStageCreateInfo stages[] = { raygenModule, missModule, chitModule };    // 3 개의 ShaderModule 을 stage 로 엮음 (배열로 저장해서 stages 에는 앞에서부터 Index 가 붙는다.)
                                        // ShaderModule 에서 VkPipelineShaderStageCreateInfo 로 암묵적인 형변환을 할 수 있는 연산자를 만들어 적용

    VkRayTracingShaderGroupCreateInfoKHR shaderGroups[] = {                         // Group 은 총 3 개
                                                                                    // Stage 와 Group 은 완전히 다른 것이다.
                                                                                    // Shader 소스 코드가 총 3 개이므로 Stage 는 총 3 개이다. (Shader Module 1 개당 Stage 1 개)
                                                                                    // shaderGroup (Group) 은 Stage 개수와 전혀 상관이 없다.

        {                                                                           // General Group (raygenModule 를 참조하고 있음)
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,                   // VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR type 으로 하면
                                                                                    //  무조건 closestHitShader, anyHitShader, intersectionShader 모두
                                                                                    //  VK_SHADER_UNUSED_KHR 로 설정해야 한다.
            .generalShader = 0,                                                     // stages 배열에서 0 번째 Index 인 stage ( raygenModule )
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
        {                                                                           // miss Shader 가 여러 개 있는 경우, 이들에 대한 General Group (missModule 을 참조)
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = 1,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,    // Triangles Hit Group 
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,       // 이 Group 에서는 generalShader, intersectionShader 모두 VK_SHADER_UNUSED_KHR 이어야 하고,
            .generalShader = VK_SHADER_UNUSED_KHR,                                  //  closestHitShader, anyHitShader 은 VK_SHADER_UNUSED_KHR 이어도 되고 아니어도 된다.
            .closestHitShader = 2,                                                  // 다만, closestHitShader, anyHitShader 둘 중 최소 하나는 지정이 되어야 한다.
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
    };
    
    VkRayTracingPipelineCreateInfoKHR ci2{                                  // 예전에 Compute Pipeline 만들 때에는 Compute Shader 하나만 사용해서 sType, stage, layout 만 넣어주어도 되었었는데
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,    //  Raytracing Pipeline 과 Graphics Pipeline 은 여러 종류의 Shader 들을 사용해서
        .stageCount = sizeof(stages) / sizeof(stages[0]),                   //  stageCount 와 pStages 가 들어가야 한다.
                                                                            // Graphics Pipeline 에는 Vertex 와 Fragment Shader 가 1 개씩밖에 들어갈 수 없지만
                                                                            //  Raytracing Pipeline 에는 rayGen Shader 는 1 개밖에 들어갈 수 없으나
                                                                            //  miss, anyHit, closestHit Shader 는 여러 개가 들어갈 수 있다.

        .pStages = stages,                                                  // 위에서 만든 stage 를 넣어줌
        .groupCount = sizeof(shaderGroups) / sizeof(shaderGroups[0]),
        .pGroups = shaderGroups,                                            // 
        .maxPipelineRayRecursionDepth = 1,                                  // 재귀 호출을 최대 몇 번까지 진행하는 지 설정
                                                                                // GPU 는 CPU 와 다르게 함수 호출 시 스택을 사용하지 않는다.
                                                                                // CPU 에서는 함수 호출 시 무조건 스택을 사용하지만
                                                                                //  GPU 에서는 스택을 사용하지 않는 대신 C++ 의 Inline 함수처럼 동작한다.
                                                                                // GPU 함수는 매개인자를 받으면 따로 해당 매개변수를 위한 공간(스택) 을 만들지 않고
                                                                                //  바로 받은 변수의 위치를 즉각적으로 이용해서 최적화가 잘 되어 있다.
                                                                            // GPU 함수는 재귀호출이 불가능해서 CPU 함수를 많이 사용했었지만,
                                                                            //  Raytracing 은 재귀호출이 필수적이어서 GPU 에서도 재귀호출이 가능하도록
                                                                            //  내부적으로 작업이 되어있다. (물론, 재귀호출이 없는 것이 제일 좋은 최적화이다.)
        .layout = vk.pipelineLayout,
    };
    vk.vkCreateRayTracingPipelinesKHR(vk.device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &ci2, nullptr, &vk.pipeline);   // Raytracing Pipeline 생성
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
        .pSetLayouts = &vk.descriptorSetLayout,     // descriptorPool 내부의 descriptorSet 에 메모리를 할당해주어야 하기 때문에, descriptorSetLayout 이 필요하다.
    };
    vkAllocateDescriptorSets(vk.device, &ai0, &vk.descriptorSet);

    VkWriteDescriptorSet write_temp{                        // 아래의 write 3 개의 공통적인 부분들을 여기에서 설정
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

    VkWriteDescriptorSet writeInfos[] = { write0, write1, write2 };     // binding 0, 1, 2 에 write 하기 위한 작업
    vkUpdateDescriptorSets(vk.device, sizeof(writeInfos) / sizeof(writeInfos[0]), writeInfos, 0, VK_NULL_HANDLE);
    /*
    [VUID-VkWriteDescriptorSet-descriptorType-00336]
    If descriptorType is VK_DESCRIPTOR_TYPE_STORAGE_IMAGE or VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 
    the imageView member of each element of pImageInfo must have been created with the identity swizzle.
    */
}

struct ShaderGroupHandle {
    uint8_t data[SHADER_GROUP_HANDLE_SIZE]; // Shader Binding Table 을 이루는 Record 들의 개수
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
void createShaderBindingTable()             // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#raytracing-output 참고

                                                // Shader Binding Table 은 일련의 Record 들의 연속으로 이루어져 있다.
                                                // 하나의 Record 안에는 Shader Handle (Shader Identifier) (필수 O) 과 추가적인 데이터들 (필수 X) 이 들어가 있는데,
                                                //  Shader Handle 은 하나의 Shader 가 아닌 Group (Shader Group) 에 대한 Handle 이다.
                                                // 예를 들어, Hit Group 의 경우, anyHit 과 closestHit Shader 는 여러 개가 있을 수 있는데 이 Group 을 다루는 Handle 이 있는 것이다.
                                                
                                                // Shader Binding Table 의 목적 2 가지 :
                                                    // 1. hit 발생 시 해당하는 오브젝트의 material 에 해당하는 적절한 closest Shader 을 호출하기 위해 필요
                                                    // 2. 추가적인 데이터를 사용하기 위함
                                                    //      (Raytracing 은 한 번에 모든 Scene 오브젝트들을 호출하기 때문에 uniform 을 통해 오브젝트마다 각기 다른 값을 받아올 수가 없다.
                                                    //        이로 인해 Handle 뒤에 추가적인 데이터들을 넣는 것이다.)
                                                
                                                // Shader Binding Table 의 Handle 의 목적 :
                                                    //  geometry 마다 record 가 하나씩 있었고 각 record 마다 handle 이 하나씩 있었는데,
                                                    //   기존의 rasterizer 에서는 모든 mesh 들 (거대한 Scene 의 경우) 을 하나하나 drawCall 을 해야 했는데,
                                                    //   Raytracing 에서는 drawCall 을 단 한 번만 하면 되어서 훨씬 복잡한 상황들이 많아진다.
                                                    //  즉, 대량의 data 들을 다루는 것이 Raytracing 에서 더 난이도가 있다.
                                                    //  Ray 들을 Shooting 하게 되면 rayGen Shader 을 호출해서 삼각형들과의 교차가 일어나는 지 여부를 구하게 되는데
                                                    //   Ray 가 충돌되면 closest Shader 가 호출되고, 그렇지 않으면 miss Shader 가 호출된다.
                                                    //  BVH 에 모든 오브젝트들을 다 집어넣어야 하는데, 각 오브젝트들마다 material 이 다르고 material 마다 closest Shader 가 다른데,
                                                    //   충돌이 발생하면 해당 충돌점의 오브젝트에 해당하는 closest Shader 가 필요하다.
                                                    //  이를 위해서 Geometry Record 의 맨 앞에 Shader Handle 이 저장되어 있는 것이다.
                                                    //  이 Handle 안에는 anyHit Shader 와 closestHit Shader 가 있기 때문에 어떤 적당한 closestHit Shader 를 사용해야 할 지 알 수 있다.
                                                
{
    auto alignTo = [](auto value, auto alignment) -> decltype(value) {                          // alignment 는 2 의 승수이어야 한다.
        return (value + (decltype(value))alignment - 1) & ~((decltype(value))alignment - 1);
    };
    const uint32_t handleSize = SHADER_GROUP_HANDLE_SIZE;
    const uint32_t groupCount = 3; // 1 raygen, 1 miss, 1 hit group     // 총 Group 의 개수
    std::vector<ShaderGroupHandle> handels(groupCount);
    vk.vkGetRayTracingShaderGroupHandlesKHR(vk.device, vk.pipeline, 0, groupCount, handleSize * groupCount, handels.data());
                                                    // Group Handle 이 32 Byte 정보로 이루어져 있는데 이를 가져오는 작업이다.
                                                    // 만든 pipeline 에서 0 번째부터 3 개의 Group 들을 가져와 총 96 Byte 를 확보하고 handels.data() 에 저장한다.
    ShaderGroupHandle rgenHandle = handels[0];  // Handle 정보들을 각각의 용도에 따라 분리
    ShaderGroupHandle missHandle = handels[1];
    ShaderGroupHandle hitgHandle = handels[2];
                            // Stride 는 Record 의 Byte 길이를 의미한다.     // shaderGroupHandleAlignment 는 32 보다 작은 값이다. (2 의 승수이다.)
    const uint32_t rgenStride = alignTo(handleSize, vk.rtProperties.shaderGroupHandleAlignment);    // shaderGroupHandleAlignment 는 GPU 드라이버마다 다를 수 있다.
    vk.rgenSbt = { 0, rgenStride, rgenStride };     // rayGen Shader Binding Table 은 stride 와 size 가 반드시 동일해야 한다.
                // Device Address, Stride, Size         // stride 는 shaderGroupHandleAlignment 의 배수이어야 한다. size 는 상관 없지만 stride 의 배수이다.
    
    const uint64_t missOffset = alignTo(vk.rgenSbt.size, vk.rtProperties.shaderGroupBaseAlignment);
    const uint32_t missStride = alignTo(handleSize, vk.rtProperties.shaderGroupHandleAlignment);
    vk.missSbt = { 0, missStride, missStride };     // miss Shader Binding Table 은 stride 와 size 가 동일한 크기일 필요는 없다.
                                                        // miss Shader 가 여러 개가 있을 수 있고, 각 miss Shader 마다 Group Handle 이 하나씩 있어야 한다.
                                                        // 만약 miss Shader 가 3 개가 있다면, miss Shader Binding Table 는 해당 offset 을 시작으로 3 개의 stride 만큼을 차지한다.

    const uint32_t hitgCustomDataSize = sizeof(HitgCustomData); // 12 Byte 이 추가로 필요 (추가적인 데이터)
    const uint32_t geometryCount = 4;                   // offset 은 shaderGroupBaseAlignment 의 배수이어야 한다.
    const uint64_t hitgOffset = alignTo(missOffset + vk.missSbt.size, vk.rtProperties.shaderGroupBaseAlignment);
    const uint32_t hitgStride = alignTo(handleSize + hitgCustomDataSize, vk.rtProperties.shaderGroupHandleAlignment);   // 32 Byte 에 추가적인 데이터가 들어가서 총 32 Byte 2 개가 필요하다.
    vk.hitgSbt = { 0, hitgStride, hitgStride * geometryCount };     // geometryCount 는 4 개의 사각형을 표현하기 위함. 사각형 1 개마다 record 를 1 개씩 배정.

    // Shader Binding Table 의 구성 :
    // 
    //          | rgen | miss | hitg | hitg | hitg | hitg | ... ... |
    // offset:  |0     |64    |128   |192   |256   |320   | ... ... |
    // size :   |  32  |  32  |  64  |  64  |  64  |  64  | ... ... |

    const uint64_t sbtSize = hitgOffset + vk.hitgSbt.size;
    std::tie(vk.sbtBuffer, vk.sbtBufferMem) = createBuffer(
        sbtSize,
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);                // LOCAL 로 해도 되는데, 간단화를 위해 HOST 로 함 (쉽게 Update 가능)

    auto sbtAddress = getDeviceAddressOf(vk.sbtBuffer);
    if (sbtAddress != alignTo(sbtAddress, vk.rtProperties.shaderGroupBaseAlignment)) {  // 전체 Shader Binding Table 에 GPU 메모리 할당
        throw std::runtime_error("It will not be happened!");
    }
    vk.rgenSbt.deviceAddress = sbtAddress;
    vk.missSbt.deviceAddress = sbtAddress + missOffset;
    vk.hitgSbt.deviceAddress = sbtAddress + hitgOffset;     // 각 Shader Binding Table 의 시작주소 넣음

    uint8_t* dst;
    vkMapMemory(vk.device, vk.sbtBufferMem, 0, sbtSize, 0, (void**)&dst);   // HOST 로 하긴 했지만, 초기에 Buffer 와 값들을 한 번 지정해주고 그 이후로는 Update 하지 않을 것이다.
    {                                                       // ShaderGroupHandle* 로 강제 형변환을 하면 보기에도 더 좋고 실제로 더 빠르다.
        *(ShaderGroupHandle*)dst = rgenHandle;                  // rayGen Handle 를 넣기
        *(ShaderGroupHandle*)(dst + missOffset) = missHandle;   // miss Handle 을 넣기

        *(ShaderGroupHandle*)(dst + hitgOffset + 0 * hitgStride             ) = hitgHandle;                             // hitg Handle 을 넣어주고
        *(HitgCustomData*   )(dst + hitgOffset + 0 * hitgStride + handleSize) = {0.6f, 0.1f, 0.2f}; // Deep Red Wine    //  추가적인 데이터를 넣어준다.
        *(ShaderGroupHandle*)(dst + hitgOffset + 1 * hitgStride             ) = hitgHandle;
        *(HitgCustomData*   )(dst + hitgOffset + 1 * hitgStride + handleSize) = {0.1f, 0.8f, 0.4f}; // Emerald Green
        *(ShaderGroupHandle*)(dst + hitgOffset + 2 * hitgStride             ) = hitgHandle;
        *(HitgCustomData*   )(dst + hitgOffset + 2 * hitgStride + handleSize) = {0.9f, 0.7f, 0.1f}; // Golden Yellow
        *(ShaderGroupHandle*)(dst + hitgOffset + 3 * hitgStride             ) = hitgHandle;
        *(HitgCustomData*   )(dst + hitgOffset + 3 * hitgStride + handleSize) = {0.3f, 0.6f, 0.9f}; // Dawn Sky Blue

                                                                // rayGen 과 miss 에서 32 Byte 씩 2 번, 총 64 Byte 의 메모리 낭비가 일어나지만,
                                                                //  이는 그리 크리티컬하지 않다. 오히려 이렇게 하면 메모리 관리가 용이해지고
                                                                //  메모리 Buffer 을 한 개만 만들면 되기 때문에 유용하다.
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
    if (vkBeginCommandBuffer(vk.commandBuffer, &beginInfo) != VK_SUCCESS) {         // Encoding 시작
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    {
        vkCmdBindPipeline(vk.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vk.pipeline);
        vkCmdBindDescriptorSets(
            vk.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 
            vk.pipelineLayout, 0, 1, &vk.descriptorSet, 0, 0);

                                // 100 개의 Geometry 가 있고 100 개의 Record 가 있는 상황에서 충돌이 일어날 때, 
                                //  해당 Geometry 의 Record 로 어떻게 찾아 이동하는가?
                                //  => https://registry.khronos.org/vulkan/specs/latest/pdf/vkspec.pdf 의 40.3.1. Indexing Rules 참고
                                //      ( instanceShaderBindingTableRecordOffset + geometryIndex × sbtRecordStride + sbtRecordOffset ) 계산으로 얼마나 많은 Record 들을 건너뛸 지 결정된다.
                                //          geometryIndex 는 BLAS 에 저장되는 순서에 따른 Index 이고, sbtRecordStride 는 rayGen Shader 에서 명시된 sbtRecordStride 이다.
                                //          rayGen Shader 에 따르면 geometryIndex 칸만큼 이동하는 것이다.
                                //      Geometry 각각을 구분해야 하지만, Instance 각각도 구분할 수 있어야 한다.
                                //      Instance 끼리의 구분은 TLAS 생성할 때 instanceShaderBindingTableRecordOffset 를 각 Instance 마다 설정해준 것을 통해 이루어진다.

                                //      | Record 0 |    | Record 1 |
                                //
                                //      | Record 2 |    | Record 3 |

        vk.vkCmdTraceRaysKHR(       // Ray 를 처음 쏘는 부분 (Ray Shooting) (Rasterization 에서의 drawCall 에 해당)
            vk.commandBuffer,
            &vk.rgenSbt,
            &vk.missSbt,            // rayGen Shader 에 명시되어 있는 missIndex 를 보고 어느 miss Shader 을 사용할 지 결정한다.
            &vk.hitgSbt,
            &callSbt,
            WIDTH, HEIGHT, 1);          // depth 는 1 이니, WIDTH 와 HEIGHT 를 곱한 개수의 core 들 각각 raygen Shader 가 실행됨
        
        setImageLayout(
            vk.commandBuffer, 
            vk.outImage,                    // outImage dml Layout 은 VK_IMAGE_LAYOUT_GENERAL 인데,
                                            //  Raytracing Pipeline 에서 write 가 가능하게 하려면 Layout 이 VK_IMAGE_LAYOUT_GENERAL 이어야 한다.
            VK_IMAGE_LAYOUT_GENERAL,        // 이러한 VK_IMAGE_LAYOUT_GENERAL Layout 을 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL 로 바꿔주어야 한다.
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            subresourceRange);
            
        setImageLayout(
            vk.commandBuffer,
            vk.swapChainImages[imageIndex],     // swapChainImages[imageIndex] 의 Layout 이 VK_IMAGE_LAYOUT_UNDEFINED 로 간주하고 있으니
            VK_IMAGE_LAYOUT_UNDEFINED,          //  이를 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 로 바꿔야 한다.
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            subresourceRange);
        
        vkCmdCopyImage(                 // outImage (Ray tracing 결과) 를 Swap Chain Image 로 복사
            vk.commandBuffer,
            vk.outImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  // 복사 전 outImage 가 가지고 있을 Layout
            vk.swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,   // 복사 전 swapChainImages 가 가지고 있을 Layout
            1, &copyRegion);
                                                // 복사 후에는 다음 프레임을 위해 Layout 을 각각 원래대로 되돌려놔야 한다.
        setImageLayout(
            vk.commandBuffer,
            vk.outImage,                            // outImage 를 VK_IMAGE_LAYOUT_GENERAL 로 되돌려놓는다.
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL,
            subresourceRange);

        setImageLayout(
            vk.commandBuffer,
            vk.swapChainImages[imageIndex],         // swapChainImages[imageIndex] 를 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR 로 바꾼다.
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,   // 왜냐하면 이 다음에 swapChainImages[imageIndex] 로 Present 를 진행해야 하기 때문에
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,        //  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR Layout 이어야 한다.
            subresourceRange);                  // swapChainImages[imageIndex] 의 시작 Layout 과 끝 Layout 이 서로 다른데
                                                //  이는 render() 함수 초기에 실행되는 vkAcquireNextImageKHR 함수에서 VK_IMAGE_LAYOUT_UNDEFINED 로 초기화되어 괜찮기 때문이다.
                                                // 또한, 기존 Layout 에서 다른 Layout 로 바꾸는 과정에서 기존의 내용을 읽을 필요가 없으면 (기존의 Layout 으로 기존의 내용을 읽을 필요가 없는 경우)
                                                //  기존의 것을 VK_IMAGE_LAYOUT_UNDEFINED 로 간주하고 변환을 이어가도 무방하기 때문이기도 하다.
    }
    if (vkEndCommandBuffer(vk.commandBuffer) != VK_SUCCESS) {                   // Encoding 종료
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