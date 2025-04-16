#pragma once

#include <iostream>
#include <memory>
#include <queue>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "vulkan_utility.h"

#define DEVICE_SELECTION 0
#define PRINT_INFO


#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif


inline VkBool32 debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void * pUserData) {


    return VK_FALSE;
}

inline VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debugCallback,
};

namespace nutshell {

    /**
     * all the draw callbacks must be implemented allways.
     * it can be done like this
     *
     * void nutshell::drawCallBackMain(GLFWwindow *pWindow, vk::Instance instance, vk::Device device, vk::Queue queue) {
     *
     * }
     *
     *
     * here. copy this.
     *
    void nutshell::beforeRender() {}
    void nutshell::drawCallPreRender(GLFWwindow *pWindow, vk::Instance instance, vk::Device device, vk::Queue queue) {}
    void nutshell::whileRendering() {}
    void nutshell::drawCallBackMain(GLFWwindow *pWindow, vk::Instance instance, vk::Device device, vk::Queue queue) {}
    void nutshell::drawCallPostRender(GLFWwindow *pWindow, vk::Instance instance, vk::Device device, vk::Queue queue) {}
    void nutshell::afterRedner() {}
     *
     */


    void (beforeRender)();                                                                                     /* anything before the renderpass starts */
    //void (drawCallPreRender)(GLFWwindow *pWindow, VkInstance * instance, VkDevice * device, VkQueue * queue);  /* inside the renderpass before something renders */
    void (whileRendering)();                                                                                   /* something to dso in program loop */
    void (drawCallBackMain)(GLFWwindow *pWindow, VkInstance instance, VkDevice device, VkQueue queue);   /* main rendering stage */
    //void (drawCallPostRender)(GLFWwindow *pWindow, VkInstance * instance, VkDevice * device, VkQueue * queue); /* after the rendering inside a renderpass */
    void (afterRedner)();                                                                                      /* last thing to do in main loop */

    /**
     * Very simple Vulkan instance context with some device info and the command pool.
     **/
    typedef struct VkContext_ {
        struct {
            GLFWwindow * window = nullptr;
            VkSurfaceKHR surface = nullptr;
            VkSwapchainKHR swapChaine = nullptr;
        } PresentationUnit;

        std::vector<const char *> instanceLayerRequestList {};
        std::vector<const char *> instanceExtensionRequestList = {
            //"VK_KHR_get_physical_device_properties2",
            //"VK_KHR_get_surface_capabilities2",

            //"VK_EXT_DEBUG_UTILS_EXTENSION_NAME",
            //"VK_EXT_debug_utils",
            "VK_KHR_portability_enumeration",

#ifdef __APPLE__

#endif

        };


        VkInstance instance;

        std::vector<VkPhysicalDevice> physicalDevices{};
        VkDevice deviceUnique;
        const float queuePriorities = 1.0;
        VkQueue queueUnique;
        VkCommandPool commandPoolUnique;
        VkCommandBuffer commandBufferUnique;




        VkContext_();


        /**
         * After call this function the program will be started and loops.
         */
        void programLoop() const;


        ~VkContext_();

    } VkContext;

    inline VkContext_::VkContext_() {
        glfwInit();

        printf("GLFW Vulkan supported: %s\n", (glfwVulkanSupported() ? "YES" : "NO"));

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        PresentationUnit.window = glfwCreateWindow(1200, 800, "Vulkan Training Unit", nullptr, nullptr);

        if (!PresentationUnit.window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        if (glfwVulkanSupported() == GLFW_FALSE) {
            std::cout << "Vulkan is not supported!" << std::endl;
            exit(VK_ERROR_INITIALIZATION_FAILED);
        }

        if (enableValidationLayers) {
            this->instanceLayerRequestList.push_back("VK_LAYER_KHRONOS_validation");
        }

        uint32_t glfwRequiredInstanceExtensionsCount;
        const char **glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredInstanceExtensionsCount);

        for (uint32_t i = 0; i < glfwRequiredInstanceExtensionsCount; i += 1) {
            instanceExtensionRequestList.push_back(glfwRequiredExtensions[i]);
        }

        //vkut::checkValidationLayerSupport();
        //vkut::showInstanceExtensions();
        //vkut::showInstanceLayers();


        VkApplicationInfo appInfo {
            VK_STRUCTURE_TYPE_APPLICATION_INFO,
            //&messengerCreateInfo,
            nullptr,
            "vk_nutshell",
            0,
            "nutshell",
            0,
#ifdef __APPLE__
            VK_API_VERSION_1_2
#else
            VK_API_VERSION_1_4
#endif
        };

        VkInstanceCreateInfo instanceCreateInfo {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            nullptr,
#ifdef __APPLE__
            //VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR |
#endif
            0,
            &appInfo,
            static_cast<unsigned int>(instanceLayerRequestList.size()),
            instanceLayerRequestList.data(),
            static_cast<unsigned int>(instanceExtensionRequestList.size()),
            instanceExtensionRequestList.data()
        };

        VkInstance instance;
        vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

        if (instance == VK_NULL_HANDLE) {
            std::cerr << "Failed to create instance" << std::endl;
            exit(EXIT_FAILURE);
        }


        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
        if (physicalDeviceCount == 0) {
            std::cerr << "No device available for vulkan." << std::endl;
        }
        physicalDevices.resize(physicalDeviceCount);
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

#ifdef PRINT_INFO
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevices.at(DEVICE_SELECTION), &deviceProperties);
        std::cout << "Physical device selected: " << "number: " << DEVICE_SELECTION  << std::endl << "device name:" << deviceProperties.deviceName << std::endl;

#endif
    }


    inline void VkContext_::programLoop() const {
        while ( !glfwWindowShouldClose(PresentationUnit.window) ) {
            beforeRender();
            {
                //drawCallPreRender(PresentationUnit.window, instanceUnique.get(), deviceUnique.get(), queueUnique.get());
                {
                    whileRendering();
                    drawCallBackMain(PresentationUnit.window, instance, deviceUnique, queueUnique);;
                }
                //drawCallPostRender(PresentationUnit.window, reinterpret_cast<VkInstance>(instanceUnique.get()), *device, *queue);
            }
            afterRedner();

            glfwSwapBuffers(PresentationUnit.window);
            glfwPollEvents();


        }
    }

    inline VkContext_::~VkContext_() {
    /*
     * in case we are using unique, we don't have to free things manually.
     */

        glfwDestroyWindow(PresentationUnit.window);

        std::cout << "Nutshell says goodbye~" << std::endl; // if this doesn't happen, you are doing destroy in wrong way.
    }

}


