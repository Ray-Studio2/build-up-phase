#define GLFW_INCLUDE_VULKAN
#include "glfw/glfw3.h"

#include "core/app.hpp"
#include "math/vec3.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <cstring>              // strcmp()
using std::cout, std::endl;
using std::ifstream;
using std::vector;
using std::tuple;

constexpr unsigned int WINDOW_WIDTH  = 1280;
constexpr unsigned int WINDOW_HEIGHT = 720;

// 현재 BLAS, TLAS 빌드 까지 완료
// TODO: SwapChain, Desc, Uniform Buffer, render함수 세팅
//       global buffer 정리, images, RT pipeline, shader, SBT

struct VKglobal {
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

    VkInstance instance;

    #ifdef DEBUG
        VkDebugUtilsMessengerEXT debugMessenger;
    #endif

    VkSurfaceKHR windowSurface;

    VkPhysicalDevice gpu;
    unsigned int graphicsQueueIdx;
    VkQueue graphicsQueue;
    VkDevice logicalDevice;

    VkSwapchainKHR swapChain;
    vector<VkImage> swapChainImages;
    vector<VkImageView> swapChainImageViews;

    vector<VkFramebuffer> frameBuffers;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore availableSemaphore;
    VkSemaphore renderDoneSemaphore;
    VkFence fence;

    tuple<VkBuffer, VkDeviceMemory> vertexBuffer;
    tuple<VkBuffer, VkDeviceMemory> indexBuffer;
    tuple<VkBuffer, VkDeviceMemory> geometryBuffer;
    tuple<VkBuffer, VkDeviceMemory> instanceBuffer;
    tuple<VkBuffer, VkDeviceMemory> blasBuffer;
    tuple<VkBuffer, VkDeviceMemory> tlasBuffer;

    VkAccelerationStructureKHR blas;
    VkAccelerationStructureKHR tlas;
    VkDeviceAddress blasAddress;
    VkDeviceAddress tlasAddress;

    VkDescriptorSetLayout uboDescSetLayout;
    VkDescriptorPool descPool;
    VkDescriptorSet uboDescSet;

    const VkFormat swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    const VkColorSpaceKHR swapChainImageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    const VkExtent2D swapChainImageExtent = {
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT
    };

    ~VKglobal() noexcept {
        vkDestroyFence(logicalDevice, fence, nullptr);
        vkDestroySemaphore(logicalDevice, availableSemaphore, nullptr);
        vkDestroySemaphore(logicalDevice, renderDoneSemaphore, nullptr);

        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

        vkDestroyAccelerationStructureKHR(logicalDevice, tlas, nullptr);
        vkDestroyAccelerationStructureKHR(logicalDevice, blas, nullptr);

        vkFreeMemory(logicalDevice, std::get<1>(tlasBuffer), nullptr);
        vkFreeMemory(logicalDevice, std::get<1>(blasBuffer), nullptr);
        vkFreeMemory(logicalDevice, std::get<1>(instanceBuffer), nullptr);
        vkFreeMemory(logicalDevice, std::get<1>(geometryBuffer), nullptr);
        vkFreeMemory(logicalDevice, std::get<1>(indexBuffer), nullptr);
        vkFreeMemory(logicalDevice, std::get<1>(vertexBuffer), nullptr);
        vkDestroyBuffer(logicalDevice, std::get<0>(tlasBuffer), nullptr);
        vkDestroyBuffer(logicalDevice, std::get<0>(blasBuffer), nullptr);
        vkDestroyBuffer(logicalDevice, std::get<0>(instanceBuffer), nullptr);
        vkDestroyBuffer(logicalDevice, std::get<0>(geometryBuffer), nullptr);
        vkDestroyBuffer(logicalDevice, std::get<0>(indexBuffer), nullptr);
        vkDestroyBuffer(logicalDevice, std::get<0>(vertexBuffer), nullptr);

        vkDestroyDescriptorPool(logicalDevice, descPool, nullptr);
        vkDestroyDescriptorSetLayout(logicalDevice, uboDescSetLayout, nullptr);

        vkDestroyPipeline(logicalDevice, pipeline, nullptr);
        vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);

        for (auto frameBuffer: frameBuffers)
            vkDestroyFramebuffer(logicalDevice, frameBuffer, nullptr);

        for (auto imageView: swapChainImageViews)
            vkDestroyImageView(logicalDevice, imageView, nullptr);

        vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);

        vkDestroyDevice(logicalDevice, nullptr);
        vkDestroySurfaceKHR(instance, windowSurface, nullptr);

        #ifdef DEBUG
            ((PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"))(instance, debugMessenger, nullptr);
        #endif

        vkDestroyInstance(instance, nullptr);
    }
} vkGlobal;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
          VkDebugUtilsMessageSeverityFlagBitsEXT severity    , VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT*  callbackData, void*                           userData
) {
    const char* debugMSG{ };

    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
            debugMSG = "[Verbose]";
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
            debugMSG = "[Warning]";
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
            debugMSG = "[Error]";
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
            debugMSG = "[Info]";
            break;
        }

        default:
            debugMSG = "[Unknown]";
    }

    const char* types{ };

    switch (type) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: {
            types = "[General]";
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: {
            types = "[Performance]";
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: {
            types = "[Validation]";
            break;
        }

        default:
            types = "[Unknown]";
    }

    cout << "[Debug]" << severity << types << callbackData->pMessage << endl;
    return VK_FALSE;
}

void loadDeviceExtensionFunctions() {
    // anonymous namespace 내부에 함수들 넣어두면 vkGlobal. 붙이지 않고 쓸 수 있나?/
    // ::vkGetBufferDeviceAddressKHR 처럼

    vkGlobal.vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)(vkGetDeviceProcAddr(vkGlobal.logicalDevice, "vkGetBufferDeviceAddressKHR"));
    vkGlobal.vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)(vkGetDeviceProcAddr(vkGlobal.logicalDevice, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkGlobal.vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)(vkGetDeviceProcAddr(vkGlobal.logicalDevice, "vkCreateAccelerationStructureKHR"));
    vkGlobal.vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)(vkGetDeviceProcAddr(vkGlobal.logicalDevice, "vkDestroyAccelerationStructureKHR"));
    vkGlobal.vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)(vkGetDeviceProcAddr(vkGlobal.logicalDevice, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGlobal.vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)(vkGetDeviceProcAddr(vkGlobal.logicalDevice, "vkCmdBuildAccelerationStructuresKHR"));
	vkGlobal.vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)(vkGetDeviceProcAddr(vkGlobal.logicalDevice, "vkCreateRayTracingPipelinesKHR"));
	vkGlobal.vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)(vkGetDeviceProcAddr(vkGlobal.logicalDevice, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkGlobal.vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)(vkGetDeviceProcAddr(vkGlobal.logicalDevice, "vkCmdTraceRaysKHR"));
}
inline VkDeviceAddress getDeviceAddress(VkBuffer buf) {
    VkBufferDeviceAddressInfoKHR deviceAddressInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buf
    };

    return vkGlobal.vkGetBufferDeviceAddressKHR(vkGlobal.logicalDevice, &deviceAddressInfo);
}
inline VkDeviceAddress getDeviceAddress(VkAccelerationStructureKHR as) {
    VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = as
    };

    return vkGlobal.vkGetAccelerationStructureDeviceAddressKHR(vkGlobal.logicalDevice, &deviceAddressInfo);
}

bool isExtensionSupport(const VkPhysicalDevice& device, const vector<const char*>& extensions) {
    unsigned int deviceExtensionCount{ };
    vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);

    vector<VkExtensionProperties> supportExtensions(deviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, supportExtensions.data());

    int checkCount{ };
    for (const auto& needExtension: extensions) {
        for (const auto& support: supportExtensions) {
            if (strcmp(needExtension, support.extensionName) == 0)
                ++checkCount;

            if (checkCount == extensions.size())
                return true;
        }
    }

    return false;
}
vector<char> readFile(const char* filePath) {
    ifstream reader(filePath, std::ios::binary | std::ios::ate);
    vector<char> fileData;

    if (reader) {
        unsigned long long fileSize = static_cast<unsigned long long>(reader.tellg());
        fileData.resize(fileSize);

        reader.seekg(std::ios::beg);
        reader.read(fileData.data(), fileSize);
        reader.close();
    }
    else
        cout << "[ERROR]: Failed to open file \"" << filePath << '\"' << endl;

    return fileData;
}
VkShaderModule makeShaderModule(const char* filePath) {
    VkShaderModule shaderModule{ };

    vector<char> spvFile = readFile(filePath);
    if (!spvFile.empty()) {
        VkShaderModuleCreateInfo shaderModuleCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = spvFile.size(),
            .pCode = reinterpret_cast<const unsigned int*>(spvFile.data())
        };

        if (vkCreateShaderModule(vkGlobal.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
            cout << "[ERROR]: vkCreateShaderModule()" << endl;
    }

    return shaderModule;
}
tuple<VkBuffer, VkDeviceMemory> createBuffer(const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& reqMemFlags) {
    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage
    };

    VkBuffer buffer{ };
    if (vkCreateBuffer(vkGlobal.logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateBuffer()" << endl;

    unsigned int memIdx{ };
    VkMemoryRequirements memRequirements;
    {
        vkGetBufferMemoryRequirements(vkGlobal.logicalDevice, buffer, &memRequirements);

        VkPhysicalDeviceMemoryProperties gpuMemSpec;
        vkGetPhysicalDeviceMemoryProperties(vkGlobal.gpu, &gpuMemSpec);

        for (; memIdx < gpuMemSpec.memoryTypeCount; ++memIdx) {
            if ((memRequirements.memoryTypeBits) && ((reqMemFlags & gpuMemSpec.memoryTypes[memIdx].propertyFlags) == reqMemFlags))
                break;
        }
    }

    VkMemoryAllocateInfo memAllocInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memIdx
    };

    // VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT 가 켜져 있는 경우
    // 이 설정을 해주는 것으로 GPU가 버퍼를 찾을 수 있음
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkMemoryAllocateFlagsInfo memAllocFlagsInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
            .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR
        };

        // Next Chain 연결
        memAllocInfo.pNext = &memAllocFlagsInfo;
    }

    VkDeviceMemory memory{ };
    if (vkAllocateMemory(vkGlobal.logicalDevice, &memAllocInfo, nullptr, &memory) != VK_SUCCESS)
        cout << "[ERROR]: vkAllocateMemory()" << endl;

    vkBindBufferMemory(vkGlobal.logicalDevice, buffer, memory, 0);

    return { buffer, memory };
}

// Depth Image 코드 잔재
// 후속 강의에 Image 생성 파트가 있는것 같아서 일단 남겨둠
tuple<VkImage, VkDeviceMemory> createImage(
    const VkFormat&          format, const VkExtent3D&                 extent, const VkImageTiling& tiling,
    const VkImageUsageFlags& usage , const VkMemoryPropertyFlags& reqMemFlags
) {
    VkImageCreateInfo imageInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage
    };

    VkImage image{ };
    if (vkCreateImage(vkGlobal.logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateBuffer()" << endl;

    unsigned int memIdx{ };
    VkMemoryRequirements memRequirements;
    {
        // 이미지에 필요한 메모리 요구사항 탐색
        vkGetImageMemoryRequirements(vkGlobal.logicalDevice, image, &memRequirements);

        // GPU상에 지원되는 메모리 정보 탐색
        VkPhysicalDeviceMemoryProperties gpuMemSpec;
        vkGetPhysicalDeviceMemoryProperties(vkGlobal.gpu, &gpuMemSpec);

        for (; memIdx < gpuMemSpec.memoryTypeCount; ++memIdx) {
            if ((memRequirements.memoryTypeBits) && ((reqMemFlags & gpuMemSpec.memoryTypes[memIdx].propertyFlags) == reqMemFlags))
                break;
        }
    }

    // 메모리 요구 사항에 맞추어 allocate
    VkMemoryAllocateInfo memAllocInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memIdx
    };

    VkDeviceMemory memory{ };
    if (vkAllocateMemory(vkGlobal.logicalDevice, &memAllocInfo, nullptr, &memory) != VK_SUCCESS)
        cout << "[ERROR]: vkAllocateMemory()" << endl;

    vkBindImageMemory(vkGlobal.logicalDevice, image, memory, 0);

    return { image, memory };
}
VkImageView createImageView(const VkImage& image, const VkFormat& format, const VkImageSubresourceRange& resourceRange) {
    VkImageViewCreateInfo imageViewInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = resourceRange
        };

    VkImageView imageView;
    if (vkCreateImageView(vkGlobal.logicalDevice, &imageViewInfo, nullptr, &imageView) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateImageView()" << endl;

    return imageView;
}

void createInstance(GLFWwindow* window) {
    VkApplicationInfo appInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .apiVersion = VK_API_VERSION_1_0
    };

    unsigned int glfwExtCount{ };
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtCount);

    #ifdef DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        extensions.shrink_to_fit();

        vector<const char*> validationLayers;
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback
        };
    #endif

    VkInstanceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,

        #ifdef DEBUG
            .pNext = &debugCreateInfo,
        #endif

        .pApplicationInfo = &appInfo,

        #ifdef DEBUG
            .enabledLayerCount = static_cast<unsigned int>(validationLayers.size()),
            .ppEnabledLayerNames = validationLayers.data(),
        #endif

        .enabledExtensionCount = static_cast<unsigned int>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    if (vkCreateInstance(&createInfo, nullptr, &vkGlobal.instance) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateInstance()" << endl;

    #ifdef DEBUG
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkGlobal.instance, "vkCreateDebugUtilsMessengerEXT");

        if (func == nullptr or func(vkGlobal.instance, &debugCreateInfo, nullptr, &vkGlobal.debugMessenger) != VK_SUCCESS)
            cout << "[ERROR]: Debug Messenger Setup" << endl;
    #endif

    if (glfwCreateWindowSurface(vkGlobal.instance, window, nullptr, &vkGlobal.windowSurface) != VK_SUCCESS)
        cout << "[ERROR]: glfwCreateWindowSurface()" << endl;
}
void createDevice() {
    unsigned int physicalDeviceCount{ };
    vkEnumeratePhysicalDevices(vkGlobal.instance, &physicalDeviceCount, nullptr);

    vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(vkGlobal.instance, &physicalDeviceCount, physicalDevices.data());

    vector<const char*> needExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME
    };

    for (const auto& physicalDevice: physicalDevices) {
        if (isExtensionSupport(physicalDevice, needExtensions)) {
            vkGlobal.gpu = physicalDevice;
            break;
        }
    }

    unsigned int queueFamilyCount{ };
    vkGetPhysicalDeviceQueueFamilyProperties(vkGlobal.gpu, &queueFamilyCount, nullptr);

    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vkGlobal.gpu, &queueFamilyCount, queueFamilies.data());

    while (vkGlobal.graphicsQueueIdx < queueFamilyCount) {
        VkBool32 presentSupport{ false };
        vkGetPhysicalDeviceSurfaceSupportKHR(vkGlobal.gpu, vkGlobal.graphicsQueueIdx, vkGlobal.windowSurface, &presentSupport);

        if ((queueFamilies[vkGlobal.graphicsQueueIdx].queueFlags & VK_QUEUE_GRAPHICS_BIT) and presentSupport)
            break;

        ++vkGlobal.graphicsQueueIdx;
    }

    float queuePriorities = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vkGlobal.graphicsQueueIdx,
        .queueCount = 1,
        .pQueuePriorities = &queuePriorities
    };

    VkDeviceCreateInfo logicalDeviceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = static_cast<unsigned int>(needExtensions.size()),
        .ppEnabledExtensionNames = needExtensions.data()
    };

    // AS 에서 필요한 feature enable
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .bufferDeviceAddress = VK_TRUE
    };
    VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .accelerationStructure = VK_TRUE
    };

    logicalDeviceCreateInfo.pNext = &bufferDeviceAddressFeatures;
    asFeatures.pNext = &asFeatures;

    if (vkCreateDevice(vkGlobal.gpu, &logicalDeviceCreateInfo, nullptr, &vkGlobal.logicalDevice) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateDevice()" << endl;

    vkGetDeviceQueue(vkGlobal.logicalDevice, vkGlobal.graphicsQueueIdx, 0, &vkGlobal.graphicsQueue);
}
void createSwapChain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkGlobal.gpu, vkGlobal.windowSurface, &capabilities);

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    {
        unsigned int presentModeCount{ };
        vkGetPhysicalDeviceSurfacePresentModesKHR(vkGlobal.gpu, vkGlobal.windowSurface, &presentModeCount, nullptr);

        vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(vkGlobal.gpu, vkGlobal.windowSurface, &presentModeCount, presentModes.data());

        for (const auto& mode: presentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vkGlobal.windowSurface,
        .minImageCount = capabilities.minImageCount + 1,
        .imageFormat = vkGlobal.swapChainImageFormat,
        .imageColorSpace = vkGlobal.swapChainImageColorSpace,
        .imageExtent = vkGlobal.swapChainImageExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE
    };

    if (vkCreateSwapchainKHR(vkGlobal.logicalDevice, &swapChainCreateInfo, nullptr, &vkGlobal.swapChain) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateSwapchainKHR()" << endl;

    unsigned int swapChainImageCount{ };
    vkGetSwapchainImagesKHR(vkGlobal.logicalDevice, vkGlobal.swapChain, &swapChainImageCount, nullptr);

    vkGlobal.swapChainImages.resize(swapChainImageCount);
    vkGetSwapchainImagesKHR(vkGlobal.logicalDevice, vkGlobal.swapChain, &swapChainImageCount, vkGlobal.swapChainImages.data());

    for (const auto& image: vkGlobal.swapChainImages) {
        VkImageViewCreateInfo imageViewInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vkGlobal.swapChainImageFormat,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        };

        VkImageView imageView;
        if (vkCreateImageView(vkGlobal.logicalDevice, &imageViewInfo, nullptr, &imageView) != VK_SUCCESS)
            cout << "[ERROR]: vkCreateImageView()" << endl;

        vkGlobal.swapChainImageViews.push_back(imageView);
    }
}
void createDescSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    };

    VkDescriptorSetLayoutCreateInfo uboDescLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding
    };

    if (vkCreateDescriptorSetLayout(vkGlobal.logicalDevice, &uboDescLayoutCreateInfo, nullptr, &vkGlobal.uboDescSetLayout) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateDescriptorSetLayout()" << endl;
}
void createCommandBuffers() {
    VkCommandPoolCreateInfo commandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vkGlobal.graphicsQueueIdx
    };

    if (vkCreateCommandPool(vkGlobal.logicalDevice, &commandPoolCreateInfo, nullptr, &vkGlobal.commandPool) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateCommandPool()" << endl;

    VkCommandBufferAllocateInfo commandBufferAllocInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vkGlobal.commandPool,
        .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(vkGlobal.logicalDevice, &commandBufferAllocInfo, &vkGlobal.commandBuffer) != VK_SUCCESS)
        cout << "[ERROR]: vkAllocateCommandBuffers()" << endl;
}
void createDescPool() {
    VkDescriptorPoolSize descPoolSize {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1
    };

    VkDescriptorPoolCreateInfo descPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &descPoolSize
    };

    if (vkCreateDescriptorPool(vkGlobal.logicalDevice, &descPoolCreateInfo, nullptr, &vkGlobal.descPool))
        cout << "[ERROR]: vkCreateDescriptorPool()" << endl;
}
void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreCreateInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    if ((vkCreateSemaphore(vkGlobal.logicalDevice, &semaphoreCreateInfo, nullptr, &vkGlobal.availableSemaphore) != VK_SUCCESS) or
        (vkCreateSemaphore(vkGlobal.logicalDevice, &semaphoreCreateInfo, nullptr, &vkGlobal.renderDoneSemaphore) != VK_SUCCESS) or
        (vkCreateFence(vkGlobal.logicalDevice, &fenceCreateInfo, nullptr, &vkGlobal.fence)))
        cout << "[ERROR]: createSyncObjects()" << endl;
}

// Ray와 Scene의 교차 테스트를 수행하는데, BLAS, TLAS 생성으로 가속 작업 Scene을 만든 것 (BVH 구조를 생성한 것)
void createBLAS() {
    /* Clowk-Wise
        0 ---- 1
        |      |
        |      |
        3 ---- 2
    */
    Vec3 vertices[] = {
        { -1.0f, -1.0f, 0.0f },
        {  1.0f, -1.0f, 0.0f },
        {  1.0f,  1.0f, 0.0f },
        { -1.0f,  1.0f, 0.0f }
    };
    unsigned int indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    // 두 개를 사용하는 이유는 사각형을 두 개를 그리기 위함
    VkTransformMatrixKHR transforms[] = {
        {   // 좌측에 그려질 사각형에 사용
            1.0f, 0.0f, 0.0f, -2.0f,
            0.0f, 1.0f, 0.0f,  0.0f,
            0.0f, 0.0f, 1.0f,  0.0f
        },
        {   // 우측에 그려질 사각형에 사용
            1.0f, 0.0f, 0.0f, 2.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        }
    };

    unsigned int verticesSize = sizeof(vertices);
    unsigned int indicesSize = sizeof(indices);
    unsigned int transformsSize = sizeof(transforms);

    // BLAS의 input이 되는 대상 버퍼들 생성
    // AS 생성시 (빌드할 때) GPU 내부에서 빌드하는 프로그램이 있다.
    // 그 빌드 프로그램에서 우리가 생성한 버퍼를 참조해야 하는데, 그 버퍼의 주소를 알려줘야 하므로 USAGE 수정
    // VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR 는 AS의 INPUT으로 사용한다는 의미
    auto& [vBuffer, vMemory] = vkGlobal.vertexBuffer;
    std::tie(vBuffer, vMemory) = createBuffer(
        verticesSize,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    auto& [iBuffer, iMemory] = vkGlobal.indexBuffer;
    std::tie(iBuffer, iMemory) = createBuffer(
        indicesSize,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    auto& [gBuffer, gMemory] = vkGlobal.geometryBuffer;
    std::tie(gBuffer, gMemory) = createBuffer(
        transformsSize,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    void* dst{ };

    vkMapMemory(vkGlobal.logicalDevice, vMemory, 0, verticesSize, 0, &dst);
    memcpy(dst, vertices, verticesSize);
    vkUnmapMemory(vkGlobal.logicalDevice, vMemory);

    vkMapMemory(vkGlobal.logicalDevice, iMemory, 0, indicesSize, 0, &dst);
    memcpy(dst, indices, indicesSize);
    vkUnmapMemory(vkGlobal.logicalDevice, iMemory);

    vkMapMemory(vkGlobal.logicalDevice, gMemory, 0, transformsSize, 0, &dst);
    memcpy(dst, transforms, transformsSize);
    vkUnmapMemory(vkGlobal.logicalDevice, gMemory);

    // BLAS 생성 시작
    // geometry type BLAS: VK_GEOMETRY_TYPE_TRIANGLES_KHR
    // geometry type TLAS: VK_GEOMETRY_TYPE_INSTANCES_KHR
    VkAccelerationStructureGeometryKHR geometry0 {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,

        // maxVertex: 최대 index 번호 지정
        .geometry = {
            .triangles = {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
                .vertexData = { .deviceAddress = getDeviceAddress(vBuffer) },
                .vertexStride = sizeof(vertices[0]),
                .maxVertex = static_cast<unsigned int>((verticesSize / sizeof(vertices[0])) - 1),
                .indexType = VK_INDEX_TYPE_UINT32,
                .indexData = { .deviceAddress = getDeviceAddress(iBuffer) },
                .transformData = { .deviceAddress = getDeviceAddress(gBuffer) }
            }
        },
        // 불투명한 물체이므로 OPAQUE
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR
    };

    VkAccelerationStructureGeometryKHR geometries[] = { geometry0, geometry0 };
    unsigned int triangleCounts[] = { 2, 2 };

    // BLAS 빌드시 필요한 용량을 얻기 위한 정보 세팅
    VkAccelerationStructureBuildGeometryInfoKHR blasBuildInfo {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount = (sizeof(geometries) / sizeof(geometries[0])),
        .pGeometries = geometries
    };
    VkAccelerationStructureBuildSizesInfoKHR blasBuildSizeInfo { .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };

    // BLAS 빌드에 필요한 용량 획득
    // AS Build Device를 GPU로 지정 (Vulkan에서는 CPU에서도 가능, DX12는 GPU만 가능)
    vkGlobal.vkGetAccelerationStructureBuildSizesKHR(
        vkGlobal.logicalDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &blasBuildInfo, triangleCounts, &blasBuildSizeInfo
    );

    // BLAS가 저장 된 실제 버퍼의 사이즈 = blasBuildSizeInfo.accelerationStructureSize
    // GPU에서 빌드되므로 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    auto& [blasBuffer, blasMemory] = vkGlobal.blasBuffer;
    std::tie(blasBuffer, blasMemory) = createBuffer(
        blasBuildSizeInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    // 빌드 과정에 추가적으로 필요한 용량 확보를 위한 버퍼
    // GPU에서 빌드되므로 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    // 빌드가 끝나면 필요 없으므로 local 선언
    auto [scratchBuffer, scratchMemory] = createBuffer(
        blasBuildSizeInfo.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    // BLAS에 대한 핸들과, 실제 BLAS가 저장되어 있는 곳 (버퍼) 이 존재
    // 이 단계에서 BLAS의 핸들 획득
    VkAccelerationStructureCreateInfoKHR asInfo {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = std::get<0>(vkGlobal.blasBuffer),
        .size = blasBuildSizeInfo.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
    };
    // BLAS 핸들 생성 및 해당 핸들값 저장
    // BLAS 핸들 값은 TLAS에서 사용됨
    vkGlobal.vkCreateAccelerationStructureKHR(vkGlobal.logicalDevice, &asInfo, nullptr, &vkGlobal.blas);
    vkGlobal.blasAddress = getDeviceAddress(vkGlobal.blas);

    // BLAS build
    // GPU상에서 이루어지므로, Command Buffer 사용
    VkCommandBufferBeginInfo commandBufferBeginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(vkGlobal.commandBuffer, &commandBufferBeginInfo);
    {
        blasBuildInfo.dstAccelerationStructure = vkGlobal.blas;
        blasBuildInfo.scratchData.deviceAddress = getDeviceAddress(scratchBuffer);

        // blasBuildInfo 상에 존재하는 geometry가 2개였으므로 그에 대한 명시
        // 이전에 사각형 한 개만을 정의했고, indices도 한 개만 존재
        // 따라서, primitiveOffset은 지정해주지 않아도 됨
        VkAccelerationStructureBuildRangeInfoKHR blasBuildRangeInfo[] = {
            {   // 첫 번째 geometry의 삼각형 수
                .primitiveCount = triangleCounts[0],
                .transformOffset = 0
            },
            {   // 두 번째 geometry의 삼각형 수
                .primitiveCount = triangleCounts[0],
                .transformOffset = sizeof(transforms[0])
            }
        };

        VkAccelerationStructureBuildGeometryInfoKHR blasBuildInfos[] = { blasBuildInfo };
        VkAccelerationStructureBuildRangeInfoKHR* blasBuildRangeInfos[] = { blasBuildRangeInfo };

        // 여러 개의 BLAS를 한 번의 호출로 동시에 생성할 수 있다는 점 인지
        vkGlobal.vkCmdBuildAccelerationStructuresKHR(vkGlobal.commandBuffer, 1, blasBuildInfos, blasBuildRangeInfos);
    }
    vkEndCommandBuffer(vkGlobal.commandBuffer);

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkGlobal.commandBuffer,
    };
    vkQueueSubmit(vkGlobal.graphicsQueue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(vkGlobal.graphicsQueue);

    vkFreeMemory(vkGlobal.logicalDevice, scratchMemory, nullptr);
    vkDestroyBuffer(vkGlobal.logicalDevice, scratchBuffer, nullptr);
}
void createTLAS() {
    // BLAS에서 그랬던 것 처럼, 두 개의 instance 생성
    VkTransformMatrixKHR transforms[] = {
        {   // 첫 번째 instance에 적용 될 matrix
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 2.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        },
        {   // 첫 번째 instance에 적용 될 matrix
            1.0f, 0.0f, 0.0f,  0.0f,
            0.0f, 1.0f, 0.0f, -2.0f,
            0.0f, 0.0f, 1.0f,  0.0f
        },
    };

    // Back-face Culling Disable
    // 앞면인지 뒷면인지 상관하지 않고 삼각형끼리 충돌 시, shader 호출
    // instance는 BLAS 를 참조
    VkAccelerationStructureInstanceKHR instance0 {
        .mask = 0xFF,
        .instanceShaderBindingTableRecordOffset = 0,
        .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        .accelerationStructureReference = vkGlobal.blasAddress
    };

    VkAccelerationStructureInstanceKHR instances[] = { instance0, instance0 };
    instances[0].transform = transforms[0];
    instances[1].transform = transforms[1];

    unsigned int instancesSize = sizeof(instances);
    unsigned int instanceCount = 2;

    // TLAS의 input이 되는 대상 버퍼들 생성
    auto& [instanceBuffer, instanceMemory] = vkGlobal.instanceBuffer;
    std::tie(instanceBuffer, instanceMemory) = createBuffer(
        instancesSize,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    void* dst{ };

    vkMapMemory(vkGlobal.logicalDevice, instanceMemory, 0, instancesSize, 0, &dst);
    memcpy(dst, instances, instancesSize);
    vkUnmapMemory(vkGlobal.logicalDevice, instanceMemory);

    // TLAS 생성 시작
    // geometry type BLAS: VK_GEOMETRY_TYPE_TRIANGLES_KHR
    // geometry type TLAS: VK_GEOMETRY_TYPE_INSTANCES_KHR
    VkAccelerationStructureGeometryKHR instanceData {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry = {
            .instances = {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                .data = { .deviceAddress = getDeviceAddress(instanceBuffer) }
            }
        },
        // 불투명한 물체이므로 OPAQUE
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR
    };

    // TLAS 빌드시 필요한 용량을 얻기 위한 정보 세팅
    // BLAS는 하나 이상의 geometry로 구성되어 geometry Count가 1이 아닐 수 있음
    // TLAS는 여러 개의 instance로 이루어지고, 여러 개의 instance가 하나의 geometry로 구성
    // vulkan spec 에서도 geometryCount는 1 이어야 한다고 명시되어 있음
    VkAccelerationStructureBuildGeometryInfoKHR tlasBuildInfo {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount = 1,
        .pGeometries = &instanceData
    };
    VkAccelerationStructureBuildSizesInfoKHR tlasBuildSizeInfo { .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };

    // TLAS 빌드에 필요한 용량 획득
    // AS Build Device를 GPU로 지정 (Vulkan에서는 CPU에서도 가능, DX12는 GPU만 가능)
    vkGlobal.vkGetAccelerationStructureBuildSizesKHR(
        vkGlobal.logicalDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &tlasBuildInfo, &instanceCount, &tlasBuildSizeInfo
    );

    // TLAS가 저장 된 실제 버퍼의 사이즈 = blasBuildSizeInfo.accelerationStructureSize
    // GPU에서 빌드되므로 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    auto& [tlasBuffer, tlasMemory] = vkGlobal.tlasBuffer;
    std::tie(tlasBuffer, tlasMemory) = createBuffer(
        tlasBuildSizeInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    // 빌드 과정에 추가적으로 필요한 용량 확보를 위한 버퍼
    // GPU에서 빌드되므로 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    // 빌드가 끝나면 필요 없으므로 local 선언
    auto [scratchBuffer, scratchMemory] = createBuffer(
        tlasBuildSizeInfo.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    // TLAS에 대한 핸들과, 실제 TLAS가 저장되어 있는 곳 (버퍼) 이 존재
    // 이 단계에서 TLAS의 핸들 획득
    VkAccelerationStructureCreateInfoKHR asInfo {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = std::get<0>(vkGlobal.tlasBuffer),
        .size = tlasBuildSizeInfo.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
    };
    // TLAS 핸들 생성 및 해당 핸들값 저장
    vkGlobal.vkCreateAccelerationStructureKHR(vkGlobal.logicalDevice, &asInfo, nullptr, &vkGlobal.tlas);
    vkGlobal.blasAddress = getDeviceAddress(vkGlobal.tlas);

    // TLAS build
    // GPU상에서 이루어지므로, Command Buffer 사용
    VkCommandBufferBeginInfo commandBufferBeginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(vkGlobal.commandBuffer, &commandBufferBeginInfo);
    {
        tlasBuildInfo.dstAccelerationStructure = vkGlobal.tlas;
        tlasBuildInfo.scratchData.deviceAddress = getDeviceAddress(scratchBuffer);

        VkAccelerationStructureBuildRangeInfoKHR tlasBuildRangeInfo = { .primitiveCount = instanceCount };
        VkAccelerationStructureBuildRangeInfoKHR* tlasBuildRangeInfos[] = { &tlasBuildRangeInfo };

        // 여러 개의 TLAS를 한 번의 호출로 동시에 생성할 수 있다는 점 인지
        vkGlobal.vkCmdBuildAccelerationStructuresKHR(vkGlobal.commandBuffer, 1, &tlasBuildInfo, tlasBuildRangeInfos);
    }
    vkEndCommandBuffer(vkGlobal.commandBuffer);

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkGlobal.commandBuffer,
    };
    vkQueueSubmit(vkGlobal.graphicsQueue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(vkGlobal.graphicsQueue);

    vkFreeMemory(vkGlobal.logicalDevice, scratchMemory, nullptr);
    vkDestroyBuffer(vkGlobal.logicalDevice, scratchBuffer, nullptr);
}

void render() {
    const VkCommandBufferBeginInfo commandBufferBeginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    vkWaitForFences(vkGlobal.logicalDevice, 1, &vkGlobal.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkGlobal.logicalDevice, 1, &vkGlobal.fence);

    unsigned int imgIndex{ };
    vkAcquireNextImageKHR(vkGlobal.logicalDevice, vkGlobal.swapChain, UINT64_MAX, vkGlobal.availableSemaphore, nullptr, &imgIndex);

    vkResetCommandBuffer(vkGlobal.commandBuffer, 0);
    if (vkBeginCommandBuffer(vkGlobal.commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
        cout << "[ERROR]: vkBeginCommandBuffer()" << endl;
    {
        vkCmdBindPipeline(vkGlobal.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vkGlobal.pipeline);
        //
    }
    if (vkEndCommandBuffer(vkGlobal.commandBuffer) != VK_SUCCESS)
        cout << "[ERROR]: vkEndCommandBuffer()" << endl;

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vkGlobal.availableSemaphore,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkGlobal.commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &vkGlobal.renderDoneSemaphore
    };

    if (vkQueueSubmit(vkGlobal.graphicsQueue, 1, &submitInfo, vkGlobal.fence) != VK_SUCCESS)
        cout << "[ERROR]: vkQueueSubmit()" << endl;

    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vkGlobal.renderDoneSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &vkGlobal.swapChain,
        .pImageIndices = &imgIndex
    };

    vkQueuePresentKHR(vkGlobal.graphicsQueue, &presentInfo);
}

int main() {
    App::init(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Raytracing");

    createInstance(App::activeWindow()->handle());
    createDevice();
    loadDeviceExtensionFunctions();
    createSwapChain();
    createDescSetLayout();
    createCommandBuffers();
    createDescPool();
    createSyncObjects();

    createBLAS();
    createTLAS();

    // render loop
    while (!glfwWindowShouldClose(App::activeWindow()->handle())) {
        glfwPollEvents();
        render();
    }

    vkDeviceWaitIdle(vkGlobal.logicalDevice);
    glfwTerminate();
    return 0;
}