#pragma once

#include "vulkan_utility.h"
#include <iostream>
#include <GLFW/glfw3.h>


#define DEVICE_SELECTION 0

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

    /**
     * Very simple Vulkan instance context with some device info and the command pool.
     **/
    typedef struct VkContext_ {
        struct {
            GLFWwindow * window = nullptr;
            vk::SurfaceKHR surface;
            vk::SwapchainKHR swapchaine;
        } PresentationUnit;

        vk::ApplicationInfo appInfo = vk::ApplicationInfo("vk_nutshell", 0, "nutshell", 0);
        vk::Instance instance;
        std::vector<vk::PhysicalDevice> physicalDevices;
        vk::Device device;
        const float queuePriorities = 1.0;
        vk::Queue queue;
        vk::CommandPool commandPool;




        VkContext_();


        void injectGLFWWindow(GLFWwindow * pptrWindow);

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

        PresentationUnit.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Training Unit",  NULL, NULL);
        if (!PresentationUnit.window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        if ( glfwVulkanSupported() == GLFW_FALSE ) {
            std::cout << "Vulkan is not supported!" << std::endl;
            exit(VK_ERROR_INITIALIZATION_FAILED);
        }

        uint32_t glfwRequiredInstanceExtensionsCount;
        const char** glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredInstanceExtensionsCount);


        std::vector<const char *> instanceLayerRequestList = {
            "VK_LAYER_KHRONOS_validation"

            //"VK_LAYER_NV_optimus"
        };
        std::vector<const char *> instanceExtensionRequestList = {
            "VK_KHR_get_physical_device_properties2",
            "VK_KHR_get_surface_capabilities2",
            "VK_KHR_portability_enumeration",
        };

        for (uint32_t i = 0; i < glfwRequiredInstanceExtensionsCount; i += 1) {
            instanceExtensionRequestList.push_back(glfwRequiredExtensions[i]);
        }

        {
            instance = vk::createInstance(
                vk::InstanceCreateInfo{
                        {
                            vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR
                        },
                    &appInfo,


                    static_cast<unsigned int>(instanceLayerRequestList.capacity()),
                    instanceLayerRequestList.data(),


                    static_cast<unsigned int>(instanceExtensionRequestList.capacity()),
                    instanceExtensionRequestList.data(),
                }
            );
        }

        std::cout << "In a nut shell, vulkan is a Graphics API Spec." << std::endl;

        physicalDevices = instance.enumeratePhysicalDevices();

        VkSurfaceKHR surface;
        VkResult error = glfwCreateWindowSurface(instance, PresentationUnit.window, nullptr, &surface);
        if (error) {
            std::cerr << "Failed to create window surface. error code: " << error << std::endl;
        }
        PresentationUnit.surface = vk::SurfaceKHR { surface };


        uint32_t queueFamilyFoundedIndex = 0;
        {
            uint32_t q = 0;
            for (const auto vec_qf = physicalDevices.at(DEVICE_SELECTION).getQueueFamilyProperties(); vk::QueueFamilyProperties queueFamily: vec_qf) {
                /*
                 * Queue family needs to support transfer, graphics.
                 */ if (queueFamily.queueCount >= 1 && queueFamily.queueFlags | vk::QueueFlagBits::eGraphics && queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) {
                        break;
                    }

                    q += 1;
            }
            queueFamilyFoundedIndex = q;
        }




        const auto devQueueCreateInfo = vk::DeviceQueueCreateInfo {
            {},
            queueFamilyFoundedIndex,
            1, //one Queue
            &queuePriorities
        };

        const std::vector<const char *> deviceEnabledLayers = {
        };
        std::vector<const char *> deviceEnabledExtensions = {
            //"VK_KHR_swapchain"
        };

        for (const auto p : physicalDevices.at(DEVICE_SELECTION).enumerateDeviceExtensionProperties()) {
            if (strcmp(p.extensionName, "VK_KHR_portability_subset")) {
                deviceEnabledExtensions.push_back("VK_KHR_portability_subset");
                break;
            }
        }



        device = physicalDevices.at(DEVICE_SELECTION).createDevice(
            vk::DeviceCreateInfo{
                {}, //flags
                1, //queue create info count
                &devQueueCreateInfo,

                static_cast<unsigned int>(deviceEnabledLayers.capacity()), // enabled extension count
                deviceEnabledLayers.data(), // enabled extensions

                static_cast<unsigned int>(deviceEnabledExtensions.capacity()), //enabled layer name count,
                deviceEnabledExtensions.data() // enabled layer names
            }
        );


        queue = device.getQueue(
            queueFamilyFoundedIndex, //Queue family index
            0 // Queue index
        );

        PresentationUnit.swapchaine = device.createSwapchainKHR(vkut::display::createSwapchainInfo(physicalDevices[DEVICE_SELECTION], PresentationUnit.surface, queueFamilyFoundedIndex));

    }



    inline void VkContext_::programLoop() const {
        while ( !glfwWindowShouldClose(PresentationUnit.window) ) {
            beforeRender();
            {
                drawCallPreRender(PresentationUnit.window, instance, device, queue);
                {
                    whileRendering();
                    drawCallBackMain(PresentationUnit.window, instance, device, queue);
                }
                drawCallPostRender(PresentationUnit.window, instance, device, queue);
            }
            afterRedner();



            if ( PresentationUnit.window != nullptr ) {
            } else {
                //offscreen
            }

            glfwSwapBuffers(PresentationUnit.window);
            glfwPollEvents();


        }
    }

    inline VkContext_::~VkContext_() {

        device.destroySwapchainKHR(PresentationUnit.swapchaine);
        device.destroy();
        instance.destroy();

        glfwDestroyWindow(PresentationUnit.window);

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
