#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "vulkan_utility.h"

#define DEVICE_SELECTION 0
#define QUEUE_PRIORITY 1.0f
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
    typedef struct Synchronizer_ {
        VkFence fence;
        VkSemaphore semaphore;
    } Synchronizer;

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

<<<<<<< HEAD
    void (whileRendering)();                                                                                   /* something to do in program loop */
    void (drawCallBackMain)(GLFWwindow *pWindow, VkInstance instance, VkDevice device, VkQueue queue, VkCommandBuffer commandBuffer, Synchronizer synchronizer);   /* main rendering stage */
=======
    void (initFinished)(VkInstance instance, VkDevice device, );
    void (whileRendering)();                                                                                   /* something to do in program loop */
    void (drawCallBackMain)(GLFWwindow *pWindow, VkInstance instance, VkDevice device, VkQueue queue, Synchronizer synchronizer);   /* main rendering stage */
>>>>>>> ab3c14c (all)


    /**
     * Very simple Vulkan instance context with some device info and the command pool.
     **/
    typedef struct VkContext_ {
        struct {
            GLFWwindow * window = nullptr;
            VkSurfaceKHR surface = nullptr;
            VkSwapchainKHR swapChaine = nullptr;
            std::vector<VkImage> swapChainImages{};
            std::vector<VkImageView> swapChainImageViews{};
            std::vector<VkFramebuffer> framebuffer{};
        } PresentationUnit;

        std::vector<const char *> instanceLayerRequestList {
        };
        std::vector<const char *> instanceExtensionRequestList = {
            "VK_KHR_portability_enumeration",

#ifdef __APPLE__
            "VK_KHR_portability_subset"
#endif

        };


        VkInstance instance;

        std::vector<VkPhysicalDevice> physicalDevices{};
        VkDevice device;
        const float queuePriorities = 1.0;
        VkQueue queue;
        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;

        Synchronizer synchronizer = {};

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
            //this->instanceLayerRequestList.push_back("VK_LAYER_KHRONOS_validation");
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
            VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR |
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



        uint32_t queueFamilySelection = vkut::autoSelectQueueFamily(physicalDevices.at(DEVICE_SELECTION), DEVICE_SELECTION);


        float priority = QUEUE_PRIORITY;
        VkDeviceQueueCreateInfo queueCreateInfo = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
            0,
            queueFamilySelection,
            1,
            &priority
        };

        std::vector<const char *>  deviceExtensionRequestList = {
            "VK_KHR_swapchain"
        };

        VkDeviceCreateInfo deviceCreateInfo {
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            nullptr,
            0,
            1,
            &queueCreateInfo,
            0, // deprecated
            {}, // deprecated
            static_cast<unsigned int>(deviceExtensionRequestList.size()),
            deviceExtensionRequestList.data(),
            {}
        };

        vkCreateDevice(physicalDevices.at(DEVICE_SELECTION), &deviceCreateInfo, nullptr, &device);

        if (device == VK_NULL_HANDLE) {
            std::cerr << "Failed to create device" << std::endl;
        }

        VkCommandPoolCreateInfo cmdPoolCreateInfo {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,

        };
        vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &commandPool);

        if (commandPool == VK_NULL_HANDLE) {
            std::cerr << "Failed to create command pool" << std::endl;
        }

        if (glfwCreateWindowSurface(instance, PresentationUnit.window, nullptr, &PresentationUnit.surface)) {
            std::cerr << "Failed to create surface" << std::endl;
        }

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            nullptr,
            commandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            1
        };
        vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);

        vkut::SurfaceInfo surfaceInfo = vkut::getSurfaceInfo(physicalDevices.at(DEVICE_SELECTION), PresentationUnit.surface, nullptr);

        VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr,
        0,
        PresentationUnit.surface,
            2,
            surfaceInfo.imageFormat,
            surfaceInfo.imageColorSpace,
            surfaceInfo.imageExtent,
            surfaceInfo.imageArrayLayerCount,
            surfaceInfo.imageUsage,
            VK_SHARING_MODE_EXCLUSIVE,

            1,
            &queueFamilySelection,
            surfaceInfo.preTransform,
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_PRESENT_MODE_FIFO_KHR,
            VK_TRUE,
            VK_NULL_HANDLE,

        };
        vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &PresentationUnit.swapChaine);

        uint32_t imageCount;
        vkGetSwapchainImagesKHR(device, PresentationUnit.swapChaine, &imageCount, nullptr);
        std::vector<VkImage> swapchainImages(imageCount);
        vkGetSwapchainImagesKHR(device, PresentationUnit.swapChaine, &imageCount, swapchainImages.data());


        std::vector<VkImageView> swapchainImageViews(imageCount);
        for (int i = 0; i < imageCount; i += 1) {
            VkImageViewCreateInfo swapchainImageViewCreateInfo = {
                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                nullptr,
                0,
                swapchainImages.at(i),
                VK_IMAGE_VIEW_TYPE_2D,
                surfaceInfo.imageFormat,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_IMAGE_ASPECT_COLOR_BIT,
                0,
                1,
                0,
                1
            };

            VkImageView swapchainImageView;
            vkCreateImageView(device, &swapchainImageViewCreateInfo, nullptr, &swapchainImageView);

            swapchainImageViews.push_back(swapchainImageView);
        }


        //RenderPass
        //Pieline



        for (uint32_t i = 0; i < imageCount; i += 1) {

            /**
             *
            *VkFramebufferCreateFlags    flags;
    VkRenderPass                renderPass;
    uint32_t                    attachmentCount;
    const VkImageView*          pAttachments;
    uint32_t                    width;
    uint32_t                    height;
    uint32_t                    layers;
             */
            VkFramebufferCreateInfo swapchainFramebufferCreateInfo = {
                VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                nullptr,
                0,
                nullptr,

            };
        }

        VkFenceCreateInfo fenceCreateInfo = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        nullptr,
            VK_FENCE_CREATE_SIGNALED_BIT
        };
        vkCreateFence(device, &fenceCreateInfo, nullptr, &synchronizer.fence);


    }


    inline void VkContext_::programLoop() const {
        while ( !glfwWindowShouldClose(PresentationUnit.window) ) {

            {
                {
                    whileRendering();
                    drawCallBackMain(PresentationUnit.window, instance, device, queue, commandBuffer, synchronizer);
                }
            }


            glfwSwapBuffers(PresentationUnit.window);
            glfwPollEvents();


        }
    }

    inline VkContext_::~VkContext_() {
        for (const auto imageView: PresentationUnit.swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        for (const auto image: PresentationUnit.swapChainImages) {
            vkDestroyImage(device, image, nullptr);
        }

        vkDestroyFence(device, synchronizer.fence, nullptr);

        vkDestroySwapchainKHR(device, PresentationUnit.swapChaine, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);


        glfwDestroyWindow(PresentationUnit.window);


        //vkDestroySurfaceKHR(instance, PresentationUnit.surface, nullptr);
        //vkDestroyInstance(instance, nullptr);

        std::cout << "Nutshell says goodbye~" << std::endl; // if this doesn't happen, you are doing destroy in wrong way.
    }


    /* For the future use
    typedef struct RenderingInstruction_ {
        VkDevice device;
        VkCommandBuffer commandBuffer;
        VkPipeline pipeline;
        VkRenderPass renderPass;


        void setDevice(VkDevice const *device);

        void allocCommandBuffer(const VkCommandPool * commandPool);
        void createPipline();

        void createRenderPass(VkFormat swapChainImageFormat) ;

        void drawCall();

        void cleanup() const;
    } RenderingInstruction;

    inline void RenderingInstruction_::setDevice(VkDevice const* device) {
        this->device = *device;
    }

    inline void RenderingInstruction_::allocCommandBuffer(const VkCommandPool * commandPool) {
        const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            nullptr,
            *commandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            1
        };
        vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
    }

    inline void RenderingInstruction_::createPipline() {

    }

    inline void RenderingInstruction_::createRenderPass(const VkFormat swapChainImageFormat) {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;


        VkRenderPassCreateInfo renderPassCreateInfo = {
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,
            0,
            1,
            &colorAttachment,
            1,
            &subpass,
            0,
            nullptr
        };

        vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
    }

    inline void RenderingInstruction_::cleanup() const {
        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
    }
<<<<<<< HEAD
    */
=======

    inline void RenderingInstruction_::drawCall() {
        VkCommandBufferBeginInfo commandBeginInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            nullptr,
            0,
            nullptr
          };
        vkBeginCommandBuffer(commandBuffer, &commandBeginInfo);

        vkEndCommandBuffer(commandBuffer);

        vkResetCommandBuffer(commandBuffer, 0);
    }
>>>>>>> ab3c14c (all)
}


