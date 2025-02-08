#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <tuple>
#include <bitset>
#include <span>
#include <cmath>
#include <random>
//#include "glsl2spv.h"

typedef unsigned int uint;

const uint32_t WIDTH = 1600;
const uint32_t HEIGHT = 1200;
const uint32_t PARTICLE_COUNT = 8192;

#ifdef NDEBUG
const bool ON_DEBUG = false;
#else
const bool ON_DEBUG = true;
#endif


struct Global {
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

    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;

    VkPipeline graphicsPipeline;            // graphics pipeline layout 삭제됨

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence graphicsFence;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    VkBuffer storageBuffer;                 // vertex 버퍼와 index 버퍼 대신 사용
    VkDeviceMemory storageBufferMemory;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    VkPipelineLayout computeLayout;         // compute 부분 추가
    VkPipeline computePipeline;
    VkCommandBuffer computeCommandBuffer;
    VkSemaphore computeFinishedSemaphore;
    VkFence computeFence;

    ~Global() {
        vkDestroySemaphore(device, computeFinishedSemaphore, nullptr);
        vkDestroyFence(device, computeFence, nullptr);
        vkDestroyPipeline(device, computePipeline, nullptr);

        vkDestroyBuffer(device, uniformBuffer, nullptr);
        vkFreeMemory(device, uniformBufferMemory, nullptr);
        vkDestroyBuffer(device, storageBuffer, nullptr);
        vkFreeMemory(device, storageBufferMemory, nullptr);

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroyFence(device, graphicsFence, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);

        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, computeLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

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


struct EmptyPipelineLayout {
    VkPipelineLayout layout;
    static VkPipelineLayout get() {
        static EmptyPipelineLayout g;       // get 함수를 처음 호출할 때 g 또한 생성되게 되는데, 이떄 생성자가 호출되면서 PipelineLayout 이 만들어진다. 그리고 프로그램 종료 시 소멸자를 호출한다.
        return g.layout;                    // static 함수이기 때문에 프로그램 종료까지 쭉 존재한다. 또한 싱글톤 패턴이기 떄문에 private 로 만들어졌다.
    }
private: // For prohibiting new instance
    EmptyPipelineLayout() {
        if (!vk.device) {
            throw std::runtime_error("vk.device must be initialized prior to the first call for EmptyPipelineLayout::get()!");
        }

        VkPipelineLayoutCreateInfo info{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        if (vkCreatePipelineLayout(vk.device, &info, nullptr, &layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }
    ~EmptyPipelineLayout() {
        vkDestroyPipelineLayout(vk.device, layout, nullptr);
    }
};


struct ShaderModule {
private:
    VkShaderModule module;

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

public:
    VkShaderModule get() {
        return module;
    }

    ShaderModule(const char* filename) {
        if (!vk.device) {
            throw std::runtime_error("vk.device must be initialized prior to new ShaderModule()!");
        }

        auto vsSpv = readFile(filename);
        VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = vsSpv.size(),
            .pCode = (uint*)vsSpv.data(),
        };

        if (vkCreateShaderModule(vk.device, &createInfo, nullptr, &module) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
    }
    ~ShaderModule() {
        vkDestroyShaderModule(vk.device, module, nullptr);
    }
};


struct Particle {
    float position[2];
    float velocity[2];
    float color[4];

    static VkVertexInputBindingDescription getBindingDescription() {
        return {
            .binding = 0,
            .stride = sizeof(Particle),
        };
    }
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        return {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,      // x, y
                .offset = 0,
            }, {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,   // r, g, b, a
                .offset = 16,
            }
        };
    }
};


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

template <typename T, typename F, typename... Args>
inline std::vector<T> arrayOf(F pFunc, Args... args)
{
    uint32_t count;
    pFunc(args..., &count, nullptr);
    std::vector<T> result(count);
    pFunc(args..., &count, result.data());
    return result;
}

bool checkValidationLayerSupport(std::vector<const char*>& reqestNames) 
{
    auto availables = arrayOf<VkLayerProperties>(vkEnumerateInstanceLayerProperties);

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
    auto availables = arrayOf<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, device, nullptr);

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
        .apiVersion = VK_API_VERSION_1_0
    };

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if(ON_DEBUG) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    std::vector<const char*> validationLayers;
    if(ON_DEBUG) validationLayers.push_back("VK_LAYER_KHRONOS_validation");
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

    auto devices = arrayOf<VkPhysicalDevice>(vkEnumeratePhysicalDevices, vk.instance);

    std::vector<const char*> extentions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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

    auto queueFamilies = arrayOf<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, vk.physicalDevice);

    vk.queueFamilyIndex = 0;
    {
        for (; vk.queueFamilyIndex < queueFamilies.size(); ++vk.queueFamilyIndex)
        {
            VkBool32 presentSupport = false;                                                    // physicalDevice에 걸맞는 queueFamilyIndex 를 지원하지 않는 ( presentSupport 내용 ) 그래픽카드가 요즘은 거의 없기 때문에 사용
            vkGetPhysicalDeviceSurfaceSupportKHR(vk.physicalDevice, vk.queueFamilyIndex, vk.surface, &presentSupport);

            if (queueFamilies[vk.queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT &&        // VK_QUEUE_GRAPHICS_BIT 와 VK_QUEUE_COMPUTE_BIT 그리고 presentSupport 모두 함께 지원하는 
                queueFamilies[vk.queueFamilyIndex].queueFlags & VK_QUEUE_COMPUTE_BIT &&         //  범용 queue 를 찾음
                presentSupport)
                break;
        }

        if (vk.queueFamilyIndex >= queueFamilies.size())
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
        auto formats = arrayOf<VkSurfaceFormatKHR>(vkGetPhysicalDeviceSurfaceFormatsKHR, vk.physicalDevice, vk.surface);

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
        auto presentModes = arrayOf<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR, vk.physicalDevice, vk.surface);

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

    vk.swapChainImages = arrayOf<VkImage>(vkGetSwapchainImagesKHR, vk.device, vk.swapChain);

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

void createRenderPass()
{
    VkAttachmentDescription colorAttachment{
        .format = vk.swapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentRef0{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass{
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef0,
    };

    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    if (vkCreateRenderPass(vk.device, &renderPassInfo, nullptr, &vk.renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

    for (const auto& view : vk.swapChainImageViews) {
        VkFramebufferCreateInfo framebufferInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = vk.renderPass,
            .attachmentCount = 1,
            .pAttachments = &view,
            .width = WIDTH,
            .height = HEIGHT,
            .layers = 1,
        };

        VkFramebuffer frameBuffer;
        if (vkCreateFramebuffer(vk.device, &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }

        vk.framebuffers.push_back(frameBuffer);
    }
}

void createDescriptorRelated()
{
    // Create Descriptor Set Layout
    {   
        VkDescriptorSetLayoutBinding bindings[] = {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
            {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding),
            .pBindings = bindings,
        };

        if (vkCreateDescriptorSetLayout(vk.device, &layoutInfo, nullptr, &vk.descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize poolSizes[] = {
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
            },
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
            }
        };
        
        VkDescriptorPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1,
            .poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize),
            .pPoolSizes = poolSizes,
        };

        if (vkCreateDescriptorPool(vk.device, &poolInfo, nullptr, &vk.descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    // Create Descriptor Set
    {
        VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = vk.descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &vk.descriptorSetLayout,
        };

        if (vkAllocateDescriptorSets(vk.device, &allocInfo, &vk.descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
    }

    // Create Pipeline Layout
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &vk.descriptorSetLayout,
        };

        if (vkCreatePipelineLayout(vk.device, &pipelineLayoutInfo, nullptr, &vk.computeLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }
}


void createGraphicsPipeline() 
{
    ShaderModule vs("shader.vert.spv");
    ShaderModule fs("shader.frag.spv");

    VkPipelineShaderStageCreateInfo vsStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vs.get(),
        .pName = "main",
    };
    VkPipelineShaderStageCreateInfo fsStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fs.get(),
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vsStageInfo, fsStageInfo };

    auto bindingDescription = Particle::getBindingDescription();
    auto attributeDescriptions = Particle::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = (uint) attributeDescriptions.size(),
        .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,                           // Vertex 하나당 하나의 point 로 간주
    };

    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0,
    };

    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = VK_TRUE,                                                 // draw 할 때 color 의 alpha 를 혼합 연산의 가중치로 사용하기 떄문에 blend true 로 설정
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,                       // 새로운 색상에 대해 srcAlpha 를 그대로 사용
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,             // 기존의 색상에 대해 ( 1 − srcAlpha ) 를 가중치로 사용
        .colorBlendOp = VK_BLEND_OP_ADD,                                        // 덧셈 연산으로 최종 색상 계산 방법을 지정해 resultColor = ( srcColor ∗ srcAlpha ) + ( dstColor * ( 1 − srcAlpha ) ) 로 color Blend 가 진행된다.
        
        /*.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,           // srcAlphaFactor = 1 − srcAlpha
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,                            // dstAlphaFactor = 0
        .alphaBlendOp = VK_BLEND_OP_ADD,*/                                      // 알파 채널을 블렌딩할 때 덧셈(Add) 연산을 사용하여 resultAlpha = srcAlpha * (1 - srcAlpha) + dstAlpha * 0
                                                                                // 알파 값이 소스의 알파만을 기반으로 계산되며, 기존 픽셀의 알파 값은 완전히 무시되지만
                                                                                //  일반적인 블렌딩에서 소스의 알파 값을 유지하는 경우가 많기 때문에 이 설정은 잘 사용되지 않는 편이다.
        
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,    // RGBA 모두 출력
    };

    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = EmptyPipelineLayout::get(),   // Pipeline Layout 은 Pipiline 생성 input 으로 이렇게 사용이 되고, drawcall 하기 전에 Layout 과 descriptor set 을 서로 bind 시켜줄 때 사용하기도 한다.
                                                //  현재 이 EmptyPipelineLayout 는 Singleton 패턴으로 작성되었다.
        .renderPass = vk.renderPass,
        .subpass = 0,
    };

    if (vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vk.graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
}

void createComputePipeline() 
{
    ShaderModule cs("shader.comp.spv");

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = cs.get(),
        .pName = "main",
    };

    VkComputePipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = computeShaderStageInfo,                            // VkGraphicsPipelineCreateInfo 과 다르게 stage 개수가 1개이기 때문
        .layout = vk.computeLayout,
    };

    if (vkCreateComputePipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vk.computePipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
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

    if (vkAllocateCommandBuffers(vk.device, &allocInfo, &vk.computeCommandBuffer) != VK_SUCCESS) {  // compute pipeline 에 대해 처리하ㅏ기 위해 전용 command buffer 을 만듦.
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
        vkCreateFence(vk.device, &fenceInfo, nullptr, &vk.graphicsFence) != VK_SUCCESS ||
        vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &vk.computeFinishedSemaphore) != VK_SUCCESS ||    // compute 와 관련된 GPU 명령들이 모두 끝날 때까지 기다리도록 조치.
        vkCreateFence(vk.device, &fenceInfo, nullptr, &vk.computeFence) != VK_SUCCESS) {
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
    if (vkAllocateMemory(vk.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(vk.device, buffer, bufferMemory, 0);

    return { buffer, bufferMemory };
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) 
{
    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        //.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,  
    };

    vkBeginCommandBuffer(vk.commandBuffer, &beginInfo);
    {
        VkBufferCopy copyRegion{ .size = size };
        vkCmdCopyBuffer(vk.commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    }
    vkEndCommandBuffer(vk.commandBuffer);

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &vk.commandBuffer,
    };

    vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vk.graphicsQueue);
}

void createBuffers() 
{
    // Uniform buffer for deltaTime
    {
        std::tie(vk.uniformBuffer, vk.uniformBufferMemory) = createBuffer(
            sizeof(float),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);    // 계속 업데이트 되기에 HOST_VISIBLE 로 함

        // Write descriptor set
        {
            VkDescriptorBufferInfo bufferInfo{
                .buffer = vk.uniformBuffer,
                .range = VK_WHOLE_SIZE,
            };

            VkWriteDescriptorSet descriptorWrite{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = vk.descriptorSet,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &bufferInfo,
            };
            vkUpdateDescriptorSets(vk.device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    // Storage Buffer for particle info
    {
        VkDeviceSize storageBufferSize = sizeof(Particle) * PARTICLE_COUNT;     // storage buffer 크기 설정

        std::tie(vk.storageBuffer, vk.storageBufferMemory) = createBuffer(
            storageBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,  // 이 버퍼는 stroage 버퍼인 동시에 vertex 버퍼이다.
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);           // Device Local Buffer 이기 때문에 이 버퍼의 값을 copy 를 통해 채워주기 위해 ( Copy 목적지 ) TRANSFER_DST_BIT 가 필요하다.

        // Write descriptor set
        {
            VkDescriptorBufferInfo bufferInfo{
                .buffer = vk.storageBuffer,
                .range = VK_WHOLE_SIZE,
            };

            VkWriteDescriptorSet descriptorWrite{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = vk.descriptorSet,
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &bufferInfo,
            };
            vkUpdateDescriptorSets(vk.device, 1, &descriptorWrite, 0, nullptr);
        }

        // Load data
        {
            auto [stagingBuffer, stagingBufferMemory] = createBuffer(
                storageBufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);    // GPU 에서 데이터를 GPU 로 전송하는 Staging Buffer 이기 때문에 TRANSFER_SRC_BIT 로 Copy 출발지로 설정

            void* dst;
            vkMapMemory(vk.device, stagingBufferMemory, 0, storageBufferSize, 0, &dst);
            {
                std::default_random_engine rndEngine(/*(unsigned)time(nullptr)*/0);
                std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);
                
                std::vector<Particle> particles(PARTICLE_COUNT);
                for (auto& [pos, vel, col] : particles)     // 모든 Particle 에 대해
                {
                    float r = 0.25f * std::sqrt(rndDist(rndEngine));                        // rndDist(rndEngine) 는 0 에서 1 사이의 값이 나온다.
                    float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;
                    float x = r * std::cos(theta) * HEIGHT / WIDTH;     // 좌표계가 화면 비율과 일치하지 않아 타원형으로 표현되기에 보정해줌 
                    float y = r * std::sin(theta);
                    float norm = std::sqrt(x * x + y * y);
                    pos[0] = x;
                    pos[1] = y;
                    vel[0] = x / norm * 0.00025f;
                    vel[1] = y / norm * 0.00025f;
                    col[0] = rndDist(rndEngine);
                    col[1] = rndDist(rndEngine);
                    col[2] = rndDist(rndEngine);
                    col[3] = 1.0;
                }
                memcpy(dst, particles.data(), (size_t)storageBufferSize);       // 한 번에 모든 Particle 들을 GPU 에 복사
            }
            vkUnmapMemory(vk.device, stagingBufferMemory);
            copyBuffer(stagingBuffer, vk.storageBuffer, storageBufferSize);     // GPU 에서의 Copy 작업 자체는 비동기적으로 진행되기 때문에 Queue 에 submit 하고 난 후 Wait 를 해야한다. ( 제일 단순한 방법이지만, 제일 안 좋은 방법이기도 하다. )
            vkDestroyBuffer(vk.device, stagingBuffer, nullptr);
            vkFreeMemory(vk.device, stagingBufferMemory, nullptr);
        }
    }
}

void updateBuffers(float lastFrameTime)
{
    static float* ubo_address = nullptr;
    if (!ubo_address) {
        vkMapMemory(vk.device, vk.uniformBufferMemory, 0, sizeof(float), 0, (void**)&ubo_address);
    }

    *ubo_address = lastFrameTime * 2.0f;
}

void render(float lastFrameTime)
{
    const VkCommandBufferBeginInfo beginInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    // Compute submission        
    {
        vkWaitForFences(vk.device, 1, &vk.computeFence, VK_TRUE, UINT64_MAX);
        vkResetFences(vk.device, 1, &vk.computeFence);
        
        updateBuffers(lastFrameTime);

        vkResetCommandBuffer(vk.computeCommandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
        {
            if (vkBeginCommandBuffer(vk.computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording compute command buffer!");
            }
            
            vkCmdBindPipeline(vk.computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vk.computePipeline);
            vkCmdBindDescriptorSets(
                vk.computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                vk.computeLayout, 0, 1, &vk.descriptorSet, 
                0, nullptr);
            vkCmdDispatch(vk.computeCommandBuffer, PARTICLE_COUNT / 256, 1, 1);

            if (vkEndCommandBuffer(vk.computeCommandBuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record compute command buffer!");
            }
        }

        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &vk.computeCommandBuffer,
            /*.signalSemaphoreCount = 1,
            .pSignalSemaphores = &vk.computeFinishedSemaphore,*/
        };
        if (vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, vk.computeFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit compute command buffer!");
        };
    }

    const VkClearValue clearColor = { .color = {0.0f, 0.0f, 0.0f, 1.0f} };
    const VkViewport viewport{ .width = (float)WIDTH, .height = (float)HEIGHT, .maxDepth = 1.0f };
    const VkRect2D scissor{ .extent = {.width = WIDTH, .height = HEIGHT } };
    uint32_t imageIndex;

    // Graphics submission
    {
        vkWaitForFences(vk.device, 1, &vk.graphicsFence, VK_TRUE, UINT64_MAX);
        vkResetFences(vk.device, 1, &vk.graphicsFence);
        
        vkAcquireNextImageKHR(
            vk.device, vk.swapChain, UINT64_MAX, 
            vk.imageAvailableSemaphore, VK_NULL_HANDLE, 
            &imageIndex);

        vkResetCommandBuffer(vk.commandBuffer, 0);
        {
            if (vkBeginCommandBuffer(vk.commandBuffer, &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = vk.renderPass,
                .framebuffer = vk.framebuffers[imageIndex],
                .renderArea = {.extent = {.width = WIDTH, .height = HEIGHT } },
                .clearValueCount = 1,
                .pClearValues = &clearColor,
            };

            vkCmdBeginRenderPass(vk.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            {
                vkCmdBindPipeline(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.graphicsPipeline);
                vkCmdSetViewport(vk.commandBuffer, 0, 1, &viewport);
                vkCmdSetScissor(vk.commandBuffer, 0, 1, &scissor);

                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(vk.commandBuffer, 0, 1, &vk.storageBuffer, offsets);
                vkCmdDraw(vk.commandBuffer, PARTICLE_COUNT, 1, 0, 0);
            }
            vkCmdEndRenderPass(vk.commandBuffer);

            if (vkEndCommandBuffer(vk.commandBuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

        VkSemaphore waitSemaphores[] = { 
            //vk.computeFinishedSemaphore,          
            vk.imageAvailableSemaphore 
        };
        VkPipelineStageFlags waitStages[] = { 
            //VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,   
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT 
        };

        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = sizeof(waitSemaphores)/sizeof(VkSemaphore),
            .pWaitSemaphores = waitSemaphores,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &vk.commandBuffer,
            /*.signalSemaphoreCount = 1,
            .pSignalSemaphores = &vk.renderFinishedSemaphore,*/
        };
        if (vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, vk.graphicsFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
    }
    
    // Present submission
    {
        VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            /*.waitSemaphoreCount = 1,
            .pWaitSemaphores = &vk.renderFinishedSemaphore,*/
            .swapchainCount = 1,
            .pSwapchains = &vk.swapChain,
            .pImageIndices = &imageIndex,
        };

        vkQueuePresentKHR(vk.graphicsQueue, &presentInfo);
    }
}

int main() 
{
    glfwInit();
    GLFWwindow* window = createWindow();
    createVkInstance(window);
    createVkDevice();
    createSwapChain();
    createRenderPass();
    createDescriptorRelated();
    createGraphicsPipeline();
    createComputePipeline();
    createCommandCenter();
    createSyncObjects();
    createBuffers();

    glfwFocusWindow(window);    // Application window is above console window

    double lastTime = glfwGetTime();
    float lastFrameTime = 0.f;
    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();
        render(lastFrameTime);
        
        double currentTime = glfwGetTime();
        lastFrameTime = (float)(currentTime - lastTime) * 1000.0f;
        lastTime = currentTime;
    }
    
    vkDeviceWaitIdle(vk.device);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}