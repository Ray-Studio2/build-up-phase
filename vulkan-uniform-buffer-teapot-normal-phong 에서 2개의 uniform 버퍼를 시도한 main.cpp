#define GLFW_INCLUDE_VULKAN
#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_FORCE_RADIANS

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <tuple>
#include <bitset>
#include <span>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

#include <chrono>

#include "tiny_obj_loader.h"

//#include "glsl2spv.h"

typedef unsigned int uint;

const uint32_t WIDTH = 1600;
const uint32_t HEIGHT = 1200;

#ifdef NDEBUG
const bool ON_DEBUG = false;
#else
const bool ON_DEBUG = true;
#endif

float* vData;

tinyobj::attrib_t attrib;
std::vector<tinyobj::shape_t> shapes;
std::vector<tinyobj::material_t> materials;

std::string warn;
std::string err;

bool isNormalMode = true;

void keyboard(GLFWwindow* window, int key, int code, int action, int mods);

struct MVP_Projecttion {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    bool isNormalMode = true;
} MVP;

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

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    //VkBuffer uniformBuffer;
    VkBuffer uniformBuffer1;
    VkBuffer uniformBuffer2;

    //VkDeviceMemory uniformBufferMemory;
    VkDeviceMemory uniformBufferMemory1;
    VkDeviceMemory uniformBufferMemory2;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    ~Global() {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        //vkDestroyBuffer(device, uniformBuffer, nullptr);
        vkDestroyBuffer(device, uniformBuffer1, nullptr);
        vkDestroyBuffer(device, uniformBuffer2, nullptr);

        //vkFreeMemory(device, uniformBufferMemory, nullptr);
        vkFreeMemory(device, uniformBufferMemory1, nullptr);
        vkFreeMemory(device, uniformBufferMemory2, nullptr);

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        vkDestroyFence(device, inFlightFence, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);

        for (auto framebuffer : framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
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


struct Geometry {
    static const uint vertexBytesSize = 24;
    static const uint vertexPositionOffset = 0;
    static const uint vertexColorOffset = 12;

    inline static float* data_v = nullptr;
    inline static uint16_t* data_i = nullptr;

    /*static const uint vertexBytesSize = 20;
    static const uint vertexPositionOffset = 0;
    static const uint vertexColorOffset = 8;*/

    static std::tuple<float*, size_t> getVertices() {
    //std::tuple<float*, size_t> getVertices() {
        //static float data[] = {
        //    -0.5, -0.5, 1.0, 1, 0, 0,   // x, y, z, r, g, b -> total 24 bytes per vertex
        //    0.5, -0.5, 1.0, 0, 1, 0,
        //    0.5, 0.5, 1.0, 0, 0, 1,
        //    -0.5, 0.5, 1.0, 0, 0, 1,
        //};

        //static float* data = vData;
        //static float* data;     // = new float[3180];   // x, y, z, x_n, y_n, z_n -> 530 vertices -> 12720 Bytes
        //float* data;

        //return { data, (4 * vertexBytesSize) };
        return { data_v, attrib.vertices.size() / 3 * vertexBytesSize };
    }
    
    /*static void putVertices(float* inputVertices, size_t size) {
        data_v = new float[size / 4];

        for (int i = 0; i < size / 4; i++)
            data_v[i] = inputVertices[i];
    }*/

    static std::tuple<uint16_t*, size_t> getIndices() {
    //std::tuple<uint16_t*, size_t> getIndices() {
        /*static uint16_t data[] = {
            0, 1, 2, 2, 3, 0
        };*/

        //static uint16_t* data = (uint16_t*)shapes[0].mesh.indices.data();
        //static uint16_t* data;  // = new uint16_t[2976];
        //uint16_t* data;

        //return { data, 6 * 2 };
        return { data_i, shapes[0].mesh.indices.size() * 2 };
    }

    static VkVertexInputBindingDescription getBindingDescription() {
        return {
            .binding = 0,
            .stride = vertexBytesSize,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,   // Vertex 정보를 넘긴다는 의미
        };
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        return {
            {
                .location = 0,
                .binding = 0,
                //.format = VK_FORMAT_R32G32_SFLOAT,      // x, y
                .format = VK_FORMAT_R32G32B32_SFLOAT,      // x, y, z
                .offset = vertexPositionOffset,
            }, {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,   // r, g, b
                .offset = vertexColorOffset,
            }
        };
    }
};

//Geometry geo;

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

std::vector<char> readFile(const std::string& filename)
{
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
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(vk.physicalDevice, vk.queueFamilyIndex, vk.surface, &presentSupport);

            if (queueFamilies[vk.queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport)
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

void createDescriptorRelated()      // 지금의 Descriptor 는 Uniform Buffer 오브젝트 타입을 레퍼런스하는 역할을 맡는다.
{                                   // Vulkan 의 Uniform Buffer Object 는 Descriptor 의 한 종류이다.
                                    // Descriptor Set 은 그러한 Descriptor 들을 모아놓은 집합을 말한다.
                                    // Descriptor 와 Descriptor Set 은 모두 Descriptor Pool 에서 정의된다. 메모리의 어딘가에서 할당이 되어야 하는데, 이 할당 메모리를 제공하는 것이 Dexcriptor Pool 이다.
                                    // Descriptor Set Layout 은 Descriptor Pool 안에서 Descriptor Set 이 정의될 때, 어떻게 정의되어야 하는 지에 대한 메모리 관련 정보가 담긴 것이다.
                                    //  Shader Interface 의 Binding 넘버 마다 어떤 정보가 bind 되어 있는 지에 대한 정보도 다룬다.
                                    // Pipeline Layout 은 여러 개의 Descriptor Set Layout 을 리스트 또는 페이지로 보관하는 역할을 맡는데, Shader Interface 에서 Set 넘버와 대응된다.

    // Create Descriptor Set Layout             
    {                                           
        /*VkDescriptorSetLayoutBinding uboLayoutBinding{
            .binding = 0,                                               // Binding 넘버는 0으로 설정
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,        // UNIFORM_BUFFER 타입의 Binding 임을 설정
            //.descriptorCount = 1,                                       // Descriptor Set 내에 몇 개의 Descriptor 가 들어갈 것인지 개수 지정 ( 2 이상이면 Descriptor 들이 배열로서 저장됨 )
            .descriptorCount = 2,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,                   // Vertex Shader 에서만 적용되는 것으로 설정
        };*/

        VkDescriptorSetLayoutBinding uboLayoutBindings[2];

        uboLayoutBindings[0] = {
            .binding = 0,                                               // Binding 넘버는 0으로 설정
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,        // UNIFORM_BUFFER 타입의 Binding 임을 설정
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        uboLayoutBindings[1] = {
            .binding = 1,                                               // Binding 넘버는 0으로 설정
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,        // UNIFORM_BUFFER 타입의 Binding 임을 설정
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            //.bindingCount = 1,
            .bindingCount = 2,
            .pBindings = uboLayoutBindings,
        };

        if (vkCreateDescriptorSetLayout(vk.device, &layoutInfo, nullptr, &vk.descriptorSetLayout) != VK_SUCCESS) {  // Descriptor Set Layout 을 만듦
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize poolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,                  // UNIFORM_BUFFER 타입 전용으로 사용
            //.descriptorCount = 1,                                       // UNIFORM_BUFFER 타입으로 사용될 Descriptor 개수를 예상하여 입력해야 함. ( 여기에서는 1개 )
            .descriptorCount = 2,
        };

        VkDescriptorPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1,                                               // Descriptor Pool 안에서 Descriptor Set 이 얼마나 많이 생성될지 예측하여 정하는 최대 개수
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize,
        };

        if (vkCreateDescriptorPool(vk.device, &poolInfo, nullptr, &vk.descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    // Create Descriptor Set
    {
        VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = vk.descriptorPool,            // Descriptor Pool 에 메모리를 할당 ( 공간을 미리 확보 )
            .descriptorSetCount = 1,                        // 몇 개의 descriptorSetCount 만큼의 Descriptor Set 들을 동시에 메모리 allocate 하여 넘겨줄 지 정함.
            .pSetLayouts = &vk.descriptorSetLayout,         // Descriptor Set Layout 들이 있는 배열의 시작 주소
        };

        if (vkAllocateDescriptorSets(vk.device, &allocInfo, &vk.descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
    }

    // Create Pipeline Layout
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{                  // Descriptor Set Layout 들을 모아서 하나의 Pipeline Layout 으로 정의함
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,                                        // Descriptor Set Layout 가 1개
            .pSetLayouts = &vk.descriptorSetLayout,
        };

        if (vkCreatePipelineLayout(vk.device, &pipelineLayoutInfo, nullptr, &vk.pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    // vkDestroyDescriptorSetLayout(vk.device, vk.descriptorSetLayout, nullptr);
}

void createGraphicsPipeline()
{
    auto spv2shaderModule = [](const char* filename) {
        auto vsSpv = readFile(filename);
        VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = vsSpv.size(),
            .pCode = (uint*)vsSpv.data(),
        };

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(vk.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
        };

    VkShaderModule vsModule = spv2shaderModule("vertex_input_vs.spv");
    VkShaderModule fsModule = spv2shaderModule("vertex_input_fs.spv");

    /*
    ////////////////////////////////

    auto vs_spv = glsl2spv(GLSLANG_STAGE_VERTEX, "vertex_input_vs.glsl");

    VkShaderModuleCreateInfo vsModuleInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = vs_spv.size(),
        .pCode = vs_spv.data(),
    };

    VkShaderModule vsModule;
    if (vkCreateShaderModule(vk.device, &vsModuleInfo, nullptr, &vsModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Vertex shader module!");
    }

    auto fs_spv = glsl2spv(GLSLANG_STAGE_FRAGMENT, "vertex_input_fs.glsl");

    VkShaderModuleCreateInfo fsModuleInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = fs_spv.size(),
        .pCode = fs_spv.data(),
    };

    VkShaderModule fsModule;
    if (vkCreateShaderModule(vk.device, &fsModuleInfo, nullptr, &fsModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Fragment shader module!");
    }

    ////////////////////////////////
    */

    VkPipelineShaderStageCreateInfo vsStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vsModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fsStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fsModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vsStageInfo, fsStageInfo };

    auto bindingDescription = Geometry::getBindingDescription();
    auto attributeDescriptions = Geometry::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = (uint)attributeDescriptions.size(),
        .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
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
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
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
        .layout = vk.pipelineLayout,
        .renderPass = vk.renderPass,
        .subpass = 0,
    };

    if (vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vk.graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(vk.device, vsModule, nullptr);
    vkDestroyShaderModule(vk.device, fsModule, nullptr);
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
        vkCreateFence(vk.device, &fenceInfo, nullptr, &vk.inFlightFence) != VK_SUCCESS) {
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

void createVertexBuffer()
{
    auto [data, size] = Geometry::getVertices();

    std::tie(vk.vertexBuffer, vk.vertexBufferMemory) = createBuffer(
        size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* dst;
    vkMapMemory(vk.device, vk.vertexBufferMemory, 0, size, 0, &dst);
    memcpy(dst, data, size);
    //memcpy(dst, Geometry::data_v, size);
    vkUnmapMemory(vk.device, vk.vertexBufferMemory);
}

void updateVertexBuffer(float t)
{
    size_t size = std::get<1>(Geometry::getVertices());
    uint count = (uint)size / sizeof(float);
    void* dst;
    vkMapMemory(vk.device, vk.vertexBufferMemory, 0, size, 0, &dst);
    for (uint i = 0; i < count; i += 5)
        ((float*)dst)[i] += t;
    vkUnmapMemory(vk.device, vk.vertexBufferMemory);
}

void createIndexBuffer()
{
    auto [data, size] = Geometry::getIndices();

    auto [stagingBuffer, stagingBufferMemory] = createBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    std::tie(vk.indexBuffer, vk.indexBufferMemory) = createBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    void* dst;
    vkMapMemory(vk.device, stagingBufferMemory, 0, size, 0, &dst);
    memcpy(dst, data, size);
    //memcpy(dst, Geometry::data_i, size);
    vkUnmapMemory(vk.device, stagingBufferMemory);

    copyBuffer(stagingBuffer, vk.indexBuffer, size);

    vkDestroyBuffer(vk.device, stagingBuffer, nullptr);
    vkFreeMemory(vk.device, stagingBufferMemory, nullptr);
}

void updateUniformBuffer(float t = 0.0)
{
    //static void* dst;               // dst 는 프로그램 종료까지 쭉 존재해야 하기 때문에 static 으로 사용
    static void* dst1;
    static void* dst2;

    //float translation[2] = { 0, std::sin(t * 100) };
    //float translation[2] = { 0, t };

    //if (!vk.uniformBuffer1)          // uniformBuffer 가 없는 경우
    if (!vk.uniformBuffer1 && !vk.uniformBuffer2)
    {
        std::tie(vk.uniformBuffer1, vk.uniformBufferMemory1) = createBuffer(      // uniformBuffer 생성
            //sizeof(translation),
            sizeof(MVP),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);    // 자주 값이 변하기 때문에 HOST_VISIBLE 해야 함

        std::tie(vk.uniformBuffer2, vk.uniformBufferMemory2) = createBuffer(      // uniformBuffer 생성
            //sizeof(translation),
            sizeof(isNormalMode),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        //vkMapMemory(vk.device, vk.uniformBufferMemory, 0, sizeof(MVP), 0, &dst);
        //vkMapMemory(vk.device, vk.uniformBufferMemory1, 0, sizeof(MVP), 0, &dst1);
        //vkMapMemory(vk.device, vk.uniformBufferMemory2, 0, sizeof(MVP), 0, &dst2);
        //vkMapMemory(vk.device, vk.uniformBufferMemory, 0, sizeof(translation), 0, &dst);    // CPU 쪽의 주소를 dst 로 할당받고
                                                                                            //  Unmap 으로 해제를 해주지 않아도 자동으로 GPU 로 dst 의 정보가 전달됨.
        //VkDescriptorBufferInfo bufferInfo{                                                  // Unmap 해주지 않았다는 것은 dst 가 계속 유효하다는 의미
        //    .buffer = vk.uniformBuffer,             // Uniform Buffer 핸들을 지정
        //    .range = VK_WHOLE_SIZE,                 // 할당받은 메모리 전체를 적용.
        //};

        VkDescriptorBufferInfo bufferInfos[2];
        bufferInfos[0] = { vk.uniformBuffer1, 0, VK_WHOLE_SIZE };
        bufferInfos[1] = { vk.uniformBuffer2, 0, VK_WHOLE_SIZE };

        //VkWriteDescriptorSet descriptorWrite{
        //    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        //    .dstSet = vk.descriptorSet,
        //    .dstBinding = 0,                                        // 0번 Binding 으로 설정
        //    //.descriptorCount = 1,
        //    .descriptorCount = 2,
        //    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        //    .pBufferInfo = bufferInfos,
        //};

        VkWriteDescriptorSet descriptorWrites[2];

        descriptorWrites[0] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = vk.descriptorSet,
            .dstBinding = 0,                                        // 0번 Binding 으로 설정
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfos[0],
        };

        descriptorWrites[1] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = vk.descriptorSet,
            .dstBinding = 1,                                        // 1번 Binding 으로 설정
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfos[1],
        };

        vkUpdateDescriptorSets(vk.device, 2, descriptorWrites, 0, nullptr);

        vkMapMemory(vk.device, vk.uniformBufferMemory1, 0, sizeof(MVP), 0, &dst1);
        vkMapMemory(vk.device, vk.uniformBufferMemory2, 0, sizeof(isNormalMode), 0, &dst2);
    }

    memcpy(dst1, &MVP, sizeof(MVP));

    memcpy(dst2, &isNormalMode, sizeof(isNormalMode));
}

void render()
{
    const VkClearValue clearColor = { .color = {0.0f, 0.0f, 0.0f, 1.0f} };
    const VkViewport viewport{ .width = (float)WIDTH, .height = (float)HEIGHT, .maxDepth = 1.0f };
    const VkRect2D scissor{ .extent = {.width = WIDTH, .height = HEIGHT } };
    const VkCommandBufferBeginInfo beginInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    vkWaitForFences(vk.device, 1, &vk.inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &vk.inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(vk.device, vk.swapChain, UINT64_MAX, vk.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

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
            size_t numIndices = std::get<1>(Geometry::getIndices()) / sizeof(uint16_t);
            vkCmdBindVertexBuffers(vk.commandBuffer, 0, 1, &vk.vertexBuffer, offsets);
            vkCmdBindIndexBuffer(vk.commandBuffer, vk.indexBuffer, 0, VK_INDEX_TYPE_UINT16);
            vkCmdBindDescriptorSets(                                                            // uniform buffer 을 bind 시킴
                vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,                              // Pipeline Layout 과 Shader 는 이미 서로 bind 되어 있음
                vk.pipelineLayout, 0,                                                           //  ( 최적화를 위해 Pipeline 생성할 때 Pipeline Layout 과 Shader 정보를 부여하면서 bind 시킴 )
                1, &vk.descriptorSet,           // Descriptor Set 에는 Uniform Buffer 에 대한 정보가 이미 있으니 이를 bind 해주면, 결과적으로 쉐이더와 Uniform Buffer 가 연결이 된다.
                0, nullptr);
            //for(uint i=0; i<200000; i++)
                vkCmdDrawIndexed(vk.commandBuffer, (uint)numIndices, 1, 0, 0, 0);

        }
        vkCmdEndRenderPass(vk.commandBuffer);

        if (vkEndCommandBuffer(vk.commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    VkPipelineStageFlags waitStages[] = {
        //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT 
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
    };

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vk.imageAvailableSemaphore,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &vk.commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &vk.renderFinishedSemaphore,
    };

    if (vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, vk.inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vk.renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &vk.swapChain,
        .pImageIndices = &imageIndex,
    };

    vkQueuePresentKHR(vk.graphicsQueue, &presentInfo);
}

void readOBJ(string inputfile) {
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, inputfile.c_str());

	if (!warn.empty()) {
		std::cout << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0) {
					tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
				}

				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
				}
				// Optional: vertex colors
				// tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
				// tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
				// tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
			}
			index_offset += fv;

			// per-face material
			//shapes[s].mesh.material_ids[f];
		}
	}

    cout << attrib.vertices.size() << endl;
    cout << attrib.normals.size() << endl;
    cout << shapes[0].mesh.indices.size() << endl;

    vData = new float[attrib.vertices.size() + attrib.normals.size()];

    int dataIndex = 0;
	int vertexIndex = 0;
	int normalIndex = 0;

	for (int i = 0; i < attrib.vertices.size() / 3; i++) {
		for (int j = 0; j < 3; j++)
			vData[dataIndex++] = attrib.vertices[vertexIndex++];

		for(int k = 0; k < 3; k++)
			vData[dataIndex++] = attrib.normals[normalIndex++];
	}

    /*auto [data_v, size_v] = Geometry::getVertices();
    for (int i = 0; i < size_v / 4; i++)
        data_v[i] = vData[i];
    

    auto [data_i, size_i] = Geometry::getIndices();
    for (int i = 0; i < size_i / 2; i++)
        data_i[i] = (uint16_t)shapes[0].mesh.indices[i].vertex_index;*/

    return;
}

void updateBeforeDrawing()
{
    /////////////////////////////////

    static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    MVP.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	MVP.view = glm::lookAt(glm::vec3(30.0f, 30.0f, 30.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	MVP.proj = glm::perspective(glm::radians(45.0f), vk.swapChainImageExtent.width / (float)vk.swapChainImageExtent.height, 0.1f, 1000.0f);

	MVP.proj[1][1] *= -1;

    /////////////////////////////////

    return;
}

int main()
{
    readOBJ("teapot.obj");

    {
        auto [data_v, size_v] = Geometry::getVertices();

        //Geometry::putVertices(vData, size_v);

        Geometry::data_v = new float[attrib.vertices.size() + attrib.normals.size()]; 
        for (int i = 0; i < size_v / 4; i++)
            Geometry::data_v[i] = vData[i];

        auto [data_i, size_i] = Geometry::getIndices();

        Geometry::data_i = new uint16_t[shapes[0].mesh.indices.size()];
        for (int i = 0; i < size_i / 2; i++)
            Geometry::data_i[i] = (uint16_t)shapes[0].mesh.indices[i].vertex_index;
    }

    glfwInit();
    GLFWwindow* window = createWindow();

    glfwSetKeyCallback(window, keyboard);

    createVkInstance(window);
    createVkDevice();
    createSwapChain();
    createRenderPass();
    createDescriptorRelated();
    createGraphicsPipeline();
    createCommandCenter();
    createSyncObjects();
    createVertexBuffer();
    createIndexBuffer();

    float t = 0.f;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        //updateVertexBuffer(t * 0.01f);
        updateBeforeDrawing();

        updateUniformBuffer();
        //updateUniformBuffer(t);
        render();
        //t += 0.001f;
    }

    vkDeviceWaitIdle(vk.device);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void	keyboard(GLFWwindow* window, int key, int code, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_Q:
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GL_TRUE);
            break;

        case GLFW_KEY_SPACE:
            MVP.isNormalMode = !MVP.isNormalMode;
            //isNormalMode = !isNormalMode;
            break;
        }
	}
}