#pragma once

#define NDBUG


#include <iostream>
#include <memory>
#include <queue>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "vulkan_utility.h"

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


    void (beforeRender)();                                                                                     /* anything before the renderpass starts */
    //void (drawCallPreRender)(GLFWwindow *pWindow, VkInstance * instance, VkDevice * device, VkQueue * queue);  /* inside the renderpass before something renders */
    void (whileRendering)();                                                                                   /* something to dso in program loop */
    void (drawCallBackMain)(GLFWwindow *pWindow, VkInstance * instance, VkDevice * device, VkQueue * queue);   /* main rendering stage */
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

        std::vector<const char *> instanceLayerRequestList = {
            "VK_LAYER_KHRONOS_validation"
        };
        std::vector<const char *> instanceExtensionRequestList = {
            "VK_KHR_get_physical_device_properties2",
        };


        std::unique_ptr<VkInstance> instanceUnique = std::make_unique<VkInstance>();

        std::vector<VkPhysicalDevice> physicalDevices;
        std::unique_ptr<VkDevice> deviceUnique = std::make_unique<VkDevice>();
        const float queuePriorities = 1.0;
        std::unique_ptr<VkQueue> queueUnique = std::make_unique<VkQueue>();
        std::unique_ptr<VkCommandPool> commandPoolUnique = std::make_unique<VkCommandPool>();
        std::unique_ptr<VkCommandBuffer> commandBufferUnique = std::make_unique<VkCommandBuffer>();




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

        PresentationUnit.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Training Unit", nullptr, nullptr);

        if (!PresentationUnit.window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        if (glfwVulkanSupported() == GLFW_FALSE) {
            std::cout << "Vulkan is not supported!" << std::endl;
            exit(VK_ERROR_INITIALIZATION_FAILED);
        }

        uint32_t glfwRequiredInstanceExtensionsCount;
        const char **glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredInstanceExtensionsCount);


        for (uint32_t i = 0; i < glfwRequiredInstanceExtensionsCount; i += 1) {
            instanceExtensionRequestList.push_back(glfwRequiredExtensions[i]);
        }


        VkApplicationInfo appInfo{
            VK_STRUCTURE_TYPE_APPLICATION_INFO,
            nullptr,
            "vk_nutshell",
            0,
            "nutshell",
            0,
            VK_API_VERSION_1_3
        };

        VkInstanceCreateInfo instanceCreateInfo {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            nullptr,
            0,
            &appInfo,
            static_cast<unsigned int>(instanceLayerRequestList.capacity()),
            instanceLayerRequestList.data(),
            static_cast<unsigned int>(instanceExtensionRequestList.capacity()),
            instanceExtensionRequestList.data()
        };

        vkCreateInstance(&instanceCreateInfo, nullptr, this->instanceUnique.get());
    }


    inline void VkContext_::programLoop() const {
        while ( !glfwWindowShouldClose(PresentationUnit.window) ) {
            beforeRender();
            {
                //drawCallPreRender(PresentationUnit.window, instanceUnique.get(), deviceUnique.get(), queueUnique.get());
                {
                    whileRendering();
                    drawCallBackMain(PresentationUnit.window, instanceUnique.get(), deviceUnique.get(), queueUnique.get());;
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


