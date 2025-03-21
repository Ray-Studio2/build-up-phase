#include "Vulkan.h"
#include "shader_module.h"
#include "RenderSettings.h"
#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/imgui_impl_glfw.h"
#include "ThirdParty/imgui/imgui_impl_vulkan.h"

using namespace A3;

VulkanRendererBackend::VulkanRendererBackend( GLFWwindow* window )
{
    createVkInstance();
    createVkSurface( window );
    createVkDevice();
    loadDeviceExtensionFunctions( device );
    createSwapChain();
    createCommandCenter();
    createSyncObjects();
}

VulkanRendererBackend::~VulkanRendererBackend()
{
    vkDeviceWaitIdle( device );

    vkDestroyBuffer( device, tlasBuffer, nullptr );
    vkFreeMemory( device, tlasBufferMem, nullptr );
    vkDestroyAccelerationStructureKHR( device, tlas, nullptr );

    vkDestroyBuffer( device, blasBuffer, nullptr );
    vkFreeMemory( device, blasBufferMem, nullptr );
    vkDestroyAccelerationStructureKHR( device, blas, nullptr );

    vkDestroyImageView( device, outImageView, nullptr );
    vkDestroyImage( device, outImage, nullptr );
    vkFreeMemory( device, outImageMem, nullptr );

    vkDestroyBuffer( device, uniformBuffer, nullptr );
    vkFreeMemory( device, uniformBufferMem, nullptr );

    vkDestroyBuffer( device, sbtBuffer, nullptr );
    vkFreeMemory( device, sbtBufferMem, nullptr );

    vkDestroyPipeline( device, pipeline, nullptr );
    vkDestroyPipelineLayout( device, pipelineLayout, nullptr );
    vkDestroyDescriptorSetLayout( device, descriptorSetLayout, nullptr );

    vkDestroyDescriptorPool( device, descriptorPool, nullptr );


    vkDestroySemaphore( device, renderFinishedSemaphore, nullptr );
    vkDestroySemaphore( device, imageAvailableSemaphore, nullptr );
    vkDestroyFence( device, fence0, nullptr );

    vkDestroyCommandPool( device, commandPool, nullptr );

    // for (auto imageView : swapChainImageViews) {
    //     vkDestroyImageView(device, imageView, nullptr);
    // }
    vkDestroySwapchainKHR( device, swapChain, nullptr );
    vkDestroyDevice( device, nullptr );
    if( ON_DEBUG )
    {
        ( ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" ) )
            ( instance, debugMessenger, nullptr );
    }
    vkDestroySurfaceKHR( instance, surface, nullptr );
    vkDestroyInstance( instance, nullptr );
}

void VulkanRendererBackend::beginFrame()
{

    vkWaitForFences( device, 1, &fence0, VK_TRUE, UINT64_MAX );
    vkResetFences( device, 1, &fence0 );

    vkAcquireNextImageKHR( device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex );

    vkResetCommandBuffer( commandBuffer, 0 );
    if( vkBeginCommandBuffer( commandBuffer, &beginInfo ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to begin recording command buffer!" );
    }
}

void VulkanRendererBackend::endFrame()
{
    if( vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to record command buffer!" );
    }

    VkSemaphore waitSemaphores[] = {
        imageAvailableSemaphore
    };
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = sizeof( waitSemaphores ) / sizeof( waitSemaphores[ 0 ] ),
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };

    if( vkQueueSubmit( graphicsQueue, 1, &submitInfo, fence0 ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to submit draw command buffer!" );
    }

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &swapChain,
        .pImageIndices = &imageIndex,
    };

    vkQueuePresentKHR( graphicsQueue, &presentInfo );
}

void VulkanRendererBackend::beginRaytracingPipeline()
{
    vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline );
    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        pipelineLayout, 0, 1, &descriptorSet, 0, 0 );

    vkCmdTraceRaysKHR(
        commandBuffer,
        &rgenSbt,
        &missSbt,
        &hitgSbt,
        &callSbt,
        RenderSettings::screenWidth, RenderSettings::screenHeight, 1 );

    setImageLayout(
        commandBuffer,
        outImage,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        subresourceRange );

    setImageLayout(
        commandBuffer,
        swapChainImages[ imageIndex ],
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange );

    vkCmdCopyImage(
        commandBuffer,
        outImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        swapChainImages[ imageIndex ], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copyRegion );

    setImageLayout(
        commandBuffer,
        outImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_GENERAL,
        subresourceRange );

    setImageLayout(
        commandBuffer,
        swapChainImages[ imageIndex ],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        subresourceRange );
}

void VulkanRendererBackend::initImgui( GLFWwindow* window )
{
    return;
    //1: create descriptor pool for IMGUI
// the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size( pool_sizes );
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
    vkCreateDescriptorPool( device, &pool_info, nullptr, &imguiPool );

    /*VkRenderPassCreateInfo2 renderPass_info = {
        .attachmentCount = 1,
        .flags = VkRenderPassCreateFlags::
    }
    vkCreateRenderPass2KHR(device, &renderPass_info )*/

    // 2: initialize imgui library

    //this initializes the core structures of imgui
    ImGui::CreateContext();

    //this initializes imgui for SDL
    ImGui_ImplGlfw_InitForVulkan( window, true );

    //this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = device;
    init_info.Queue = graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    //init_info.RenderPass = 

    ImGui_ImplVulkan_Init( &init_info );

    ////execute a gpu command to upload imgui font textures
    //immediate_submit( [ & ]( VkCommandBuffer cmd )
    //                  {
    //                      ImGui_ImplVulkan_CreateFontsTexture( cmd );
    //                  } );

    ////clear font textures from cpu data
    //ImGui_ImplVulkan_DestroyFontUploadObjects();

    ////add the destroy the imgui created structures
    //_mainDeletionQueue.push_function( [ = ]()
    //                                  {

    //                                      vkDestroyDescriptorPool( _device, imguiPool, nullptr );
    //                                      ImGui_ImplVulkan_Shutdown();
    //                                  } );
}

void VulkanRendererBackend::loadDeviceExtensionFunctions( VkDevice device )
{
    vkGetBufferDeviceAddressKHR = ( PFN_vkGetBufferDeviceAddressKHR )( vkGetDeviceProcAddr( device, "vkGetBufferDeviceAddressKHR" ) );
    vkGetAccelerationStructureDeviceAddressKHR = ( PFN_vkGetAccelerationStructureDeviceAddressKHR )( vkGetDeviceProcAddr( device, "vkGetAccelerationStructureDeviceAddressKHR" ) );
    vkCreateAccelerationStructureKHR = ( PFN_vkCreateAccelerationStructureKHR )( vkGetDeviceProcAddr( device, "vkCreateAccelerationStructureKHR" ) );
    vkDestroyAccelerationStructureKHR = ( PFN_vkDestroyAccelerationStructureKHR )( vkGetDeviceProcAddr( device, "vkDestroyAccelerationStructureKHR" ) );
    vkGetAccelerationStructureBuildSizesKHR = ( PFN_vkGetAccelerationStructureBuildSizesKHR )( vkGetDeviceProcAddr( device, "vkGetAccelerationStructureBuildSizesKHR" ) );
    vkCmdBuildAccelerationStructuresKHR = ( PFN_vkCmdBuildAccelerationStructuresKHR )( vkGetDeviceProcAddr( device, "vkCmdBuildAccelerationStructuresKHR" ) );
    vkCreateRayTracingPipelinesKHR = ( PFN_vkCreateRayTracingPipelinesKHR )( vkGetDeviceProcAddr( device, "vkCreateRayTracingPipelinesKHR" ) );
    vkGetRayTracingShaderGroupHandlesKHR = ( PFN_vkGetRayTracingShaderGroupHandlesKHR )( vkGetDeviceProcAddr( device, "vkGetRayTracingShaderGroupHandlesKHR" ) );
    vkCmdTraceRaysKHR = ( PFN_vkCmdTraceRaysKHR )( vkGetDeviceProcAddr( device, "vkCmdTraceRaysKHR" ) );

    VkPhysicalDeviceProperties2 deviceProperties2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &rtProperties,
    };
    vkGetPhysicalDeviceProperties2( physicalDevice, &deviceProperties2 );

    if( rtProperties.shaderGroupHandleSize != RenderSettings::shaderGroupHandleSize )
    {
        throw std::runtime_error( "shaderGroupHandleSize must be 32 mentioned in the vulakn spec (Table 69. Required Limits)!" );
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRendererBackend::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData )
{
    const char* severity;
    switch( messageSeverity )
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severity = "[Verbose]"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severity = "[Warning]"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: severity = "[Error]"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: severity = "[Info]"; break;
        default: severity = "[Unknown]";
    }

    const char* types;
    switch( messageType )
    {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: types = "[General]"; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: types = "[Performance]"; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: types = "[Validation]"; break;
        default: types = "[Unknown]";
    }

    std::cout << "[Debug]" << severity << types << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

bool VulkanRendererBackend::checkValidationLayerSupport( std::vector<const char*>& reqestNames )
{
    uint32_t count;
    vkEnumerateInstanceLayerProperties( &count, nullptr );
    std::vector<VkLayerProperties> availables( count );
    vkEnumerateInstanceLayerProperties( &count, availables.data() );

    for( const char* reqestName : reqestNames )
    {
        bool found = false;

        for( const auto& available : availables )
        {
            if( strcmp( reqestName, available.layerName ) == 0 )
            {
                found = true;
                break;
            }
        }

        if( !found )
        {
            return false;
        }
    }

    return true;
}

bool VulkanRendererBackend::checkDeviceExtensionSupport( VkPhysicalDevice device, std::vector<const char*>& reqestNames )
{
    uint32_t count;
    vkEnumerateDeviceExtensionProperties( device, nullptr, &count, nullptr );
    std::vector<VkExtensionProperties> availables( count );
    vkEnumerateDeviceExtensionProperties( device, nullptr, &count, availables.data() );

    for( const char* reqestName : reqestNames )
    {
        bool found = false;

        for( const auto& available : availables )
        {
            if( strcmp( reqestName, available.extensionName ) == 0 )
            {
                found = true;
                break;
            }
        }

        if( !found )
        {
            return false;
        }
    }

    return true;
}

void VulkanRendererBackend::createVkInstance()
{
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .apiVersion = VK_API_VERSION_1_3
    };

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
    std::vector<const char*> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );
    if( ON_DEBUG ) extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

    std::vector<const char*> validationLayers;
    if( ON_DEBUG ) validationLayers.push_back( "VK_LAYER_KHRONOS_validation" );
    if( !checkValidationLayerSupport( validationLayers ) )
    {
        throw std::runtime_error( "validation layers requested, but not available!" );
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
        .enabledLayerCount = ( uint32 )validationLayers.size(),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = ( uint32 )extensions.size(),
        .ppEnabledExtensionNames = extensions.data(),
    };

    if( vkCreateInstance( &createInfo, nullptr, &instance ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create instance!" );
    }

    if( ON_DEBUG )
    {
        auto func = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
        if( !func || func( instance, &debugCreateInfo, nullptr, &debugMessenger ) != VK_SUCCESS )
        {
            throw std::runtime_error( "failed to set up debug messenger!" );
        }
    }
}

void VulkanRendererBackend::createVkSurface( GLFWwindow* window )
{
    if( glfwCreateWindowSurface( instance, window, nullptr, &surface ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create window surface!" );
    }
}

void VulkanRendererBackend::createVkDevice()
{
    physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices( instance, &deviceCount, nullptr );
    std::vector<VkPhysicalDevice> devices( deviceCount );
    vkEnumeratePhysicalDevices( instance, &deviceCount, devices.data() );

    std::vector<const char*> extentions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,

        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, // not used
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, // not used
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,

        VK_KHR_SPIRV_1_4_EXTENSION_NAME, // not used
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    };

    for( const auto& device : devices )
    {
        if( checkDeviceExtensionSupport( device, extentions ) )
        {
            physicalDevice = device;
            break;
        }
    }

    if( physicalDevice == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "failed to find a suitable GPU!" );
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, nullptr );
    std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, queueFamilies.data() );

    queueFamilyIndex = 0;
    {
        for( ; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex )
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, queueFamilyIndex, surface, &presentSupport );

            if( queueFamilies[ queueFamilyIndex ].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport )
                break;
        }

        if( queueFamilyIndex >= queueFamilyCount )
            throw std::runtime_error( "failed to find a graphics & present queue!" );
    }
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = ( uint32 )extentions.size(),
        .ppEnabledExtensionNames = extentions.data(),
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures f1{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .bufferDeviceAddress = VK_TRUE,
    };

    VkPhysicalDeviceAccelerationStructureFeaturesKHR f2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .accelerationStructure = VK_TRUE,
    };

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR f3{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .rayTracingPipeline = VK_TRUE,
    };

    createInfo.pNext = &f1;
    f1.pNext = &f2;
    f2.pNext = &f3;

    if( vkCreateDevice( physicalDevice, &createInfo, nullptr, &device ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create logical device!" );
    }

    vkGetDeviceQueue( device, queueFamilyIndex, 0, &graphicsQueue );
}

void VulkanRendererBackend::createSwapChain()
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physicalDevice, surface, &capabilities );

    const VkColorSpaceKHR defaultSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    {
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, surface, &formatCount, nullptr );
        std::vector<VkSurfaceFormatKHR> formats( formatCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, surface, &formatCount, formats.data() );

        bool found = false;
        for( auto format : formats )
        {
            if( format.format == swapChainImageFormat && format.colorSpace == defaultSpace )
            {
                found = true;
                break;
            }
        }

        if( !found )
        {
            throw std::runtime_error( "" );
        }
    }

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    {
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, surface, &presentModeCount, nullptr );
        std::vector<VkPresentModeKHR> presentModes( presentModeCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, surface, &presentModeCount, presentModes.data() );

        for( auto mode : presentModes )
        {
            if( mode == VK_PRESENT_MODE_MAILBOX_KHR )
            {
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }

    uint32 imageCount = capabilities.minImageCount + 1;
    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = swapChainImageFormat,
        .imageColorSpace = defaultSpace,
        .imageExtent = {.width = RenderSettings::screenWidth , .height = RenderSettings::screenHeight },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT, // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
    };

    if( vkCreateSwapchainKHR( device, &createInfo, nullptr, &swapChain ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create swap chain!" );
    }

    vkGetSwapchainImagesKHR( device, swapChain, &imageCount, nullptr );
    swapChainImages.resize( imageCount );
    vkGetSwapchainImagesKHR( device, swapChain, &imageCount, swapChainImages.data() );

    for( const auto& image : swapChainImages )
    {
        VkImageViewCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapChainImageFormat,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        // VkImageView imageView;
        // if (vkCreateImageView(device, &createInfo, nullptr, &imageView) != VK_SUCCESS) {
        //     throw std::runtime_error("failed to create image views!");
        // }
        // swapChainImageViews.push_back(imageView);
    }
}

void VulkanRendererBackend::createCommandCenter()
{
    VkCommandPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndex,
    };

    if( vkCreateCommandPool( device, &poolInfo, nullptr, &commandPool ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create command pool!" );
    }

    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .commandBufferCount = 1,
    };

    if( vkAllocateCommandBuffers( device, &allocInfo, &commandBuffer ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to allocate command buffers!" );
    }
}

void VulkanRendererBackend::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    if( vkCreateSemaphore( device, &semaphoreInfo, nullptr, &imageAvailableSemaphore ) != VK_SUCCESS ||
        vkCreateSemaphore( device, &semaphoreInfo, nullptr, &renderFinishedSemaphore ) != VK_SUCCESS ||
        vkCreateFence( device, &fenceInfo, nullptr, &fence0 ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create synchronization objects for a frame!" );
    }

}

uint32 VulkanRendererBackend::findMemoryType( uint32_t memoryTypeBits, VkMemoryPropertyFlags reqMemProps )
{
    uint32 memTypeIndex = 0;
    std::bitset<32> isSuppoted( memoryTypeBits );

    VkPhysicalDeviceMemoryProperties spec;
    vkGetPhysicalDeviceMemoryProperties( physicalDevice, &spec );

    for( auto& [props, _] : std::span<VkMemoryType>( spec.memoryTypes, spec.memoryTypeCount ) )
    {
        if( isSuppoted[ memTypeIndex ] && ( props & reqMemProps ) == reqMemProps )
        {
            break;
        }
        ++memTypeIndex;
    }
    return memTypeIndex;
}

std::tuple<VkBuffer, VkDeviceMemory> VulkanRendererBackend::createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags reqMemProps )
{
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;

    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
    };
    if( vkCreateBuffer( device, &bufferInfo, nullptr, &buffer ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create vertex buffer!" );
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements( device, buffer, &memRequirements );

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, reqMemProps ),
    };

    if( usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT )
    {
        VkMemoryAllocateFlagsInfo flagsInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
            .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
        };
        allocInfo.pNext = &flagsInfo;
    }

    if( vkAllocateMemory( device, &allocInfo, nullptr, &bufferMemory ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to allocate vertex buffer memory!" );
    }

    vkBindBufferMemory( device, buffer, bufferMemory, 0 );

    return { buffer, bufferMemory };
}

std::tuple<VkImage, VkDeviceMemory> VulkanRendererBackend::createImage(
    VkExtent2D extent,
    VkFormat format,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags reqMemProps )
{
    VkImage image;
    VkDeviceMemory imageMemory;

    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { extent.width, extent.height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if( vkCreateImage( device, &imageInfo, nullptr, &image ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create image!" );
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements( device, image, &memRequirements );

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, reqMemProps ),
    };

    if( vkAllocateMemory( device, &allocInfo, nullptr, &imageMemory ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to allocate image memory!" );
    }

    vkBindImageMemory( device, image, imageMemory, 0 );

    return { image, imageMemory };
}

void VulkanRendererBackend::setImageLayout(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask )
{
    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldImageLayout,
        .newLayout = newImageLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresourceRange,
    };

    if( oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if( oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL )
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }

    if( newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
    {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if( newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL )
    {
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }

    vkCmdPipelineBarrier(
        cmdbuffer, srcStageMask, dstStageMask, 0,
        0, nullptr, 0, nullptr, 1, &barrier );
}

inline VkDeviceAddress VulkanRendererBackend::getDeviceAddressOf( VkBuffer buffer )
{
    VkBufferDeviceAddressInfoKHR info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer,
    };
    return vkGetBufferDeviceAddressKHR( device, &info );
}

inline VkDeviceAddress VulkanRendererBackend::getDeviceAddressOf( VkAccelerationStructureKHR as )
{
    VkAccelerationStructureDeviceAddressInfoKHR info{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = as,
    };
    return vkGetAccelerationStructureDeviceAddressKHR( device, &info );
}

void VulkanRendererBackend::createBLAS()
{
    float vertices[][ 3 ] = {
        { -1.0f, -1.0f, 0.0f },
        {  1.0f, -1.0f, 0.0f },
        {  1.0f,  1.0f, 0.0f },
        { -1.0f,  1.0f, 0.0f },
    };
    uint32_t indices[] = { 0, 1, 3, 1, 2, 3 };

    VkTransformMatrixKHR geoTransforms[] = {
        {
            1.0f, 0.0f, 0.0f, -2.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        },
        {
            1.0f, 0.0f, 0.0f, 2.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        },
    };

    auto [vertexBuffer, vertexBufferMem] = createBuffer(
        sizeof( vertices ),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    auto [indexBuffer, indexBufferMem] = createBuffer(
        sizeof( indices ),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    auto [geoTransformBuffer, geoTransformBufferMem] = createBuffer(
        sizeof( geoTransforms ),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    void* dst;

    vkMapMemory( device, vertexBufferMem, 0, sizeof( vertices ), 0, &dst );
    memcpy( dst, vertices, sizeof( vertices ) );
    vkUnmapMemory( device, vertexBufferMem );

    vkMapMemory( device, indexBufferMem, 0, sizeof( indices ), 0, &dst );
    memcpy( dst, indices, sizeof( indices ) );
    vkUnmapMemory( device, indexBufferMem );

    vkMapMemory( device, geoTransformBufferMem, 0, sizeof( geoTransforms ), 0, &dst );
    memcpy( dst, geoTransforms, sizeof( geoTransforms ) );
    vkUnmapMemory( device, geoTransformBufferMem );

    VkAccelerationStructureGeometryKHR geometry0{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry = {
            .triangles = {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
                .vertexData = {.deviceAddress = getDeviceAddressOf( vertexBuffer ) },
                .vertexStride = sizeof( vertices[ 0 ] ),
                .maxVertex = sizeof( vertices ) / sizeof( vertices[ 0 ] ) - 1,
                .indexType = VK_INDEX_TYPE_UINT32,
                .indexData = {.deviceAddress = getDeviceAddressOf( indexBuffer ) },
                .transformData = {.deviceAddress = getDeviceAddressOf( geoTransformBuffer ) },
            },
        },
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };
    VkAccelerationStructureGeometryKHR geometries[] = { geometry0, geometry0 };

    uint32_t triangleCount0 = sizeof( indices ) / ( sizeof( indices[ 0 ] ) * 3 );
    uint32_t triangleCounts[] = { triangleCount0, triangleCount0 };

    VkAccelerationStructureBuildGeometryInfoKHR buildBlasInfo{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount = sizeof( geometries ) / sizeof( geometries[ 0 ] ),
        .pGeometries = geometries,
    };

    VkAccelerationStructureBuildSizesInfoKHR requiredSize{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizesKHR(
        device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildBlasInfo,
        triangleCounts,
        &requiredSize );

    std::tie( blasBuffer, blasBufferMem ) = createBuffer(
        requiredSize.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    auto [scratchBuffer, scratchBufferMem] = createBuffer(
        requiredSize.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    // Generate BLAS handle
    {
        VkAccelerationStructureCreateInfoKHR asCreateInfo{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .buffer = blasBuffer,
            .size = requiredSize.accelerationStructureSize,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        };
        vkCreateAccelerationStructureKHR( device, &asCreateInfo, nullptr, &blas );

        blasAddress = getDeviceAddressOf( blas );
    }

    // Build BLAS using GPU operations
    {
        vkResetCommandBuffer( commandBuffer, 0 );
        vkBeginCommandBuffer( commandBuffer, &beginInfo );
        {
            buildBlasInfo.dstAccelerationStructure = blas;
            buildBlasInfo.scratchData.deviceAddress = getDeviceAddressOf( scratchBuffer );
            VkAccelerationStructureBuildRangeInfoKHR buildBlasRangeInfo[] = {
                {
                    .primitiveCount = triangleCounts[ 0 ],
                    .transformOffset = 0,
                },
                {
                    .primitiveCount = triangleCounts[ 1 ],
                    .transformOffset = sizeof( geoTransforms[ 0 ] ),
                }
            };

            VkAccelerationStructureBuildGeometryInfoKHR buildBlasInfos[] = { buildBlasInfo };
            VkAccelerationStructureBuildRangeInfoKHR* buildBlasRangeInfos[] = { buildBlasRangeInfo };
            vkCmdBuildAccelerationStructuresKHR( commandBuffer, 1, buildBlasInfos, buildBlasRangeInfos );
            // vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildBlasInfo, 
            // (const VkAccelerationStructureBuildRangeInfoKHR *const *)&buildBlasRangeInfo);
        }
        vkEndCommandBuffer( commandBuffer );

        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
        };
        vkQueueSubmit( graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
        vkQueueWaitIdle( graphicsQueue );
    }

    vkFreeMemory( device, scratchBufferMem, nullptr );
    vkFreeMemory( device, vertexBufferMem, nullptr );
    vkFreeMemory( device, indexBufferMem, nullptr );
    vkFreeMemory( device, geoTransformBufferMem, nullptr );
    vkDestroyBuffer( device, scratchBuffer, nullptr );
    vkDestroyBuffer( device, vertexBuffer, nullptr );
    vkDestroyBuffer( device, indexBuffer, nullptr );
    vkDestroyBuffer( device, geoTransformBuffer, nullptr );
}

void VulkanRendererBackend::createTLAS()
{
    VkTransformMatrixKHR insTransforms[] = {
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 2.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        },
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, -2.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        },
    };

    VkAccelerationStructureInstanceKHR instance0{
        .instanceCustomIndex = 100,
        .mask = 0xFF,
        .instanceShaderBindingTableRecordOffset = 0,
        .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        .accelerationStructureReference = blasAddress,
    };
    VkAccelerationStructureInstanceKHR instanceData[] = { instance0, instance0 };
    instanceData[ 0 ].transform = insTransforms[ 0 ];
    instanceData[ 1 ].transform = insTransforms[ 1 ];
    instanceData[ 1 ].instanceShaderBindingTableRecordOffset = 2; // 2 geometry (in instance0) + 2 geometry (in instance1)

    auto [instanceBuffer, instanceBufferMem] = createBuffer(
        sizeof( instanceData ),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    void* dst;
    vkMapMemory( device, instanceBufferMem, 0, sizeof( instanceData ), 0, &dst );
    memcpy( dst, instanceData, sizeof( instanceData ) );
    vkUnmapMemory( device, instanceBufferMem );

    VkAccelerationStructureGeometryKHR instances{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry = {
            .instances = {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                .data = {.deviceAddress = getDeviceAddressOf( instanceBuffer ) },
            },
        },
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
    };

    uint32_t instanceCount = 2;

    VkAccelerationStructureBuildGeometryInfoKHR buildTlasInfo{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount = 1,     // It must be 1 with .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR as shown in the vulkan spec.
        .pGeometries = &instances,
    };

    VkAccelerationStructureBuildSizesInfoKHR requiredSize{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizesKHR(
        device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildTlasInfo,
        &instanceCount,
        &requiredSize );

    std::tie( tlasBuffer, tlasBufferMem ) = createBuffer(
        requiredSize.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    auto [scratchBuffer, scratchBufferMem] = createBuffer(
        requiredSize.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    // Generate TLAS handle
    {
        VkAccelerationStructureCreateInfoKHR asCreateInfo{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .buffer = tlasBuffer,
            .size = requiredSize.accelerationStructureSize,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        };
        vkCreateAccelerationStructureKHR( device, &asCreateInfo, nullptr, &tlas );
    }

    // Build TLAS using GPU operations
    {
        vkResetCommandBuffer( commandBuffer, 0 );
        vkBeginCommandBuffer( commandBuffer, &beginInfo );
        {
            buildTlasInfo.dstAccelerationStructure = tlas;
            buildTlasInfo.scratchData.deviceAddress = getDeviceAddressOf( scratchBuffer );

            VkAccelerationStructureBuildRangeInfoKHR buildTlasRangeInfo = { .primitiveCount = instanceCount };
            VkAccelerationStructureBuildRangeInfoKHR* buildTlasRangeInfo_[] = { &buildTlasRangeInfo };
            vkCmdBuildAccelerationStructuresKHR( commandBuffer, 1, &buildTlasInfo, buildTlasRangeInfo_ );
        }
        vkEndCommandBuffer( commandBuffer );

        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
        };
        vkQueueSubmit( graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
        vkQueueWaitIdle( graphicsQueue );
    }

    vkFreeMemory( device, scratchBufferMem, nullptr );
    vkFreeMemory( device, instanceBufferMem, nullptr );
    vkDestroyBuffer( device, scratchBuffer, nullptr );
    vkDestroyBuffer( device, instanceBuffer, nullptr );
}

void VulkanRendererBackend::createOutImage()
{
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM; //VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB(==swapChainImageFormat)
    std::tie( outImage, outImageMem ) = createImage(
        { RenderSettings::screenWidth, RenderSettings::screenHeight },
        format,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    VkImageSubresourceRange subresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1,
    };

    VkImageViewCreateInfo ci0{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = outImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components = {},
            /*.r = VK_COMPONENT_SWIZZLE_B,
            .b = VK_COMPONENT_SWIZZLE_R,
        },*/
        .subresourceRange = subresourceRange,
    };
    vkCreateImageView( device, &ci0, nullptr, &outImageView );

    vkResetCommandBuffer( commandBuffer, 0 );
    vkBeginCommandBuffer( commandBuffer, &beginInfo );
    {
        setImageLayout(
            commandBuffer,
            outImage,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL,
            subresourceRange );
    }
    vkEndCommandBuffer( commandBuffer );

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };
    vkQueueSubmit( graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    vkQueueWaitIdle( graphicsQueue );
}

void VulkanRendererBackend::createUniformBuffer()
{
    struct Data
    {
        float cameraPos[ 3 ];
        float yFov_degree;
    } dataSrc;

    std::tie( uniformBuffer, uniformBufferMem ) = createBuffer(
        sizeof( dataSrc ),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    void* dst;
    vkMapMemory( device, uniformBufferMem, 0, sizeof( dataSrc ), 0, &dst );
    *( Data* )dst = { 0, 0, 10, 60 };
    vkUnmapMemory( device, uniformBufferMem );
}

const char* raygen_src = R"(
#version 460
#extension GL_EXT_ray_tracing : enable

layout(binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, rgba8) uniform image2D image;
layout(binding = 2) uniform CameraProperties 
{
    vec3 cameraPos;
    float yFov_degree;
} g;

layout(location = 0) rayPayloadEXT vec3 hitValue;

void main() 
{
    const vec3 cameraX = vec3(1, 0, 0);
    const vec3 cameraY = vec3(0, -1, 0);
    const vec3 cameraZ = vec3(0, 0, -1);
    const float aspect_y = tan(radians(g.yFov_degree) * 0.5);
    const float aspect_x = aspect_y * float(gl_LaunchSizeEXT.x) / float(gl_LaunchSizeEXT.y);

    const vec2 screenCoord = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 ndc = screenCoord/vec2(gl_LaunchSizeEXT.xy) * 2.0 - 1.0;
    vec3 rayDir = ndc.x*aspect_x*cameraX + ndc.y*aspect_y*cameraY + cameraZ;

    hitValue = vec3(0.0);

    traceRayEXT(
        topLevelAS,                         // topLevel
        gl_RayFlagsOpaqueEXT, 0xff,         // rayFlags, cullMask
        0, 1, 0,                            // sbtRecordOffset, sbtRecordStride, missIndex
        g.cameraPos, 0.0, rayDir, 100.0,    // origin, tmin, direction, tmax
        0);                                 // payload

    imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(hitValue, 0.0));
})";

const char* miss_src = R"(
#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;

void main()
{
    hitValue = vec3(0.0, 0.0, 0.2);
})";

const char* chit_src = R"(
#version 460
#extension GL_EXT_ray_tracing : enable

layout(shaderRecordEXT) buffer CustomData
{
    vec3 color;
};

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

void main()
{
    if (gl_PrimitiveID == 1 && 
        gl_InstanceID == 1 && 
        gl_InstanceCustomIndexEXT == 100 && 
        gl_GeometryIndexEXT == 1) {
        hitValue = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    }
    else {
        hitValue = color;
    }
})";

void VulkanRendererBackend::createRayTracingPipeline()
{
    VkDescriptorSetLayoutBinding bindings[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        },
    };

    VkDescriptorSetLayoutCreateInfo ci0{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = sizeof( bindings ) / sizeof( bindings[ 0 ] ),
        .pBindings = bindings,
    };
    vkCreateDescriptorSetLayout( device, &ci0, nullptr, &descriptorSetLayout );

    VkPipelineLayoutCreateInfo ci1{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
    };
    vkCreatePipelineLayout( device, &ci1, nullptr, &pipelineLayout );

    ShaderModule<VK_SHADER_STAGE_RAYGEN_BIT_KHR> raygenModule( device, raygen_src );
    ShaderModule<VK_SHADER_STAGE_MISS_BIT_KHR> missModule( device, miss_src );
    ShaderModule<VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR> chitModule( device, chit_src );
    VkPipelineShaderStageCreateInfo stages[] = { raygenModule, missModule, chitModule };

    VkRayTracingShaderGroupCreateInfoKHR shaderGroups[] = {
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = 0,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = 1,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = 2,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
    };

    VkRayTracingPipelineCreateInfoKHR ci2{
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .stageCount = sizeof( stages ) / sizeof( stages[ 0 ] ),
        .pStages = stages,
        .groupCount = sizeof( shaderGroups ) / sizeof( shaderGroups[ 0 ] ),
        .pGroups = shaderGroups,
        .maxPipelineRayRecursionDepth = 1,
        .layout = pipelineLayout,
    };
    vkCreateRayTracingPipelinesKHR( device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &ci2, nullptr, &pipeline );
}

void VulkanRendererBackend::createDescriptorSets()
{

//    //1: create descriptor pool for IMGUI
//// the size of the pool is very oversize, but it's copied from imgui demo itself.
//    VkDescriptorPoolSize pool_sizes[] =
//    {
//        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
//        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
//        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
//        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
//        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
//        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
//        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
//        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
//        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
//        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
//        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
//    };
//
//    VkDescriptorPoolCreateInfo pool_info = {};
//    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
//    pool_info.maxSets = 1000;
//    pool_info.poolSizeCount = std::size( pool_sizes );
//    pool_info.pPoolSizes = pool_sizes;
//
//    VkDescriptorPool imguiPool;
//    vkCreateDescriptorPool( device, &pool_info, nullptr, &imguiPool );


    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 },
    };
    //    // Create Descriptor Pool
    //// If you wish to load e.g. additional textures you may need to alter pools sizes and maxSets.
    //{
    //    VkDescriptorPoolSize pool_sizes[] =
    //    {
    //        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
    //    };
    //    VkDescriptorPoolCreateInfo pool_info = {};
    //    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    //    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    //    pool_info.maxSets = 0;
    //    for( VkDescriptorPoolSize& pool_size : pool_sizes )
    //        pool_info.maxSets += pool_size.descriptorCount;
    //    pool_info.poolSizeCount = ( uint32_t )IM_ARRAYSIZE( pool_sizes );
    //    pool_info.pPoolSizes = pool_sizes;
    //    err = vkCreateDescriptorPool( g_Device, &pool_info, g_Allocator, &g_DescriptorPool );
    //    check_vk_result( err );
    //}

    VkDescriptorPoolCreateInfo ci0{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 11000,
        .poolSizeCount = sizeof( poolSizes ) / sizeof( poolSizes[ 0 ] ),
        .pPoolSizes = poolSizes,
    };
    vkCreateDescriptorPool( device, &ci0, nullptr, &descriptorPool );

    VkDescriptorSetAllocateInfo ai0{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout,
    };
    vkAllocateDescriptorSets( device, &ai0, &descriptorSet );

    VkWriteDescriptorSet write_temp{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .descriptorCount = 1,
    };

    // Descriptor(binding = 0), VkAccelerationStructure
    VkWriteDescriptorSetAccelerationStructureKHR desc0{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &tlas,
    };
    VkWriteDescriptorSet write0 = write_temp;
    write0.pNext = &desc0;
    write0.dstBinding = 0;
    write0.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    // Descriptor(binding = 1), VkImage for output
    VkDescriptorImageInfo desc1{
        .imageView = outImageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkWriteDescriptorSet write1 = write_temp;
    write1.dstBinding = 1;
    write1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write1.pImageInfo = &desc1;

    // Descriptor(binding = 2), VkBuffer for uniform
    VkDescriptorBufferInfo desc2{
        .buffer = uniformBuffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    VkWriteDescriptorSet write2 = write_temp;
    write2.dstBinding = 2;
    write2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write2.pBufferInfo = &desc2;

    VkWriteDescriptorSet writeInfos[] = { write0, write1, write2 };
    vkUpdateDescriptorSets( device, sizeof( writeInfos ) / sizeof( writeInfos[ 0 ] ), writeInfos, 0, VK_NULL_HANDLE );
    /*
    [VUID-VkWriteDescriptorSet-descriptorType-00336]
    If descriptorType is VK_DESCRIPTOR_TYPE_STORAGE_IMAGE or VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
    the imageView member of each element of pImageInfo must have been created with the identity swizzle.
    */
}

struct ShaderGroupHandle
{
    uint8_t data[ RenderSettings::shaderGroupHandleSize ];
};

struct HitgCustomData
{
    float color[ 3 ];
};

/*
In the vulkan spec,
[VUID-vkCmdTraceRaysKHR-stride-03686] pMissShaderBindingTable->stride must be a multiple of VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupHandleAlignment
[VUID-vkCmdTraceRaysKHR-stride-03690] pHitShaderBindingTable->stride must be a multiple of VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupHandleAlignment
[VUID-vkCmdTraceRaysKHR-pRayGenShaderBindingTable-03682] pRayGenShaderBindingTable->deviceAddress must be a multiple of VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupBaseAlignment
[VUID-vkCmdTraceRaysKHR-pMissShaderBindingTable-03685] pMissShaderBindingTable->deviceAddress must be a multiple of VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupBaseAlignment
[VUID-vkCmdTraceRaysKHR-pHitShaderBindingTable-03689] pHitShaderBindingTable->deviceAddress must be a multiple of VkPhysicalDeviceRayTracingPipelinePropertiesKHR::shaderGroupBaseAlignment

As shown in the vulkan spec 40.3.1. Indexing Rules,
    pHitShaderBindingTable->deviceAddress + pHitShaderBindingTable->stride ¡¿ (
    instanceShaderBindingTableRecordOffset + geometryIndex ¡¿ sbtRecordStride + sbtRecordOffset )
*/
void VulkanRendererBackend::createShaderBindingTable()
{
    auto alignTo = []( auto value, auto alignment ) -> decltype( value )
        {
            return ( value + ( decltype( value ) )alignment - 1 ) & ~( ( decltype( value ) )alignment - 1 );
        };
    const uint32_t handleSize = RenderSettings::shaderGroupHandleSize;
    const uint32_t groupCount = 3; // 1 raygen, 1 miss, 1 hit group
    std::vector<ShaderGroupHandle> handles( groupCount );
    vkGetRayTracingShaderGroupHandlesKHR( device, pipeline, 0, groupCount, handleSize * groupCount, handles.data() );
    ShaderGroupHandle rgenHandle = handles[ 0 ];
    ShaderGroupHandle missHandle = handles[ 1 ];
    ShaderGroupHandle hitgHandle = handles[ 2 ];

    const uint32_t rgenStride = alignTo( handleSize, rtProperties.shaderGroupHandleAlignment );
    rgenSbt = { 0, rgenStride, rgenStride };

    const uint64_t missOffset = alignTo( rgenSbt.size, rtProperties.shaderGroupBaseAlignment );
    const uint32_t missStride = alignTo( handleSize, rtProperties.shaderGroupHandleAlignment );
    missSbt = { 0, missStride, missStride };

    const uint32_t hitgCustomDataSize = sizeof( HitgCustomData );
    const uint32_t geometryCount = 4;
    const uint64_t hitgOffset = alignTo( missOffset + missSbt.size, rtProperties.shaderGroupBaseAlignment );
    const uint32_t hitgStride = alignTo( handleSize + hitgCustomDataSize, rtProperties.shaderGroupHandleAlignment );
    hitgSbt = { 0, hitgStride, hitgStride * geometryCount };

    const uint64_t sbtSize = hitgOffset + hitgSbt.size;
    std::tie( sbtBuffer, sbtBufferMem ) = createBuffer(
        sbtSize,
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    auto sbtAddress = getDeviceAddressOf( sbtBuffer );
    if( sbtAddress != alignTo( sbtAddress, rtProperties.shaderGroupBaseAlignment ) )
    {
        throw std::runtime_error( "It will not be happened!" );
    }
    rgenSbt.deviceAddress = sbtAddress;
    missSbt.deviceAddress = sbtAddress + missOffset;
    hitgSbt.deviceAddress = sbtAddress + hitgOffset;

    uint8_t* dst;
    vkMapMemory( device, sbtBufferMem, 0, sbtSize, 0, ( void** )&dst );
    {
        *( ShaderGroupHandle* )dst = rgenHandle;
        *( ShaderGroupHandle* )( dst + missOffset ) = missHandle;

        *( ShaderGroupHandle* )( dst + hitgOffset + 0 * hitgStride ) = hitgHandle;
        *( HitgCustomData* )( dst + hitgOffset + 0 * hitgStride + handleSize ) = { 0.6f, 0.1f, 0.2f }; // Deep Red Wine
        *( ShaderGroupHandle* )( dst + hitgOffset + 1 * hitgStride ) = hitgHandle;
        *( HitgCustomData* )( dst + hitgOffset + 1 * hitgStride + handleSize ) = { 0.1f, 0.8f, 0.4f }; // Emerald Green
        *( ShaderGroupHandle* )( dst + hitgOffset + 2 * hitgStride ) = hitgHandle;
        *( HitgCustomData* )( dst + hitgOffset + 2 * hitgStride + handleSize ) = { 0.9f, 0.7f, 0.1f }; // Golden Yellow
        *( ShaderGroupHandle* )( dst + hitgOffset + 3 * hitgStride ) = hitgHandle;
        *( HitgCustomData* )( dst + hitgOffset + 3 * hitgStride + handleSize ) = { 0.3f, 0.6f, 0.9f }; // Dawn Sky Blue
    }
    vkUnmapMemory( device, sbtBufferMem );
}
