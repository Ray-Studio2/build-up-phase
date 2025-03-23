#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#define DEVICE_SELECTION 0

namespace nutshell {
    constexpr char * const vecInstanceLayers[] = {
        //R"(VK_LAYER_KHRONOS_validation)"
    };

    constexpr char * const vecInstanceExtensions[] = {
        //R"(VK_EXT_debug_report)"
    };

    /**
     * Very simple Vulkan instance context with some device info and the command pool.
     **/
    typedef struct VkContext_ {
        vk::ApplicationInfo appInfo = vk::ApplicationInfo("vk_nutshell");

        vk::Instance instance;
        std::vector<vk::PhysicalDevice> physicalDevices;
        vk::Device device;
        const float queuePriorities = 1.0;
        vk::Queue queue;
        vk::CommandPool commandPool;

        GLFWwindow*window = nullptr;

        void(drawCallback)(GLFWwindow* pWindow, vk::Instance instance, vk::Device device, vk::Queue queue);

        VkContext_();


        void injectGLFWWindow(GLFWwindow * pptrWindow);

        /**
         * After call this function the program will be started and loops.
         */
        void programLoop();


        ~VkContext_();

    } VkContext;

    inline VkContext_::VkContext_() {


        std::cout << "In a nut shell, vulkan is a Graphics API Spec." << std::endl;

        {


            /*
             * Functional filter to filter the supported ones.
             */



            instance = createInstance(
                vk::InstanceCreateInfo{
                    //vk::InstanceCreateInfo
                    {},
                    &appInfo,
                    static_cast<unsigned int>(sizeof(vecInstanceLayers) - 1), // enabled instnace layer count
                    vecInstanceLayers, // enabled extensions
                    static_cast<unsigned int>(sizeof(vecInstanceExtensions) - 1), //enabled extention count
                    vecInstanceExtensions// enabled extensions
                }
            );
        }

        physicalDevices = instance.enumeratePhysicalDevices();




        constexpr uint32_t queueFamilyIndex = 0;

        {
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
        }

        {
            const auto devQueueCreateInfo = vk::DeviceQueueCreateInfo{
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
        }

        queue = device.getQueue(
            queueFamilyIndex, //Queue family index
            0 // Queue index
        );

        std::cout << "ready to render. please inject glfw window if you wish to render on screen." << std::endl;
    }

    inline void VkContext::injectGLFWWindow(GLFWwindow * pptrWindow) {
        this->window = pptrWindow;
    }


    inline void VkContext::programLoop() {
        while ( !glfwWindowShouldClose(window) ) {
            glfwSwapBuffers(window);

            if ( window != nullptr ) {
                drawCallback(window, instance, device, queue);
            } else {
                //blank screen
            }

            glfwPollEvents();
        }
    }


    inline VkContext_::~VkContext_() {

        device.destroy();
        instance.destroy();

        std::cout << "Nutshell says goodbye~" << std::endl; // if this doesn't happen, you are doing destroy in wrong way.
    }


}

