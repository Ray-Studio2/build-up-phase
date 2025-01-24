#define GLFW_INCLUDE_VULKAN         // Vulkan 은 모든 가능성을 염두에 두고 만들어졌다.
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <fstream>
//#include "glsl2spv.h"

typedef unsigned int uint;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#ifdef NDEBUG                   // 현재 Debug 모드일 때만 ON_DEBUG 가 True 임 (자동으로 T, F 정해줌)
const bool ON_DEBUG = false;
#else
const bool ON_DEBUG = true;
#endif

struct Global {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;            // Physical Device
    VkDevice device;                            // Logical Device

    VkQueue graphicsQueue; // assume allowing graphics and present
    uint queueFamilyIndex;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    const VkFormat swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;    // intentionally chosen to match a specific format ( 일반적으로 사용되는 포맷이기에 의도적으로 선택됨 )
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

    ~Global() {
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

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,     // 메시지 엄격도
    VkDebugUtilsMessageTypeFlagsEXT messageType,                // 메시지 타입
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
    void* pUserData) 
{
    const char* severity;
    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severity = "[Verbose]"; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severity = "[Warning]"; break;    // Warning 메시지 활성화. 
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: severity = "[Error]"; break;        // Error 메시지 활성화.
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

GLFWwindow* createWindow()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);                             // 창 크기 조절 불가
    return glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);     // 창 크기 및 창 이름 등 설정
}

void createVkInstance(GLFWwindow* window)
{
    VkApplicationInfo appInfo{                          // 어플리케이션의 이름 또는 설정사항들
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .apiVersion = VK_API_VERSION_1_0
    };

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);       // Instance Extension : Vulkan 인스턴스를 초기화할 때 필요한 추가 기능을 제공
                                                                                                // 필요한 Extension 들이 플랫폼마다 다르기 때문에 GLFW 를 통해 현재 플랫폼에 맞는 Extension 을 받아옴
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);   // Vector 컨테이너로 래핑함 -> Debugger 모드일 경우 Extension 을 하나 더 추가하기 위함.
    if(ON_DEBUG) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);                       // Debugger 모드일 경우, 필수적인 Instance Extension 을 추가함

    std::vector<const char*> validationLayers;
    if(ON_DEBUG) validationLayers.push_back("VK_LAYER_KHRONOS_validation");                     // Debugger 모드일 경우, Instance Layer 을 추가함
    if (!checkValidationLayerSupport(validationLayers)) {                                       // 현재 Vulkan 이 해당 Layer 을 지원하는지 알 수 있는 함수
        throw std::runtime_error("validation layers requested, but not available!");
    }
                                                                                                // Ctrl + F5 => 문제가 있어도 디버거 출력 창에 경고 또는 오류 메시지가 뜸
                                                                                                // F5        => 디버거 출력 창에 경고 또는 오류 메시지가 뜸

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{                                         // Debug Layer 가 활성화되어있을 때에만 작동하는데, 오류 메시지들을 디버그 출력창이 아닌 콘솔창에 띄워줌.
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,                       // 그냥 사용하면 쓸데없는 메시지가 수도 없이 올라오기에, 원하는 종류의 메시지만 올라오도록 커스터마이징이 가능.
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,     // Warning 메시지와 Error 메시지만 띄우게 함. ( 가독성을 위함 )
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,       // 에러가 발생했을 때, 메시지를 출력하는데 debugCallback 함수를 따르도록 설정함.
    };

    VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = ON_DEBUG ? &debugCreateInfo : nullptr,         // 이 줄을 주석처리하면 무조건 해당 내부변수는 nullptr 이 된다.
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = (uint)validationLayers.size(),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = (uint)extensions.size(),       // Extension 을 지원하지 않는다면 이후에 Instance 를 생성하는 과정에서 실패하게 될 것이다.
        .ppEnabledExtensionNames = extensions.data(),
    };

    if (vkCreateInstance(&createInfo, nullptr, &vk.instance) != VK_SUCCESS) {       // Instance 생성
        throw std::runtime_error("failed to create instance!");
    }

    if (ON_DEBUG) {                                                                 // vkCreateDebugUtilsMessengerEXT 라는 함수를 호출을 해야 콘솔 창에 디버거 메신저가 활성화된다.
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk.instance, "vkCreateDebugUtilsMessengerEXT");   // EXT 로 끝나는 함수는 확장함수인데, 확장함수는 기본적으로 제공되지 않아 함수 포인터를 달라고 요청해야 함.
        if (!func || func(vk.instance, &debugCreateInfo, nullptr, &vk.debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    if (glfwCreateWindowSurface(vk.instance, window, nullptr, &vk.surface) != VK_SUCCESS) {     // Surface : Swap Chain 과 관련된 Instance 속성 ( Instance 만들 때 만들어야 한다. )
        throw std::runtime_error("failed to create window surface!");                           // Surface 또한 플랫폼마다 다르기에 GLFW 의 도움을 받는다.
    }
}

void createVkDevice()
{
    vk.physicalDevice = VK_NULL_HANDLE;
                                                                        // Count 를 먼저 구하고 해당 Count 만큼 배열을 만든 후 시작주소들을 해당 배열들에 넣는 과정
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vk.instance, &deviceCount, nullptr);         // Physical Device, 즉, GPU 를 찾고 그 개수를 deviceCount 에 넣음.
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vk.instance, &deviceCount, devices.data());  // Physical Device 의 핸들들이 device.data() 에 들어가게 됨. ( Logical Device 를 만들기 전 작업 )

    std::vector<const char*> extentions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };      // Device Extension 만들기. ( "VK_KHR_swapchain" 라는 Extension 은 필수적임 )

    for (const auto& device : devices) 
    {
        if (checkDeviceExtensionSupport(device, extentions))            // 나열된 Device 들 (그래픽카드들) 이 해당 Extension 을 지원하는지 알아냄.
        {
            vk.physicalDevice = device;                                     // 지원하면 Physical Device (GPU) 를 해당 그래픽카드로 지정
            break;
        }
    }

    if (vk.physicalDevice == VK_NULL_HANDLE) {                          // 모든 GPU 가 Extension 을 지원하지 않으면 프로그램은 종료됨.
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    uint32_t queueFamilyCount = 0;                                      // 명령들을 순차적으로 집어넣는 Command Queue 의 Family 개수
    vkGetPhysicalDeviceQueueFamilyProperties(vk.physicalDevice, &queueFamilyCount, nullptr);    // 그래픽카드마다 지원하는 Queue 들이 각기 다 다르기에 Queue 개수를 가져온다.
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);                       // Queue Family 내에는 여러 개의 Queue 가 있다.
    vkGetPhysicalDeviceQueueFamilyProperties(vk.physicalDevice, &queueFamilyCount, queueFamilies.data());       // Queue 에는 Compute 전용 Queue 와 Transfer 전용 Queue 등이 있다.
                                                                                                                // 범용 Queue 는 그래픽 명령, Compute, Transfer 등 여러 종류를 수행할 수 있다.
    vk.queueFamilyIndex = 0;                                                                                // 범용 Queue 를 사용하기 위해 범용 Queue 를 찾아야 한다.
    {
        for (; vk.queueFamilyIndex < queueFamilyCount; ++vk.queueFamilyIndex)
        {
            VkBool32 presentSupport = false;                                                    // Present : 렌더링 후 화면에 presentation 을 하는 Swap Chain 의 명령 또한 Queue 를 통해 진행되는데, 이러한 presentation 을 지원하는 Queue 인지에 대한 여부
            vkGetPhysicalDeviceSurfaceSupportKHR(vk.physicalDevice, vk.queueFamilyIndex, vk.surface, &presentSupport);  // 현 GPU 의 현재 QueueFamily 가 Present 를 만족하는 지 알아냄.

            if (queueFamilies[vk.queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport)    // Queue Flag 로 해당 Queue 가 어떤 기능에 대해 활성화되어있는지 알 수 있다.
                break;                                                                                      // 그래픽 명령 수행과 presentSupport 을 지원하는 범용 Queue 를 찾으면 for 문 나옴.
        }

        if (vk.queueFamilyIndex >=  queueFamilyCount)  
            throw std::runtime_error("failed to find a graphics & present queue!");
    }
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vk.queueFamilyIndex,
        .queueCount = 1,                                            // Queue Family 안에 2개 이상의 Queue 가 있을 수 있어도 1로 설정해야 한다.
        .pQueuePriorities = &queuePriority,
    };

    VkDeviceCreateInfo createInfo{                                  // Logical Device 를 만들기 위한 작업 (이전까지 한 것들은 현 그래픽카드의 기능 중 어느 기능까지 사용할 지 상세하게 정한 것이다.)
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = (uint)extentions.size(),
        .ppEnabledExtensionNames = extentions.data(),
    };

    if (vkCreateDevice(vk.physicalDevice, &createInfo, nullptr, &vk.device) != VK_SUCCESS) {    // Logical Device : Physical Device 를 어떤 식으로 사용할 지 정하기 위해 논리적으로 인터페이스를 만든 것
        throw std::runtime_error("failed to create logical device!");                           // vk.device 에서는 Queue 를 하위로 종속시킨다. 
    }

    vkGetDeviceQueue(vk.device, vk.queueFamilyIndex, 0, &vk.graphicsQueue);
}

void createSwapChain()                                                  // Swap Chain 과 Window 사이에 Surface 라는 인터페이스가 있다.
{                                                                       // Swap Chain 은 Surface 를 통해 Window 로 화면을 출력한다. Vulkan 렌더링 -> Swap Chain -> Surface -> Window 출력
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physicalDevice, vk.surface, &capabilities); // 현재의 그래픽카드가 Surface 에 대해 어디까지 사용가능한지 그 정보를 capabilities 에 넣어줌. 
    
    const VkColorSpaceKHR defaultSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;     // Swap Chain 포맷이 VK_FORMAT_B8G8R8A8_SRGB 가 아니면 해당 VK_COLOR_SPACE_SRGB_NONLINEAR_KHR 도 사용 못 함.
    {
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, vk.surface, &formatCount, nullptr);         // 현재 그래픽카드에서 Surface 에 출력하는 Format 에 대한 내용을 가져옴.
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, vk.surface, &formatCount, formats.data());      // Vector 에 지원하는 포맷들을 집어넣어 나열하고
                                                                                                                //  의도적으로 정한 Swap Chain 포맷이고 defaultSpace 도 사용 가능한지 확인되면
        bool found = false;                                                                                     //  Swap Chain Format 으로는 B8G8R8A8_SRGB 을 사용하고
        for (auto format : formats) {                                                                           //  Default Color Space 로는 SRGB_NONLINEAR 을 사용하게 된다.
            if (format.format == vk.swapChainImageFormat && format.colorSpace == defaultSpace) {
                found = true;
                break;
            }
        }

        if (!found) {                               // Format 과 Presentation Mode 의 차이 : Presentation Mode 는 Swap Chain 의 Performance 와 관련된 내용
            throw std::runtime_error("");           //                                      Format 은 그 자체가 바뀌게 되면 전체적인 로직이 바뀐다. Format 이 바뀌면 나머지 코드 모두가 내용이 바뀌어야 한다.
        }
    }

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;            // Presentation Mode 로 VK_PRESENT_MODE_MAILBOX_KHR 가 지원 안되면 VK_PRESENT_MODE_FIFO_KHR 를 그대로 사용.
    {
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, vk.surface, &presentModeCount, nullptr);       // Swap Chain 에서 Surface 로 지원하는 Presentation Mode 가 무엇이 있는지
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);                                               //  vector 에 나열하고 Mode 로 VK_PRESENT_MODE_MAILBOX_KHR 가 지원되면 사용.
        vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, vk.surface, &presentModeCount, presentModes.data());

        for (auto mode : presentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {                              // VK_PRESENT_MODE_MAILBOX_KHR 와 같은 세부기능들은 모든 그래픽카드에서 지원하는 것은 아니기 때문에
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;                          //  반드시 해당 그래픽카드에서 지원하는지 먼저 알아보고 사용해야, 나중에 호환성 문제로 귀찮아지지 않는다.
                break;
            }
        }
    }

    uint imageCount = capabilities.minImageCount + 1;
    VkSwapchainCreateInfoKHR createInfo{                                        // Swap Chain 을 생성하기 위한 설정들
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk.surface,
        .minImageCount = imageCount,                                // imageCount 에 3 을 넣으면 만들어지는 이미지(버퍼) 개수가 최소 3개이다.
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

    vkGetSwapchainImagesKHR(vk.device, vk.swapChain, &imageCount, nullptr);                     // swapChainImages 의 크기를 imageCount 만큼으로 설정
    vk.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vk.device, vk.swapChain, &imageCount, vk.swapChainImages.data());   // swapChainImages 이 이미 내부적으로 만들어져 있는데, 그것을 가져오는 것.

    for (const auto& image : vk.swapChainImages) {
        VkImageViewCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk.swapChainImageFormat,                  // Swap Chain 생성때의 Fotmat 과 동일하게 한다.
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        VkImageView imageView;
        if (vkCreateImageView(vk.device, &createInfo, nullptr, &imageView) != VK_SUCCESS) {     // swapChainImages 에 대한 Image View 를 만듦
            throw std::runtime_error("failed to create image views!");
        }
        vk.swapChainImageViews.push_back(imageView);
    }
}

void createRenderPass()                                                 // RenderPass 는 여러 개의 Subpass 들로 이루어질 수 있다.
{
    VkAttachmentDescription colorAttachment{                            // 현재는 Color Attachment 을 하나밖에 안 쓰기에 colorAttachment 배열의 0 번 인덱스만 작성하였다.
                                                                        // 현재 이 부분(명세)에서는 Render Target 과 리소스를 bind 시켜주지 않았다. 해당 작업은 이후에 수행될 것이다.
        .format = vk.swapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,                          // VK_ATTACHMENT_LOAD_OP_CLEAR 는 Shader 를 호출하기 이전에 픽셀을 clear color 로 싹 지우는 동작
                                                                        //  VK_ATTACHMENT_LOAD_OP_LOAD 는 기존의 픽셀 color 을 지우지 않고 새로운 렌더링 결과와 블랜딩하는 데에 활용가능한 동작
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,                        // VK_ATTACHMENT_STORE_OP_STORE 는 Shader 를 호출하기 이후에 최종적인 color 를 저장하도록 동작
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentRef0{                                  // 현재 colorAttachment 가 하나뿐이기 때문에 0 번 index 이다.
        .attachment = 0,                                                // Attachment 는 Render Target 에 대한 정보를 의미한다. 0 의 의미는 현재 colorAttachment 의 Reference 이다.
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,             // Color 출력만 하기에 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL 로 layout 를 선책한다.
    };

    VkSubpassDescription subpass{                                               // fragment Shader 의 아웃풋 (Render Target)이 1가지이기 때문에 colorAttachmentCount 를 1로 정해준다.
        .colorAttachmentCount = 1,                                      // Render Target, 즉, Shader 가 호출되었을 때 동시에 출력이 몇 개 수행되는지를 정의해주고
        .pColorAttachments = &colorAttachmentRef0,                      //  그리고 출력 대상이 어떠한 Render Target (리소스) 로 이어지는지에 대한 정보를 정의해준다.
    };                                                                          // 위의 Attachment 에 대한 Reference 들을 여러개 묶어 Subpass 로 보내주면
                                                                                //  Render Target 이 여러개인 상황도 정의할 수 있다.

    VkRenderPassCreateInfo renderPassInfo{                              // Attachment (또는 배열)과 Subpass (또는 배열)이 각각 필요한데, 
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,                               // 현재는 Color Attachment 을 하나밖에 안 쓰지만, 여러 개의 Attachment 를 사용하면 배열로 엮어 시작주소를 넣는다.
        .subpassCount = 1,
        .pSubpasses = &subpass,                                         // Subpass 또한 여러 개의 Subpass 를 사용하면 배열로 엮어 시작주소를 넣는다.
    };

    if (vkCreateRenderPass(vk.device, &renderPassInfo, nullptr, &vk.renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

    for (const auto& view : vk.swapChainImageViews) {                       // Frame Buffer : 원하는 형태의 Render Target 들이 마련된 Subpass 라는 어뎁터에 호환되는 콘센트의 역할
                                                                            //                Subpass 의 Render Target, 즉, 리소스들을 한꺼번에 저장할 수 있는 Buffer
        VkFramebufferCreateInfo framebufferInfo{                            // Render Target 을 Swap Chain 을 통해 출력해서 매번 Render Target 의 결과가 달라질 것이기 때문에 그 달라지는 수만큼 Frame Buffer 가 필요하다.
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = vk.renderPass,                                    // Bind 할 RenderPass 에 대한 정보를 받음
            .attachmentCount = 1,
            .pAttachments = &view,                                          // pAttachments 와 위의 subpass 내의 pColorAttachments 가 호환되어야 bind 가 가능하다.
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

void createGraphicsPipeline()                               // State Machine 인 OpenGL 은 동적인 파이프라인이 단 한 개 존재한다. OpenGL 의 State 를 바꾸면서 각기 다른 파이프라인을 만들어 돌린다.
{                                                           //  State 들이 동적으로 바뀌기 때문에 최적화가 힘들다는 것이 OpenGL 의 단점이다.
    auto spv2shaderModule = [](const char* filename) {
        auto vsSpv = readFile(filename);                    // 반면, Vulkan 은 파이프라인을 생성할 때 미리 파이프라인 State 들을 모두 명시한다. 이 파이프라인의 State 들은 그 이후로 절대 변해서는 안되는 것이다.
        VkShaderModuleCreateInfo createInfo{                //  이는 최적화를 위해 진행되는 작업이다.
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = vsSpv.size(),                   // Shader Module 의 크기를 지정
            .pCode = (uint*)vsSpv.data(),               // Shader Module 에 unsigned int 로 형변환해서 저장해야 함. ( char 타입이었던 vsSpv )
        };

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(vk.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    };
    VkShaderModule vsModule = spv2shaderModule("simple_vs.spv");    // simple_vs.glsl       // 대표적인 파이프라인인 Graphics Pipeline 중 Vertex 
    VkShaderModule fsModule = spv2shaderModule("simple_fs.spv");    // simple_fs.glsl

    VkPipelineShaderStageCreateInfo vsStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vsModule,
        .pName = "main",                                    // Shader 의 main 함수가 진입점임을 알려줌 ( main 이 되는 함수가 a 이면, a 로 명명해도 됨 )
    };
    VkPipelineShaderStageCreateInfo fsStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fsModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vsStageInfo, fsStageInfo };      // Shader Stage 를 만들 수 있는 정보를 나중에 파이프라인을 만드는 데에 사용한다.

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,       // 현재 Shader 은 들어가는 input 이 없기 때문에 가장 간단한 형식으로 작성되었다.
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer{                              // Vertex 와 Fragment Shader 사이에 있는 Shader
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0,                                   // lineWIdth 의 값은 지정해주어야 한다.
    };

    VkPipelineMultisampleStateCreateInfo multisampling{                         // 멀티샘플링 ( 한 픽셀 당 1 개의 샘플로 설정 )
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment{                   // Fragment Shader 호출 이전의 Color 과 호출 이후의 Color 을 각각 어떻게 다룰지 설정함.
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,    // 호출 이후의 Color 로 덮어씌우기로 함
    };

    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    std::vector<VkDynamicState> dynamicStates = {       // Dynamic State : 그래픽스 파이프라인의 State 가 원래는 정적이어야 하는데, 일부분을 Dynamic State 로 지정해주면 그 일부는 변화가 허용된다.
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };

    if (vkCreatePipelineLayout(vk.device, &pipelineLayoutInfo, nullptr, &vk.pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{                          // Pipeline 을 만드는 데에 사용되는 설정들
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
        .subpass = 0,                                       // basePipelineHandle 의 용도는 이전 버전의 pipeline 과 리소스을 재활용하면서 좀 더 빨리 Pipeline 을 만들 수 있게하는 것이다. 
    };

    if (vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vk.graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");        // 그래픽스 Pipeline 을 동시에 여러 개 만들 수 있기도 하다.
    }
    
    vkDestroyShaderModule(vk.device, vsModule, nullptr);    // Pipeline 이 만들어진 후에는 메모리 확보를 위해 Shader 에 대한 메모리를 해제해주는 것이 좋다.
    vkDestroyShaderModule(vk.device, fsModule, nullptr);
}

void createCommandCenter()                                              // Command Queue 는 Device 설정에서 이미 만들었고, 이제 Command Buffer 와 Command Pool 을 만들어야 한다.
{
    VkCommandPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vk.queueFamilyIndex,
    };

    if (vkCreateCommandPool(vk.device, &poolInfo, nullptr, &vk.commandPool) != VK_SUCCESS) {    // Command Pool : Command Buffer 의 명령어들이 저장된 메모리 공간
        throw std::runtime_error("failed to create command pool!");
    }

    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk.commandPool,
        .commandBufferCount = 1,
    };
                                                                                                // Command Buffer : 실제로 내린 명령들은 이 Command Buffer 에 쌓이게 되는데 나중에 이렇게 쌓인 Buffer 는
    if (vkAllocateCommandBuffers(vk.device, &allocInfo, &vk.commandBuffer) != VK_SUCCESS) {     //                   Command Queue 에 한꺼번에 제출한다. GPU 의 명령어를 처리하는 과정에서 GPU 명령들이
        throw std::runtime_error("failed to allocate command buffers!");                        //                   Command Queue 에 쌓이는데, 이는 Command Buffer 에 모아진 명령들이 제출된 것이다.
    }
}

void createSyncObjects()                                        // Semaphore 와 Fence, 이 두가지를 제공 (동기화와 관련된 부분들) <- DirectX 와 제일 차이가 나는 부분
{
    VkSemaphoreCreateInfo semaphoreInfo{ 
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO 
    };
    VkFenceCreateInfo fenceInfo{                                // Fence 는 상태가 Signaled 와 Unsignaled, 두 가지로 나뉜다.
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    if (vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &vk.imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vk.device, &semaphoreInfo, nullptr, &vk.renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(vk.device, &fenceInfo, nullptr, &vk.inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to create synchronization objects for a frame!");
    }

}

void render()
{
    const VkClearValue clearColor = { .color = {0.0f, 0.0f, 0.0f, 1.0f} };
    const VkViewport viewport{ .width = (float)WIDTH, .height = (float)HEIGHT, .maxDepth = 1.0f };      // Viewport 크기
    const VkRect2D scissor{ .extent = {.width = WIDTH, .height = HEIGHT } };                            // 실제 크기 ( 변하지 않는 값들 )
    const VkCommandBufferBeginInfo beginInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
                                                                                                               
    vkWaitForFences(vk.device, 1, &vk.inFlightFence, VK_TRUE, UINT64_MAX);      // Wait Fence : 만든 Fence 가 Signaled 상태가 될 때까지 기다리는 작업 (이미 앞서 Signaled 상태로 만들었으니 바로 다음으로 진행)
                                                                                    // VK_TRUE 는 &vk.inFlightFence 로 받은 Fence 배열의 모든 Fence 들이 Signaled 상태이면 넘어가도록 한다.
                                                                                    // UINT64_MAX 는 나노단위이다. (Time Out) 예를 들어 1000000 이면 1초인데, 이 경우 1초 기다리다가 신호가 안 오면 다음으로 넘어간다.
    vkResetFences(vk.device, 1, &vk.inFlightFence);                             // 모든 Fance 들을 Unsignaled 상태로 바꿈

    uint32_t imageIndex;
    vkAcquireNextImageKHR(vk.device, vk.swapChain, UINT64_MAX, vk.imageAvailableSemaphore, nullptr, &imageIndex);   // Swap Chain 의 이미지들의 화면에 출력되고 있는 하나를 제외한 나머지들 중
                                                                                                                    //  하나가 이전 프레임에서 렌더링이 아직 진행 (Busy 함) 되고 있다고 할 때 그 둘의 제외한 나머지들이 있는데
                                                                                                                    //  그 나머지들 중 하나를 return 해주는 역할 ( 여기에서 렌더링하는 결과는 return 받은 그 하나에 그려진다. )
                                                                                                                    //  ( imageIndex 는 return 받은 이미지의 번째 수 )
                                                                                                                    //  ( Fence 는 안 넘겨주고 Semaphore 은 넘겨주었는데, 출력을 담당한 하나를 제외한 모든 이미지가 Busy 한 경우, 다음에 놀게 될 이미지의 인덱스를 일단은 return 하지만 현재는 못쓰는 상태 그대로 넘어간다. )
                                                                                                                    //    => 사용하려는 이미지가 언제 가용한 상태가 될 지 모르기 떄문에, 해당 이미지가 가용한 상태가 되는 순간 imageAvailableSemaphore 에 Signal 을 준다. 
    
    vkResetCommandBuffer(vk.commandBuffer, 0);                                                              // Draw Call 에 대한 명령들을 Command Buffer 에 넣는 작업 ( 먼저, Command Buffer 리셋을 진행 )
    {
        if (vkBeginCommandBuffer(vk.commandBuffer, &beginInfo) != VK_SUCCESS) {                             // Command Buffer 를 시작
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = vk.renderPass,
            .framebuffer = vk.framebuffers[imageIndex],                                             // 어떤 Frame Buffer 을 사용할 지 정함
            .renderArea = { .extent = { .width = WIDTH, .height = HEIGHT } },                       // 렌더링 영역 지정 (매번 수행해주어야 하는 모양이다.)
            .clearValueCount = 1,
            .pClearValues = &clearColor,
        };

        vkCmdBeginRenderPass(vk.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);                // Render Pass 를 시작 -> Frame Buffer 와 Render Pass 둘 다 Bind 됨.
        {
            vkCmdBindPipeline(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.graphicsPipeline);      // Graphics Pipiline 을 Bind 시킴.
            vkCmdSetViewport(vk.commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(vk.commandBuffer, 0, 1, &scissor);                                              // viewport 와 scissor 를 Dynamic State 로 지정했기 때문에 무조건 여기에서 지정해주어야 한다. 그렇지 않으면 화면에 아무것도 뜨지 않게 된다.
            vkCmdDraw(vk.commandBuffer, 3, 1, 0, 0);
        }
        vkCmdEndRenderPass(vk.commandBuffer);

        if (vkEndCommandBuffer(vk.commandBuffer) != VK_SUCCESS) {                                           // Command Buffer 를 끝냄 ( Command Buffer 에 대한 기록 (렌더링된 리소스를 기록) 을 종료 )
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
        .pWaitSemaphores = &vk.imageAvailableSemaphore,                         // 사용하려는 Swap Chain 의 이미지가 준비되지 않은 상태일 수도 있으므로 pWaitSemaphores 을 imageAvailableSemaphore 로 설정해준다.
        .pWaitDstStageMask = waitStages,                                        // Semaphore 마다 waitStage Flag 가 하나씩 대응된다. Vulkan 에서는 Semaphore 의 Signal 까지 무작정 대기하지 않고 설정에 따라 일련의 작업을 진행해놓기도 가능하다.
        .commandBufferCount = 1,
        .pCommandBuffers = &vk.commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &vk.renderFinishedSemaphore,                       // 렌더링이 완료되면 renderFinishedSemaphore 에 작업이 완료되었다는 Signal 을 보내야 한다.
    };

    if (vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, vk.inFlightFence) != VK_SUCCESS) {      // 기록이 끝난 Command Buffer 를 Present Queue (graphicsQueue) 에 제출 (Submit)
        throw std::runtime_error("failed to submit draw command buffer!");                      //  이 제출은 imageAvailableSemaphore 이 Signal 된 후에야 진행된다.
    }

    VkPresentInfoKHR presentInfo{                                                           // Present Queue 의 내용을 Present 하기 위한 설정
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vk.renderFinishedSemaphore,                                     // 렌더링이 다 끝나지 않았는데 Present 되면 안되기 때문에 renderFinishedSemaphore 를 활용한다. ( Signal 받으면 그때가 되어서야 Present )
        .swapchainCount = 1,
        .pSwapchains = &vk.swapChain,
        .pImageIndices = &imageIndex,                                                       // Present 할 이미지의 인덱스
    };

    vkQueuePresentKHR(vk.graphicsQueue, &presentInfo);                                      // Present Queue 의 내용을 Present 하라는 명령을 제출 (Submit)
}

int main() 
{
    glfwInit();                             // 플랫폼마다 다른 환경에 대한 각기 다른 설정들을 GLFW 가 도맡는데, 이 GLFW 를 초기화
    GLFWwindow* window = createWindow();    // Window 생성
    
                                            // Vulkan 의 Instance 를 먼저 만들고 Device 를 만들어야 한다.

    createVkInstance(window);               // Instance :   Vulkan 애플리케이션과 GPU(그래픽 하드웨어)를 포함한 Vulkan 드라이버 간의 최초 연결을 설정하는 핵심 객체.
                                            // 역할 : Vulkan 드라이버 초기화, 물리적 디바이스 탐색, 확장(추가 기능) 지원, 검증 레이어 활성화, Vulkan 객체 생성의 시작점
    
    createVkDevice();                       // Device   :   Vulkan 애플리케이션이 GPU의 기능을 활용하기 위해 물리적 디바이스와 연결하여 생성되는 객체
                                            // 역할 : GPU와 애플리케이션 간의 인터페이스 제공, 작업 큐 생성, 확장 활성화, 메모리 관리
                                            // Device 는 Physical Device 와 Logical Device 로 나뉜다.
                                            // Physical Device : 시스템에 설치된 실제 GPU 그 자체
                                            // Logical Device : 물리적 디바이스의 논리적 표현 ( 애플리케이션이 Vulkan API를 통해 GPU를 제어할 수 있게 함 )

    createSwapChain();                      // Swap Chain 생성
    createRenderPass();                     // Render Pass 생성
    createGraphicsPipeline();               // 그래픽스 Pipeline 설정
    createCommandCenter();
    createSyncObjects();

    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();                       // 이 상태 그대로 Debug 모드에서 실행하면, 화면을 종료하면 Error 들이 뜬다.
        render();                               // render() 호출하는 과정에서 렌더링이 완료된 리소스에 관한 Command Buffer 와 Present 하라는 명령을 Present Queue 에 집어넣었었는데
                                                //  while 문에서 빠져나오고 window 가 닫히게 되는데 이 과정에서 Queue 로 제출된 모든 리소스들이 강제적으로 종료되었기에 발생하는 오류들이다.
    }                                                                                                                                         //
                                                                                                                                             //
    vkDeviceWaitIdle(vk.device);        //////////// 따라서 GPU 의 Queue 가 모두 비워질 때까지 CPU 가 기다려주도록 이와 같이 설정해줘야 한다. <===//
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}