#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "vulkan_utility.h"

#define DEVICE_SELECTION 0
#define INSTANCE_LAYER_COUNT 20

#define WIDTH 800
#define HEIGHT 600

#define NDBUG

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


    void (beforeRender)();                                                                                     /* anything before the renderpass starets */
    void (drawCallPreRender)(GLFWwindow *pWindow, vk::Instance instance, vk::Device device, vk::Queue queue);  /* inside the renderpass before something renders */
    void (whileRendering)();                                                                                   /* something to dso in program loop */
    void (drawCallBackMain)(GLFWwindow *pWindow, vk::Instance instance, vk::Device device, vk::Queue queue);   /* main rendering stage */
    void (drawCallPostRender)(GLFWwindow *pWindow, vk::Instance instance, vk::Device device, vk::Queue queue); /* after the rendering inside a renderpass */
    void (afterRedner)();                                                                                      /* last thing to do in main loop */


    std::vector<const char *> instanceLayerRequestList = {
        "VK_LAYER_KHRONOS_validation",


    };


    std::vector<const char *> instanceExtensionRequestList = {
        // "VK_KHR_portability_enumeration",
    };


    /**
     * Very simple Vulkan instance context with some device info and the command pool.
     **/
    typedef struct VkContext_ {
        vk::ApplicationInfo appInfo = vk::ApplicationInfo("vk_nutshell", 0, "nutshell", 0);
        char * const * instanceLayers[INSTANCE_LAYER_COUNT][255] = {};
        vk::Instance instance;
        std::vector<vk::PhysicalDevice> physicalDevices;
        vk::Device device;
        const float queuePriorities = 1.0;
        vk::Queue queue;
        vk::CommandPool commandPool;

        GLFWwindow*window = nullptr;


        VkContext_();


        void injectGLFWWindow(GLFWwindow * pptrWindow);

        /**
         * After call this function the program will be started and loops.
         */
        void programLoop() const;


        ~VkContext_();

    } VkContext;

    inline VkContext_::VkContext_() {

        {
            glfwInit();
            printf("GLFW Vulkan supported: %s\n", (glfwVulkanSupported() ? "YES" : "NO"));
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);


        if ( glfwVulkanSupported() == GLFW_FALSE ) {
            std::cout << "Vulkan is not supported!" << std::endl;
            exit(VK_ERROR_INITIALIZATION_FAILED);
        }

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Training Unit",  NULL, NULL);
        {
            if (!window) {
                std::cerr << "Failed to create GLFW window" << std::endl;
                glfwTerminate();
                exit(EXIT_FAILURE);
            }

            glfwMakeContextCurrent(window);
        }


        //For now we will just use layers directly in the vector initialization.
        uint32_t instanceLayerCount = instanceLayerRequestList.capacity();
        const char** instanceLayerNames = nullptr;
        instanceLayerRequestList.data();

        for (uint32_t i = 0; i < instanceLayerCount; i += 1) {
            auto layerName = std::string(instanceLayerNames[i]);
            instanceLayerRequestList.push_back(layerName.data());
        }




        uint32_t minimum_extension_count = instanceLayerRequestList.capacity();
        const char ** instanceExtensions = glfwGetRequiredInstanceExtensions(&minimum_extension_count);
        {   // extensions
            for (uint32_t i = 0; i < minimum_extension_count; i += 1) {
                auto extension = std::string(instanceExtensions[i]);
                instanceExtensionRequestList.push_back(instanceExtensions[i]);
            }
            instanceExtensionRequestList.push_back("VK_EXT_debug_report");

            if (instanceExtensions == nullptr) {
                instanceExtensionRequestList.push_back("");
            }
        }

        instance = createInstance(
            vk::InstanceCreateInfo{
                vk::InstanceCreateFlags (

                ),
                &appInfo,


                static_cast<unsigned int>(instanceLayerRequestList.capacity()), //enabled extention count
                instanceExtensionRequestList.data(),


                static_cast<unsigned int>(instanceExtensionRequestList.capacity()),
                instanceExtensionRequestList.data(),
            }
        );


        std::cout << "In a nut shell, vulkan is a Graphics API Spec." << std::endl;



        physicalDevices = instance.enumeratePhysicalDevices();

        VkSurfaceKHR surface;
        auto error = glfwCreateWindowSurface(instance, window, nullptr, &surface);

        if (error) {
            std::cerr << "Failed to create window surface. error code: " << error << std::endl;
        }


        constexpr uint32_t queueFamilyIndex = 0;


        int a = 0;
        for (const auto vecQueueFamilyIndex = physicalDevices.at(DEVICE_SELECTION).getQueueFamilyProperties();
             vk::QueueFamilyProperties queueFamily: vecQueueFamilyIndex) {
            /*
             * Queue family needs to support transfer, graphics.
             */
            if (queueFamily.queueCount >= 1 && queueFamily.queueFlags | vk::QueueFlagBits::eGraphics && queueFamily.
                queueFlags & vk::QueueFlagBits::eTransfer) {
                break;
            }

            a += 1;
        }

        const auto devQueueCreateInfo = vk::DeviceQueueCreateInfo {
            {},
            queueFamilyIndex,
            1, //one Queue
            &queuePriorities
        };

        device = physicalDevices.at(DEVICE_SELECTION).createDevice(
            vk::DeviceCreateInfo{
                {}, //flags
                1, //queue create info count
                &devQueueCreateInfo,
                0, //enabled layer name count,
                {}, // enabled layer names
                0, // enabled extension count
                {} // enabled extensions
            }
        );


        queue = device.getQueue(
            queueFamilyIndex, //Queue family index
            0 // Queue index
        );
    }



    inline void VkContext_::programLoop() const {
        while ( !glfwWindowShouldClose(window) ) {
            beforeRender();
            {
                drawCallPreRender(window, instance, device, queue);
                {
                    whileRendering();
                    drawCallBackMain(window, instance, device, queue);
                }
                drawCallPostRender(window, instance, device, queue);
            }
            afterRedner();



            if ( window != nullptr ) {
            } else {
                //offscreen
            }

            glfwSwapBuffers(window);
            glfwPollEvents();


        }
    }

    inline VkContext_::~VkContext_() {

        device.destroy();
        instance.destroy();

        glfwDestroyWindow(window);

        std::cout << "Nutshell says goodbye~" << std::endl; // if this doesn't happen, you are doing destroy in wrong way.
    }


    /*
     * The renderpass option implementation unit.
     *
     */
    typedef struct RenderPassBoundary_ {
    } RenderPassBoundary;
}






/*
            uint32_t enumerate = 0;
            for (const auto element: instanceLayerRequestList) {
                std::cout << element << (enumerate >= instanceLayerRequestList.capacity() ? ", " : "") << std::endl;
                enumerate += 1;
            }
            */


/*
vkut::filterSupportedInstanceLayers(instanceLayerRequestList);

const char* filteredInstanceLayerList[INSTANCE_LAYER_COUNT] = {};

uint32_t index = 0;
for (auto layerName: instanceLayerRequestList) {
    filteredInstanceLayerList[index] = layerName;
    index += 1;
}
*/
