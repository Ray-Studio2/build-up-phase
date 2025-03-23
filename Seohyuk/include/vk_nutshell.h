#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#define DEVICE_SELECTION 0

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
        vk::ApplicationInfo appInfo = vk::ApplicationInfo("vk_nutshell");
        char * const * instanceLayers;
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
        void programLoop();


        ~VkContext_();

    } VkContext;

    inline VkContext_::VkContext_() {
        std::cout << "In a nut shell, vulkan is a Graphics API Spec." << std::endl;
        {


            instance = createInstance(
                vk::InstanceCreateInfo{
                    //vk::InstanceCreateInfo
                    {},
                    &appInfo,
                    static_cast<unsigned int>(sizeof(vecInstanceLayers) ), // enabled instnace layer count
                    vecInstanceLayers, // enabled extensions
                    static_cast<unsigned int>(sizeof(vecInstanceExtensions) ), //enabled extention count
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

    inline void VkContext_::injectGLFWWindow(GLFWwindow * pptrWindow) {
        this->window = pptrWindow;
    }


    inline void VkContext_::programLoop() {
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

        std::cout << "Nutshell says goodbye~" << std::endl; // if this doesn't happen, you are doing destroy in wrong way.
    }


    /*
     * The renderpass option implementation unit.
     *
     */
    typedef struct RenderPassBoundary_ {
    } RenderPassBoundary;
}

