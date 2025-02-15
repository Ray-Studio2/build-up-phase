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

#ifdef NDEBUG
    const bool ON_DEBUG = false;
#else
    const bool ON_DEBUG = true;
#endif


struct Global {             // 이 아래의 6개 함수들 모두 라이브러리에 선언이 되어 있기에 중복선언을 방지하기 위해 이렇게 struct 안에 위치시켰다.
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;                    // Extension 에 해당하는 함수들 ( 드라이버에서 가져온다. )
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue graphicsQueue; // assume allowing graphics and present
    uint queueFamilyIndex;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    const VkFormat swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;    // intentionally chosen to match a specific format
    const VkExtent2D swapChainImageExtent = { .width = WIDTH, .height = HEIGHT };

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence fence0;

    VkBuffer blasBuffer;
    VkDeviceMemory blasBufferMem;
    VkAccelerationStructureKHR blas;
    VkDeviceAddress blasAddress;

    VkBuffer tlasBuffer;
    VkDeviceMemory tlasBufferMem;
    VkAccelerationStructureKHR tlas;
    VkDeviceAddress tlasAddress;

    ~Global() {
        vkDestroyBuffer(device, tlasBuffer, nullptr);
        vkFreeMemory(device, tlasBufferMem, nullptr);
        vkDestroyAccelerationStructureKHR(device, tlas, nullptr);

        vkDestroyBuffer(device, blasBuffer, nullptr);
        vkFreeMemory(device, blasBufferMem, nullptr);
        vkDestroyAccelerationStructureKHR(device, blas, nullptr);

        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroyFence(device, fence0, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
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

bool checkValidationLayerSupport(std::vector<const char*>& reqestNames) {
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

bool checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*>& reqestNames) {
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
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,            // 해당 extension 을 추가해서 이를 지원하는 device 를 찾는다고 바로 사용할 수 있는 것은 아니다.
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,         //  이후에, extension 에 대한 상세한 설정이 반드시 필요하다.
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    };

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

	VkPhysicalDeviceAccelerationStructureFeaturesKHR f1{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .accelerationStructure = VK_TRUE,       // VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME extension 에 대한 상세한 설정이 필요한데, 
    };                                          //  , accelerationStructure 을 true 로 설정해야 비로소 해당 extension 을 사용할 수 있다.

	VkPhysicalDeviceBufferDeviceAddressFeatures f2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .bufferDeviceAddress = VK_TRUE,         // VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME extension 또한 그렇다.
    };

    createInfo.pNext = &f1;         // createInfo 의 Next Chain 으로 넣어주어야 한다. ( Linked List 자료구조 ) 
    f1.pNext = &f2;                 //  f2 또한 그렇다. ( 이 다음은 당연히 null 이다. )

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
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
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

    for (const auto& image : vk.swapChainImages) {
        VkImageViewCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk.swapChainImageFormat,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        VkImageView imageView;
        if (vkCreateImageView(vk.device, &createInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
        vk.swapChainImageViews.push_back(imageView);
    }
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

    uint memTypeIndex = 0;
    {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(vk.device, buffer, &memRequirements);
        size = memRequirements.size;
        std::bitset<32> isSuppoted(memRequirements.memoryTypeBits);

        VkPhysicalDeviceMemoryProperties spec;
        vkGetPhysicalDeviceMemoryProperties(vk.physicalDevice, &spec);

        for (auto& [props, _] : std::span<VkMemoryType>(spec.memoryTypes, spec.memoryTypeCount)) {
            if (isSuppoted[memTypeIndex] && (props & reqMemProps) == reqMemProps) {
                break;
            }
            ++memTypeIndex;
        }
    }

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = size,
        .memoryTypeIndex = memTypeIndex,
    };

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkMemoryAllocateFlagsInfo flagsInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
            .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,     // 차후 createBuffer 함수에서 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT flag 적용을 위해
        };                                                          //  이렇게 flagsInfo 를 만들고 allocInfo 의 Next Chain 으로 지정해준다.
        allocInfo.pNext = &flagsInfo;
    }

    if (vkAllocateMemory(vk.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(vk.device, buffer, bufferMemory, 0);

    return { buffer, bufferMemory };
}

inline VkDeviceAddress getDeviceAddressOf(VkBuffer buffer)                  // Extension 함수인 vkGetBufferDeviceAddressKHR 를 통해
{                                                                           //  buffer 의 Device 주소 (GPU 상에 할당된 메모리 주소) 를 가져올 수 있다.
    VkBufferDeviceAddressInfoKHR info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer,
    };
    return vk.vkGetBufferDeviceAddressKHR(vk.device, &info);
}

inline VkDeviceAddress getDeviceAddressOf(VkAccelerationStructureKHR as)    // GPU 상에 할당된 Acceleration Structure Device 의 메모리 주소를 Extension 함수를 통해 가져온다.
{
    VkAccelerationStructureDeviceAddressInfoKHR info{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = as,
    };
    return vk.vkGetAccelerationStructureDeviceAddressKHR(vk.device, &info);
}

void createBLAS()
{
    float vertices[][3] = {         // 4 개의 Vertices
        { -1.0f, -1.0f, 0.0f },     // Ray Tracing 에서는 2 차원 Vertex 가 불가함.
        {  1.0f, -1.0f, 0.0f },
        {  1.0f,  1.0f, 0.0f },
        { -1.0f,  1.0f, 0.0f },
    };
    uint32_t indices[] = { 0, 1, 3, 1, 2, 3 };

    VkTransformMatrixKHR geoTransforms[] = {
        {
            1.0f, 0.0f, 0.0f, -2.0f,            // 왼쪽으로 2 만큼 이동
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        }, 
        {
            1.0f, 0.0f, 0.0f, 2.0f,            // 오른쪽으로 2 만큼 이동
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        },
    };

    auto [vertexBuffer, vertexBufferMem] = createBuffer(        // Vertex, Index, Geometry Buffer 생성
        sizeof(vertices),           // GPU 에서 각 Buffer 의 주소를 알아야 하기 떄문에 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT flag 를 사용한다.
             // VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR flag 는 이 Buffer 들이 Ray Tracing 의 가속 구조의 Input 으로 작용하는데, GPU 의 Read Only 접근만 가능하게 하는 flag 이다.
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
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,     // Geometry 를 사용할 떄 (BLAS 에서는) 는 VK_GEOMETRY_TYPE_TRIANGLES_KHR type 를 사용한다.
        .geometry = {                                           // 다른 type 들 중 VK_GEOMETRY_TYPE_AABBS_KHR 이 있는데, 이는 Intersection Shader 을 사용할 때 사용한다.
                                                            // 추가로, TLAS 에서는 VK_GEOMETRY_TYPE_INSTANCES_KHR 를 사용한다.
        
            .triangles = {                          // BLAS 이기 때문에 VkAccelerationStructureGeometryDataKHR 인 geometry 의 triangles 에 정보를 넣어준다.

                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,                                         // 3 차원을 사용하기에 RGB 모두 사용.
                .vertexData = { .deviceAddress = getDeviceAddressOf(vertexBuffer) },                // GPU 상에 할당된 메모리 주소 (uint64_t) 를 가져옴.
                .vertexStride = sizeof(vertices[0]),
                .maxVertex = sizeof(vertices) / sizeof(vertices[0]) - 1,                            // 최대 Index Number 를 지정해주기 때문에, 마지막에 -1 해준다. (반드시 -1 해야하는 것은 아니다.)
                .indexType = VK_INDEX_TYPE_UINT32,                                                  // Indices 배열의 각 요소가 uint32_t 이기 때문에 이렇게 설정.
                .indexData = { .deviceAddress = getDeviceAddressOf(indexBuffer) },
                .transformData = { .deviceAddress = getDeviceAddressOf(geoTransformBuffer) },
            },
        },
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,        // 완전히 불투명한 오브젝트를 상정하도록 설정함. ( Ray 의 투과성 X )
    };
    VkAccelerationStructureGeometryKHR geometries[] = { geometry0, geometry0 }; // 동일한 Geometry (사각형) 2 개를 넣어줌.

    uint32_t triangleCount0 = sizeof(indices) / (sizeof(indices[0]) * 3);   // 하나의 Geometry 마다 triangle 2 개씩 구성됨.
    uint32_t triangleCounts[] = { triangleCount0, triangleCount0 };         // Geometry 마다 해당하는 triangle 개수를 집어넣어줌.

    VkAccelerationStructureBuildGeometryInfoKHR buildBlasInfo{                      // BLAS 를 Build 하기위한 정보들 ( 먼저, Prebuild 를 거친 후 나중에 본격적인 Build 를 거친다. )
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,  // buildBlasInfo 구성에 필요한 정보들이 많지만, 그 중 일부만 미리 설정해두었다.
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,                    //  이는 Prebuild 과정이기에 지금은 간소하게 진행하는 것이다.
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount = sizeof(geometries) / sizeof(geometries[0]),
        .pGeometries = geometries,
    };
    
    VkAccelerationStructureBuildSizesInfoKHR requiredSize{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vk.vkGetAccelerationStructureBuildSizesKHR(             // vkGetAccelerationStructureBuildSizesKHR 함수는 Prebuild 하기 위해 필요한 Extension 함수이다.
        vk.device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,    // Build 를 CPU 에서 할 것인지 GPU 에서 할 것인지 지정해줄 수 있는데, 
        &buildBlasInfo,                                     //  VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR 는 GPU 에서 Build 하기 위한 정보이다.
        triangleCounts,                                     // ( Vulkan 은 DirectX 와 다르게 Acceleration Structure Build 과정을 CPU 에서 수행할 수 있다. (DirectX 는 무조건 GPU 상에서 수행) )
        
        &requiredSize);             // 나중에 본격적인 Build 를 할 것인데, 그 때 필요한 용량을 미리 확보하는 것이다. ( Prebuild 과정 )

    std::tie(vk.blasBuffer, vk.blasBufferMem) = createBuffer(           // BLAS Buffer 은 앞으로 계속 가지고 가야하기에 전역변수로 사용된다.
        requiredSize.accelerationStructureSize,                     // BLAS 가 저장되어 있는 Buffer 의 크기
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);                           // BLAS Buffer 와 Scratch Buffer 은 GPU 가 알아서 처리하기 때문에 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT 를 사용한다.

    auto [scratchBuffer, scratchBufferMem] = createBuffer(              // scratchBuffer 는 BLAS Buffer Build 가 끝나면 해제하기 때문에 이와 같이 지역변수로 선언한 것이다.
        requiredSize.buildScratchSize,                              // Build 과정에서 발생하는 추가적인 Buffer 크기
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Generate BLAS handle
    {
        VkAccelerationStructureCreateInfoKHR asCreateInfo{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .buffer = vk.blasBuffer,
            .size = requiredSize.accelerationStructureSize,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,            // BLAS 임을 알려주는 Bottom Level 타입 ( 앞서 buildBlasInfo 를 만들 때도 동일한 타입을 넣어주었다. )
        };
        vk.vkCreateAccelerationStructureKHR(vk.device, &asCreateInfo, nullptr, &vk.blas);   // 실제로 BLAS 가 들어간 Buffer 인 vk.blasBuffer 을 다루는 핸들인 vk.blas 를 만드는 것

        vk.blasAddress = getDeviceAddressOf(vk.blas);       // vk.blas 에 해당하는 Acceleration Structure Device 의 메모리 주소를 받아옴.
    }                                                           // 이제 본격적인 BLAS Build 를 시작한다.

    // Build BLAS using GPU operations
    {
        VkCommandBufferBeginInfo beginInfo {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(vk.commandBuffer, &beginInfo);                                 // 본격적인 BLAS Build 과정
        {
            buildBlasInfo.dstAccelerationStructure = vk.blas;
            buildBlasInfo.scratchData.deviceAddress = getDeviceAddressOf(scratchBuffer);
            VkAccelerationStructureBuildRangeInfoKHR buildBlasRangeInfo[] = {   // 위에서 buildBlasInfo.pGeometries 에 2 개의 geometry0 를 넣었으므로,
                {                                                               //  buildBlasRangeInfo 에 2 개의 geometry0 각각에 대한 정보를 넣어주어야 한다.
                
                    .primitiveCount = triangleCounts[0],            // 현 Transformation 에 사용되는 triangle 개수
                    .transformOffset = 0,
                },
                { 
                    .primitiveCount = triangleCounts[1],
                    .transformOffset = sizeof(geoTransforms[0]),    // 다음 Transformation 을 나타내는 정보의 Offset 을 지정
                }
            };

            VkAccelerationStructureBuildGeometryInfoKHR buildBlasInfos[] = { buildBlasInfo };           // buildBlasInfos 배열을 만들고
            VkAccelerationStructureBuildRangeInfoKHR* buildBlasRangeInfos[] = { buildBlasRangeInfo };   //  buildBlasRangeInfos 배열도 만든다.
            vk.vkCmdBuildAccelerationStructuresKHR(vk.commandBuffer, 1, buildBlasInfos, buildBlasRangeInfos);   // 해당 함수에서 Structures 을 발견할 수 있는데,
                                                                                                                //  이는 여러 개의 BLAS 들을 동시에 만들 수 있음을 의미한다.
            // vkCmdBuildAccelerationStructuresKHR(vk.commandBuffer, 1, &buildBlasInfo, 
            // (const VkAccelerationStructureBuildRangeInfoKHR *const *)&buildBlasRangeInfo);
        }
        vkEndCommandBuffer(vk.commandBuffer);

        VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &vk.commandBuffer,
        }; 
        vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);        // BLAS 만드는 명령을 GPU 에 전달
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
    VkTransformMatrixKHR insTransforms[] = {    // Transformation 2 개가 있었다. 하지만, 이번에는 이름이 insTransforms ( Instance Transformation ) 이다.
        {                                       // BLAS 에서는 geoTransforms 이었는데 이는 Geometry 마다 가지고 있는 Transformation 정보를 의미하고,
            1.0f, 0.0f, 0.0f, 0.0f,             //  TLAS 의 insTransforms 는 Instance 마다 적용되는 Transformation 정보를 의미한다.
            0.0f, 1.0f, 0.0f, 2.0f,
            0.0f, 0.0f, 1.0f, 0.0f          // 즉, 여기에서는 2 개의 Instance 를 만들고 그에 대한 각각의 Transformation 을 만든다.
        }, 
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, -2.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        },
    };

    VkAccelerationStructureInstanceKHR instance0 {
        .mask = 0xFF,                                   // mask 는 대부분 0xFF 을 사용
        .instanceShaderBindingTableRecordOffset = 0,        // 나중에 Shader Binding Table 에서 사용되는 정보
        .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,     // Back Face Culling 을 적용하지 않겠다는 의미의 flag
        .accelerationStructureReference = vk.blasAddress,   // Instance 는 BLAS 를 참조한다.
    };
    VkAccelerationStructureInstanceKHR instanceData[] = { instance0, instance0 };
    instanceData[0].transform = insTransforms[0];
    instanceData[1].transform = insTransforms[1];

    auto [instanceBuffer, instanceBufferMem] = createBuffer(        // instanceData 에 대한 Buffer 를 만듦.
        sizeof(instanceData), 
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* dst;
    vkMapMemory(vk.device, instanceBufferMem, 0, sizeof(instanceData), 0, &dst);
    memcpy(dst, instanceData, sizeof(instanceData));
    vkUnmapMemory(vk.device, instanceBufferMem);

    VkAccelerationStructureGeometryKHR instances{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,         // BLAS 에서는 VK_GEOMETRY_TYPE_TRIANGLES_KHR 을 사용하는 반면,
                                                                //  TLAS 에서는 type 로 VK_GEOMETRY_TYPE_INSTANCES_KHR 를 사용한다.
        .geometry = {           // geometry union 을 채우기
            .instances = {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                .data = { .deviceAddress = getDeviceAddressOf(instanceBuffer) },    // Instance Buffer 의 주소로 설정
            },
        },
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };
                                                    // BLAS 하나에 여러개의 Geometry 가 들어갈 수 있고, 각각의 Geometry 들은 여러 개의 Triangles 이 들어갈 수 있다.
    uint32_t instanceCount = 2;                     // TALS 에는 하나의 Geometry 만 올 수 있고, 그 Geometry 은 여러 개의 Instance 들의 집합이기도 하다.

    VkAccelerationStructureBuildGeometryInfoKHR buildTlasInfo{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount = 1,     // It must be 1 with .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR as shown in the vulkan spec.
        .pGeometries = &instances,      // 현재 TLAS 에서 geometry 가 1 개이므로 geometryCount 는 1 이어야 한다.
    };

    VkAccelerationStructureBuildSizesInfoKHR requiredSize{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};   // Prebuild 과정
    vk.vkGetAccelerationStructureBuildSizesKHR(
        vk.device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildTlasInfo,
        &instanceCount,
        &requiredSize);

    std::tie(vk.tlasBuffer, vk.tlasBufferMem) = createBuffer(
        requiredSize.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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

        vk.tlasAddress = getDeviceAddressOf(vk.tlas);
    }

    // Build TLAS using GPU operations
    {
        VkCommandBufferBeginInfo beginInfo {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};       // 본격적인 TLAS Build 진행
        vkBeginCommandBuffer(vk.commandBuffer, &beginInfo);
        {
            buildTlasInfo.dstAccelerationStructure = vk.tlas;
            buildTlasInfo.scratchData.deviceAddress = getDeviceAddressOf(scratchBuffer);

            VkAccelerationStructureBuildRangeInfoKHR buildTlasRangeInfo = { .primitiveCount = instanceCount };  // TLAS 의 Geometry 는 1 개뿐이기에 굳이 배열을 만들지 않았다.
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

void loadDeviceExtensionFunctions(VkDevice device)      // 드라이버에서 사용할 확장 함수의 포인터를 이쪽으로 가져온다. 해당 과정은 device 를 지정한 후 진행되어야 한다.
{
    vk.vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
                                        // device 와 관련한 외부 드라이버에서 vkGetBufferDeviceAddressKHR 함수를 찾은 후 해당 함수의 포인터를
                                        //  vk.vkGetBufferDeviceAddressKHR 함수 포인터에 대입한다.

    vk.vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
    vk.vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
    vk.vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
    vk.vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
    vk.vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
}

int main()
{
    glfwInit();
    GLFWwindow* window = createWindow();
    createVkInstance(window);
    createVkDevice();
    loadDeviceExtensionFunctions(vk.device);
    createSwapChain();
    
    // createRenderPass();          // BLAS 와 TLAS 가 있으니, RenderPass 와 Pipeline 은 필요없어졌다.
    // createGraphicsPipeline();
    createCommandCenter();
    createSyncObjects(); 

    createBLAS();       // Geometry 들로 구성   ( BLAS (Bottom-Level Acceleration Structure) )
    createTLAS();       // Instance 들로 구성   ( TLAS (Top-Level Acceleration Structure) )

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        //render();
    }

    vkDeviceWaitIdle(vk.device);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}