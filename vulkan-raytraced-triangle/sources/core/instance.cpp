#include "core/instance.hpp"

#include "glfw3.h"

#include <iostream>
#include <vector>
using std::cout, std::endl;
using std::vector;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT       severity    , VkDebugUtilsMessageTypeFlagsEXT type,
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

/* ------------ Static Variables ------------- */
bool Instance::mIsCreated = { };
VkInstance Instance::mHandle = { };

#ifdef DEBUG
VkDebugUtilsMessengerEXT Instance::mDebugMessenger = { };
#endif
/* ------------------------------------------- */

/* -------- Constructors & Destructor -------- */
Instance::Instance() { }
Instance::Instance(const char* appName, uint32 apiVer, uint32 appVer, const char* engineName, uint32 engineVer) { create(appName, appVer, apiVer, engineName, engineVer); }
Instance::~Instance() noexcept {
#ifdef DEBUG
    ((PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mHandle, "vkDestroyDebugUtilsMessengerEXT"))(mHandle, mDebugMessenger, nullptr);
#endif
    vkDestroyInstance(mHandle, nullptr);
}
/* ------------------------------------------- */

/* ---------- Operator Overloadings ---------- */
Instance::operator bool() const noexcept { return mIsCreated; }
Instance::operator VkInstance() const noexcept { return mHandle; }
/* ------------------------------------------- */

/* Return
 *   instance 생성 성공시 true
 *   instance 생성 실패시 false
*/
bool Instance::create(const char* appName, uint32 apiVer, uint32 appVer, const char* engineName, uint32 engineVer) {
    if (!mIsCreated) {
        VkApplicationInfo appInfo {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = appName,
            .applicationVersion = appVer,
            .pEngineName = engineName,
            .engineVersion = engineVer,
            .apiVersion = apiVer
        };

        uint32 glfwExtensionCount{ };
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    #ifdef DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        vector<const char*> layers;
        layers.push_back("VK_LAYER_KHRONOS_validation");

        VkDebugUtilsMessengerCreateInfoEXT debugInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback
        };
    #endif
        extensions.shrink_to_fit();

        VkInstanceCreateInfo instanceInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        #ifdef DEBUG
            .pNext = &debugInfo,
        #endif
            .pApplicationInfo = &appInfo,
        #ifdef DEBUG
            .enabledLayerCount = static_cast<uint32>(layers.size()),
            .ppEnabledLayerNames = layers.data(),
        #endif
            .enabledExtensionCount = static_cast<uint32>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };

        if (vkCreateInstance(&instanceInfo, nullptr, &mHandle) == VK_SUCCESS)
            mIsCreated = true;

    #ifdef DEBUG
        auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mHandle, "vkCreateDebugUtilsMessengerEXT");

        if ((vkCreateDebugUtilsMessengerEXT == nullptr) or (vkCreateDebugUtilsMessengerEXT(mHandle, &debugInfo, nullptr, &mDebugMessenger) != VK_SUCCESS))
            cout << "[ERROR]: vkCreateDebugUtilsMessengerEXT()" << endl;
    #endif
    }

    return mIsCreated;
}