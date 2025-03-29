#define GLFW_INCLUDE_VULKAN
#include "glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "core/app.hpp"
#include "core/buffer.hpp"
#include "core/image.hpp"
#include "core/memory.hpp"
#include "core/shaderModule.hpp"
#include "model.hpp"

#include <iostream>
#include <random>
#include <cmath>
#include <vector>
using std::cout, std::endl;
using std::sin, std::cos, std::asin, std::sqrt;
using std::vector;

constexpr float PI = 3.141'592;

constexpr unsigned int WINDOW_WIDTH  = 1280;
constexpr unsigned int WINDOW_HEIGHT = 720;
constexpr unsigned int SHADER_GROUP_HANDLE_SIZE = 32;

constexpr const char* PATH_SHADER_RAYGEN  = "./shaders/raygen.spv";
constexpr const char* PATH_SHADER_MISS_BG = "./shaders/bgMiss.spv";
constexpr const char* PATH_SHADER_MISS_AO = "./shaders/aoMiss.spv";
constexpr const char* PATH_SHADER_CHIT    = "./shaders/cHit.spv";

constexpr const char* PATH_MODEL_TEAPOT = "./resources/models/teapot.obj";
constexpr const char* PATH_MODEL_DRAGON = "./resources/models/dragon.obj";
constexpr const char* PATH_MODEL_SPONZA = "./resources/models/sponza.obj";
constexpr const char* PATH_MODEL_BUNNY  = "./resources/models/bunny.obj";

constexpr const char* PATH_TEXTURE_STATUE = "./resources/textures/statue.jpg";

namespace Random {
    static std::random_device rd;
    static std::mt19937 engine(rd());

    inline float range(float from, float to) noexcept {
        std::uniform_real_distribution<float> dis(from, to);

        return dis(engine);
    }
}

struct alignas(16) RandomSample {
    float v[3]{ };
};

unsigned int rayCount = 1000;
vector<RandomSample> samples;       // closest hit shader desc set 1, binding 3

// UBO Buffer Block Alignment layout = std140
struct alignas(16) UBO {
    Vec3 camPos = { 0.0f, 0.0f, 5.0f };
    float verticalFov = 60.0f;              // unit: degree
} ubo;

struct ShaderGroupHandle {
    unsigned char data[SHADER_GROUP_HANDLE_SIZE]{ };
};

struct ClosestHitCustomData {
    float color[4]{ };
};

Model model;

struct VKglobal {
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

    VkSwapchainKHR swapChain;
    vector<VkImage> swapChainImages;

    Buffer blasBuffer;
    VkAccelerationStructureKHR blas;
    VkDeviceAddress blasAddress;

    // ray gen shader desc set 0, binding 0
    Buffer tlasBuffer;
    VkAccelerationStructureKHR tlas;

    // ray gen shader desc set 0, binding 1
    Image outImage;
    VkImageView outImageView;

    // ray gen shader desc set 0, binding 2
    Buffer uniformBuffer;

    Buffer vertexBuffer;    // closest hit shader desc set 1, binding 0
    Buffer indexBuffer;     // closest hit shader desc set 1, binding 1

    // ray gen shader layout & closest hit shader layout
    vector<VkDescriptorSetLayout> descSetLayouts;
    VkDescriptorPool descPool;
    vector<VkDescriptorSet> descSets;

    VkPipelineLayout rtPipelineLayout;
    VkPipeline rtPipeline;

    Buffer sbtBuffer;
    VkStridedDeviceAddressRegionKHR rayGenSBT;
    VkStridedDeviceAddressRegionKHR missSBT;
    VkStridedDeviceAddressRegionKHR hitSBT;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore availableSemaphore;
    VkSemaphore renderDoneSemaphore;
    VkFence fence;

    const VkFormat swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    const VkColorSpaceKHR swapChainImageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    const VkExtent2D swapChainImageExtent = {
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT
    };

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProps = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };

    VkDescriptorPool guiDescPool;
    VkDescriptorSet guiDescSet;
    VkRenderPass guiRenderPass;

    vector<VkImageView> swapChainImageViews;
    vector<VkFramebuffer> framebuffers;

    Image texture;
    VkImageView textureImageView;
    VkSampler textureSampler;

    Buffer samples;

    ~VKglobal() noexcept {
        vkDestroyRenderPass(App::device(), guiRenderPass, nullptr);
        vkDestroyDescriptorPool(App::device(), guiDescPool, nullptr);

        vkDestroyFence(App::device(), fence, nullptr);
        vkDestroySemaphore(App::device(), availableSemaphore, nullptr);
        vkDestroySemaphore(App::device(), renderDoneSemaphore, nullptr);

        vkDestroyAccelerationStructureKHR(App::device(), tlas, nullptr);
        vkDestroyAccelerationStructureKHR(App::device(), blas, nullptr);

        vkDestroySampler(App::device(), textureSampler, nullptr);
        vkDestroyImageView(App::device(), textureImageView, nullptr);
        vkDestroyImageView(App::device(), outImageView, nullptr);

        vkDestroyDescriptorPool(App::device(), descPool, nullptr);
        for (auto& descSet: descSetLayouts)
            vkDestroyDescriptorSetLayout(App::device(), descSet, nullptr);

        vkDestroyPipeline(App::device(), rtPipeline, nullptr);
        vkDestroyPipelineLayout(App::device(), rtPipelineLayout, nullptr);

        vkDestroyCommandPool(App::device(), commandPool, nullptr);

        for (auto framebuffer: framebuffers)
            vkDestroyFramebuffer(App::device(), framebuffer, nullptr);

        for (auto imageView: swapChainImageViews)
            vkDestroyImageView(App::device(), imageView, nullptr);

        vkDestroySwapchainKHR(App::device(), swapChain, nullptr);
    }
} vkGlobal;

// hemisphere 표면상의 임의의 한 점을 반환
Vec3 sample_on_half_sphere() {
    float theta = Random::range(0.0f, PI * 2);              // 0 <= theta <= 2 * PI
    float phi = asin(sqrt(Random::range(0.0f, 1.0f)));      // sin(phi) = A

    return {
        sin(phi) * cos(theta),
        cos(phi),
        sin(phi) * sin(theta)
    };
}

void loadDeviceExtensionFunctions() {
    vkGlobal.vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)(vkGetDeviceProcAddr(App::device(), "vkGetBufferDeviceAddressKHR"));
    vkGlobal.vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)(vkGetDeviceProcAddr(App::device(), "vkGetAccelerationStructureDeviceAddressKHR"));
    vkGlobal.vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)(vkGetDeviceProcAddr(App::device(), "vkCreateAccelerationStructureKHR"));
    vkGlobal.vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)(vkGetDeviceProcAddr(App::device(), "vkDestroyAccelerationStructureKHR"));
    vkGlobal.vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)(vkGetDeviceProcAddr(App::device(), "vkGetAccelerationStructureBuildSizesKHR"));
    vkGlobal.vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)(vkGetDeviceProcAddr(App::device(), "vkCmdBuildAccelerationStructuresKHR"));
	vkGlobal.vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)(vkGetDeviceProcAddr(App::device(), "vkCreateRayTracingPipelinesKHR"));
	vkGlobal.vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)(vkGetDeviceProcAddr(App::device(), "vkGetRayTracingShaderGroupHandlesKHR"));
    vkGlobal.vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)(vkGetDeviceProcAddr(App::device(), "vkCmdTraceRaysKHR"));

    VkPhysicalDeviceProperties2 deviceProps2 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &vkGlobal.rtPipelineProps
    };
    vkGetPhysicalDeviceProperties2(App::device(), &deviceProps2);

    // SHADER_GROUP_HANDLE_SIZE는 Vulkan Spec에서 정확히 32여야 한다고 명시되어 있음
    /* if (vkGlobal.rtPipelineProps.shaderGroupHandleSize != SHADER_GROUP_HANDLE_SIZE)
        cout << "[ERROR]: Shader Group Handle Size is not 32" << endl; */
}
inline VkDeviceAddress getDeviceAddress(VkBuffer buf) {
    VkBufferDeviceAddressInfoKHR deviceAddressInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buf
    };

    return vkGlobal.vkGetBufferDeviceAddressKHR(App::device(), &deviceAddressInfo);
}
inline VkDeviceAddress getDeviceAddress(VkAccelerationStructureKHR as) {
    VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = as
    };

    return vkGlobal.vkGetAccelerationStructureDeviceAddressKHR(App::device(), &deviceAddressInfo);
}

VkImageView createImageView(
    VkImage            image    , VkFormat                format       ,
    VkComponentMapping component, VkImageSubresourceRange resourceRange
) {
    VkImageViewCreateInfo imageViewInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components = component,
            .subresourceRange = resourceRange,
        };

    VkImageView imageView;
    if (vkCreateImageView(App::device(), &imageViewInfo, nullptr, &imageView) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateImageView()" << endl;

    return imageView;
}

void setImageLayout(
    VkImage image, VkImageLayout curImgLayout, VkImageLayout newImgLayout,
    VkImageSubresourceRange resourceRange
) {
    /*
     * Image Memory Barrier의 용도
     *   1. 동기화 -> (src | dst access mask) and (src | dst stage mask)
     *   2. queue family의 resource data race 관리 -> src | dst queue family index
     *   3. image layout의 전환 -> old | new layout
     *
     * buffer에 비해 image의 layout은 매우 복잡하기 때문에 API에서 지원
    */
    VkImageMemoryBarrier imgMemBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = curImgLayout,
        .newLayout = newImgLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,     // Resource의 Queue Family 소유권 이전에 관련된 항목
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,     // Resource의 Queue Family 소유권 이전에 관련된 항목
        .image = image,
        .subresourceRange = resourceRange
    };

    if      (curImgLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    else if (curImgLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    if      (newImgLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    else if (newImgLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        vkGlobal.commandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        1, &imgMemBarrier
    );
}
void setGUIImageLayout(
    VkImage image, VkImageLayout curImgLayout, VkImageLayout newImgLayout,
    VkImageSubresourceRange resourceRange
) {
    VkImageMemoryBarrier imgMemBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = curImgLayout,
        .newLayout = newImgLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,     // Resource의 Queue Family 소유권 이전에 관련된 항목
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,     // Resource의 Queue Family 소유권 이전에 관련된 항목
        .image = image,
        .subresourceRange = resourceRange
    };

    if      (curImgLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) imgMemBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    else if (curImgLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)     imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    if      (newImgLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    else if (newImgLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)     imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(
        vkGlobal.commandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        1, &imgMemBarrier
    );
}

void createSwapChain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(App::device(), App::window(), &capabilities);

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    {
        unsigned int presentModeCount{ };
        vkGetPhysicalDeviceSurfacePresentModesKHR(App::device(), App::window(), &presentModeCount, nullptr);

        vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(App::device(), App::window(), &presentModeCount, presentModes.data());

        for (const auto& mode: presentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = App::window(),
        .minImageCount = capabilities.minImageCount + 1,
        .imageFormat = vkGlobal.swapChainImageFormat,
        .imageColorSpace = vkGlobal.swapChainImageColorSpace,
        .imageExtent = vkGlobal.swapChainImageExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE
    };

    if (vkCreateSwapchainKHR(App::device(), &swapChainCreateInfo, nullptr, &vkGlobal.swapChain) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateSwapchainKHR()" << endl;

    unsigned int swapChainImageCount{ };
    vkGetSwapchainImagesKHR(App::device(), vkGlobal.swapChain, &swapChainImageCount, nullptr);

    vkGlobal.swapChainImages.resize(swapChainImageCount);
    vkGetSwapchainImagesKHR(App::device(), vkGlobal.swapChain, &swapChainImageCount, vkGlobal.swapChainImages.data());

    // swap chain 이미지들에 대한 view 생성
    for (const auto& image: vkGlobal.swapChainImages) {
        VkImageViewCreateInfo imageViewInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vkGlobal.swapChainImageFormat,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        };

        VkImageView imageView;
        if (vkCreateImageView(App::device(), &imageViewInfo, nullptr, &imageView) != VK_SUCCESS)
            cout << "[ERROR]: vkCreateImageView()" << endl;

        vkGlobal.swapChainImageViews.push_back(imageView);
    }
}
void createCommandBuffers() {
    VkCommandPoolCreateInfo commandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = App::device().queueFamilyIndex()
    };

    if (vkCreateCommandPool(App::device(), &commandPoolCreateInfo, nullptr, &vkGlobal.commandPool) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateCommandPool()" << endl;

    VkCommandBufferAllocateInfo commandBufferAllocInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vkGlobal.commandPool,
        .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(App::device(), &commandBufferAllocInfo, &vkGlobal.commandBuffer) != VK_SUCCESS)
        cout << "[ERROR]: vkAllocateCommandBuffers()" << endl;
}
void createTexture() {
    int imgW{ },
        imgH{ },
        imgC{ };

    // ALPHA값이 없더라도 호환되도록 load (padding 수행) -> 일관성을 위함
    unsigned char* imgData = stbi_load(PATH_TEXTURE_STATUE, &imgW, &imgH, &imgC, STBI_rgb_alpha);

    // texture image 생성
    // VkBuffer에 image 데이터를 불러오고, VkImage로 복사 수행
    // buffer 생성에 사용된 img 포맷과 동일한 포맷 사용

    VkDeviceSize imgSize = imgW * imgH * STBI_rgb_alpha;
    VkFormat imgFormat = VK_FORMAT_R8G8B8A8_SRGB;
    {
        Buffer stagingBuffer(
            imgSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );
        stagingBuffer.bindMemory(
            new Memory(
                stagingBuffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            )
        );
        stagingBuffer.write(imgData);

        vkGlobal.texture.create(
            VK_IMAGE_TYPE_2D, imgFormat, { static_cast<unsigned int>(imgW), static_cast<unsigned int>(imgH), 1 },
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        );
        vkGlobal.texture.bindMemory(
            new Memory(
                vkGlobal.texture,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            )
        );

        vkGlobal.textureImageView = createImageView(
            vkGlobal.texture, imgFormat, { },
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,        // mipmap level
                .layerCount = 1         // entire data (no array data)
            }
        );

        const VkCommandBufferBeginInfo commandBufferBeginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

        vkResetCommandBuffer(vkGlobal.commandBuffer, 0);
        if (vkBeginCommandBuffer(vkGlobal.commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
            cout << "[ERROR]: vkBeginCommandBuffer() from createTexture()" << endl;
        {
            setImageLayout(
                vkGlobal.texture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,        // mipmap level
                    .layerCount = 1         // entire data (no array data)
                }
            );

            VkBufferImageCopy region {
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .layerCount = 1
                },
                .imageExtent = { static_cast<unsigned int>(imgW), static_cast<unsigned int>(imgH), 1 }
            };

            vkCmdCopyBufferToImage(
                vkGlobal.commandBuffer, stagingBuffer, vkGlobal.texture,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region
            );

            // shader에서 사용 가능하도록 image layout 변경
            setImageLayout(
                vkGlobal.texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,        // mipmap level
                    .layerCount = 1         // entire data (no array data)
                }
            );
        }
        if (vkEndCommandBuffer(vkGlobal.commandBuffer) != VK_SUCCESS)
            cout << "[ERROR]: vkEndCommandBuffer() from createTexture()" << endl;

        VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &vkGlobal.commandBuffer
        };

        if (vkQueueSubmit(App::device().queue(), 1, &submitInfo, nullptr) != VK_SUCCESS)
            cout << "[ERROR]: vkQueueSubmit() from createOutImage()" << endl;

        vkQueueWaitIdle(App::device().queue());
    }

    stbi_image_free(imgData);
}
void createTextureSampler() {
    VkPhysicalDeviceProperties gpuProperties{ };
    vkGetPhysicalDeviceProperties(App::device(), &gpuProperties);

    VkSamplerCreateInfo samplerInfo {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,      // 확대 필터 (oversampling 관련)
        .minFilter = VK_FILTER_LINEAR,      // 축소 필터 (undersampling 관련)
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,     // X
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,     // Y
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,     // Z
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = gpuProperties.limits.maxSamplerAnisotropy,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK
    };

    if (vkCreateSampler(App::device(), &samplerInfo, nullptr, &vkGlobal.textureSampler) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateSampler()" << endl;
}
void createRandomSamples() {
    samples.resize(rayCount);

    for (unsigned int i = 0; i < rayCount; ++i) {
        Vec3 v = sample_on_half_sphere();

        samples[i].v[0] = v.x;
        samples[i].v[1] = v.y;
        samples[i].v[2] = v.z;
    }
}
void createBuffers() {
    // Uniform Buffer
    {
        vkGlobal.uniformBuffer.create(
            sizeof(UBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        );
        vkGlobal.uniformBuffer.bindMemory(
            new Memory(
                vkGlobal.uniformBuffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            )
        );
        vkGlobal.uniformBuffer.write(&ubo);
    }

    // Vertex Buffer
    {
        vector<Model::Vertex> vertices = model.vertices();

        // staging buffer
        Buffer stagingBuffer(
            vertices.size() * sizeof(Model::Vertex),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );
        stagingBuffer.bindMemory(
            new Memory(
                stagingBuffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            )
        );
        stagingBuffer.write(vertices.data());

        // Vertex Buffer의 usage를 BLAS 빌드와 closest hit shader에서 사용할 storage buffer로 지정
        // AS의 input으로 사용하기 위해 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR 지정
        // GPU 내부의 AS 빌드(생성) 프로그램에서 이 버퍼의 device 주소를 참조하기 위해 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT 사용
        vkGlobal.vertexBuffer.create(
            vertices.size() * sizeof(Model::Vertex),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT          | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        );
        vkGlobal.vertexBuffer.bindMemory(
            new Memory(
                vkGlobal.vertexBuffer,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            )
        );

        VkCommandBufferBeginInfo beginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

        vkResetCommandBuffer(vkGlobal.commandBuffer, 0);
        if (vkBeginCommandBuffer(vkGlobal.commandBuffer, &beginInfo) != VK_SUCCESS)
            cout << "[ERROR]: vkBeginCommandBuffer() from createBuffers" << endl;
        {
            VkBufferCopy copyRegion { .size = vertices.size() * sizeof(Model::Vertex) };
            vkCmdCopyBuffer(vkGlobal.commandBuffer, stagingBuffer, vkGlobal.vertexBuffer, 1, &copyRegion);
        }
        if (vkEndCommandBuffer(vkGlobal.commandBuffer) != VK_SUCCESS)
            cout << "[ERROR]: vkEndCommandBuffer() from createBuffers" << endl;

        VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &vkGlobal.commandBuffer
        };

        if (vkQueueSubmit(App::device().queue(), 1, &submitInfo, nullptr) != VK_SUCCESS)
            cout << "[ERROR]: vkQueueSubmit() from createBuffers()" << endl;

        vkQueueWaitIdle(App::device().queue());
    }

    // Index Buffer
    {
        vector<unsigned int> indices = model.indices();

        // staging buffer
        Buffer stagingBuffer(
            indices.size() * sizeof(unsigned int),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );
        stagingBuffer.bindMemory(
            new Memory(
                stagingBuffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            )
        );
        stagingBuffer.write(indices.data());

        // Vertex Buffer와 마찬가지로 usage를 BLAS 빌드와 closest hit shader에서 사용할 storage buffer로 지정
        vkGlobal.indexBuffer.create(
            indices.size() * sizeof(unsigned int),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT          | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        );
        vkGlobal.indexBuffer.bindMemory(
            new Memory(
                vkGlobal.indexBuffer,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            )
        );

        VkCommandBufferBeginInfo beginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

        vkResetCommandBuffer(vkGlobal.commandBuffer, 0);
        if (vkBeginCommandBuffer(vkGlobal.commandBuffer, &beginInfo) != VK_SUCCESS)
            cout << "[ERROR]: vkBeginCommandBuffer() from createBuffers" << endl;
        {
            VkBufferCopy copyRegion { .size = indices.size() * sizeof(unsigned int) };
            vkCmdCopyBuffer(vkGlobal.commandBuffer, stagingBuffer, vkGlobal.indexBuffer, 1, &copyRegion);
        }
        if (vkEndCommandBuffer(vkGlobal.commandBuffer) != VK_SUCCESS)
            cout << "[ERROR]: vkEndCommandBuffer() from createBuffers" << endl;

        VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &vkGlobal.commandBuffer
        };

        if (vkQueueSubmit(App::device().queue(), 1, &submitInfo, nullptr) != VK_SUCCESS)
            cout << "[ERROR]: vkQueueSubmit() from createBuffers()" << endl;

        vkQueueWaitIdle(App::device().queue());
    }

    {   // random samples buffer
        // staging buffer
        Buffer stagingBuffer(
            rayCount * sizeof(RandomSample),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );
        stagingBuffer.bindMemory(
            new Memory(
                stagingBuffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            )
        );
        stagingBuffer.write(samples.data());

        vkGlobal.samples.create(
            rayCount * sizeof(RandomSample),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        );
        vkGlobal.samples.bindMemory(
            new Memory(
                vkGlobal.samples,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            )
        );

        VkCommandBufferBeginInfo beginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

        vkResetCommandBuffer(vkGlobal.commandBuffer, 0);
        if (vkBeginCommandBuffer(vkGlobal.commandBuffer, &beginInfo) != VK_SUCCESS)
            cout << "[ERROR]: vkBeginCommandBuffer() from createBuffers" << endl;
        {
            VkBufferCopy copyRegion { .size = rayCount * sizeof(RandomSample) };
            vkCmdCopyBuffer(vkGlobal.commandBuffer, stagingBuffer, vkGlobal.samples, 1, &copyRegion);
        }
        if (vkEndCommandBuffer(vkGlobal.commandBuffer) != VK_SUCCESS)
            cout << "[ERROR]: vkEndCommandBuffer() from createBuffers" << endl;

        VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &vkGlobal.commandBuffer
        };

        if (vkQueueSubmit(App::device().queue(), 1, &submitInfo, nullptr) != VK_SUCCESS)
            cout << "[ERROR]: vkQueueSubmit() from createBuffers()" << endl;

        vkQueueWaitIdle(App::device().queue());
    }
}
// Ray와 Scene의 교차 테스트 수행. BLAS, TLAS 생성 = 가속 작업 Scene 생성 (BVH 구조 생성)
void createBLAS() {
    // OpenGL 좌표계를 기준으로 함 (우측 +x, 하단 -y, 정면 -z)

    // object 두 개를 그리기 위해 각 object에 적용될 transform을 각각 정의
    VkTransformMatrixKHR transforms[] = {
        {   // left object
            1.0f, 0.0f, 0.0f, -1.0f,
            0.0f, 1.0f, 0.0f,  0.0f,
            0.0f, 0.0f, 1.0f,  0.0f
        },
        {   // right object
            1.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        }
    };

    Buffer geometryTransformBuffer(
        sizeof(transforms),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
    );
    geometryTransformBuffer.bindMemory(
        new Memory(
            geometryTransformBuffer,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        )
    );
    geometryTransformBuffer.write(transforms);

    // BLAS 생성 시작
    // geometry type BLAS: VK_GEOMETRY_TYPE_TRIANGLES_KHR
    // geometry type TLAS: VK_GEOMETRY_TYPE_INSTANCES_KHR
    VkAccelerationStructureGeometryKHR geometry0 {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry = {
            .triangles = {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
                .vertexData = { .deviceAddress = getDeviceAddress(vkGlobal.vertexBuffer) },
                .vertexStride = sizeof(Model::Vertex),
                .maxVertex = static_cast<unsigned int>(model.vertices().size() - 1),                // vertex의 최대 index 번호
                .indexType = VK_INDEX_TYPE_UINT32,
                .indexData = { .deviceAddress = getDeviceAddress(vkGlobal.indexBuffer) },
                .transformData = { .deviceAddress = getDeviceAddress(geometryTransformBuffer) }
            }
        },
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR     // 불투명한 물체이므로 OPAQUE
    };
    VkAccelerationStructureGeometryKHR geometries[] = { geometry0, geometry0 };

    unsigned int triangleCount = static_cast<unsigned int>(model.indices().size() / 3);
    unsigned int triangleCounts[] = { triangleCount, triangleCount };

    // BLAS 빌드시 필요한 용량을 얻기 위한 정보
    VkAccelerationStructureBuildGeometryInfoKHR blasBuildInfo {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount = (sizeof(geometries) / sizeof(geometries[0])),
        .pGeometries = geometries
    };
    VkAccelerationStructureBuildSizesInfoKHR blasBuildSizeInfo { .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };

    // BLAS 빌드에 필요한 용량 획득
    // AS Build Device를 GPU로 지정 (Vulkan에서는 CPU에서도 가능, DX12는 GPU만 가능)
    vkGlobal.vkGetAccelerationStructureBuildSizesKHR(
        App::device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &blasBuildInfo, triangleCounts, &blasBuildSizeInfo
    );

    // BLAS가 저장 된 실제 버퍼의 사이즈 = blasBuildSizeInfo.accelerationStructureSize
    // GPU에서 빌드되므로 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    vkGlobal.blasBuffer.create(
        blasBuildSizeInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    );
    vkGlobal.blasBuffer.bindMemory(
        new Memory(
            vkGlobal.blasBuffer,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        )
    );

    // 빌드 과정에 추가적으로 필요한 용량 확보를 위한 버퍼
    // GPU에서 빌드되므로 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    // 빌드가 끝나면 필요 없으므로 local 선언
    Buffer scratchBuffer(
        blasBuildSizeInfo.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    );
    scratchBuffer.bindMemory(
        new Memory(
            scratchBuffer,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        )
    );

    // BLAS에 대한 핸들과, 실제 BLAS가 저장되어 있는 곳 (버퍼) 이 존재
    // 이 단계에서 BLAS의 핸들 획득
    VkAccelerationStructureCreateInfoKHR asInfo {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = vkGlobal.blasBuffer,
        .size = blasBuildSizeInfo.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
    };
    // BLAS 핸들 생성 및 해당 핸들값 저장
    // BLAS 핸들 값은 TLAS에서 사용됨
    vkGlobal.vkCreateAccelerationStructureKHR(App::device(), &asInfo, nullptr, &vkGlobal.blas);
    vkGlobal.blasAddress = getDeviceAddress(vkGlobal.blas);

    // BLAS build
    // GPU상에서 이루어지므로, Command Buffer 사용
    VkCommandBufferBeginInfo commandBufferBeginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    vkResetCommandBuffer(vkGlobal.commandBuffer, 0);
    if (vkBeginCommandBuffer(vkGlobal.commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
        cout << "[ERROR]: vkBeginCommandBuffer() from createBLAS()" << endl;
    {
        blasBuildInfo.dstAccelerationStructure = vkGlobal.blas;
        blasBuildInfo.scratchData.deviceAddress = getDeviceAddress(scratchBuffer);

        // blasBuildInfo 상에 존재하는 geometry가 2개였으므로 그에 대한 명시
        // 이전에 사각형 한 개만을 정의했고, indices도 한 개만 존재
        // 따라서, primitiveOffset은 지정해주지 않아도 됨
        VkAccelerationStructureBuildRangeInfoKHR blasBuildRangeInfo[] = {
            {   // 첫 번째 geometry의 삼각형 수
                .primitiveCount = triangleCount,
                .transformOffset = 0
            },
            {   // 두 번째 geometry의 삼각형 수
                .primitiveCount = triangleCount,
                .transformOffset = sizeof(transforms[0])
            }
        };

        VkAccelerationStructureBuildGeometryInfoKHR blasBuildInfos[] = { blasBuildInfo };
        VkAccelerationStructureBuildRangeInfoKHR* blasBuildRangeInfos[] = { blasBuildRangeInfo };

        // 여러 개의 BLAS를 한 번의 호출로 동시에 생성할 수 있다는 점 인지
        vkGlobal.vkCmdBuildAccelerationStructuresKHR(vkGlobal.commandBuffer, 1, blasBuildInfos, blasBuildRangeInfos);
    }
    if (vkEndCommandBuffer(vkGlobal.commandBuffer) != VK_SUCCESS)
        cout << "[ERROR]: vkEndCommandBuffer() from createBLAS()" << endl;

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkGlobal.commandBuffer,
    };

    if (vkQueueSubmit(App::device().queue(), 1, &submitInfo, nullptr) != VK_SUCCESS)
        cout << "[ERROR]: vkQueueSubmit() from createBLAS()" << endl;

    vkQueueWaitIdle(App::device().queue());
}
void createTLAS() {
    // BLAS에서 그랬던 것 처럼, 두 개의 instance 생성
    // OpenGL 좌표계를 기준으로 함 (우측 +x, 하단 -y, 정면 -z)
    VkTransformMatrixKHR transforms[] = {
        {   // 첫 번째 instance에 적용 될 matrix
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.5f,
            0.0f, 0.0f, 1.0f, 0.0f
        },
        {   // 첫 번째 instance에 적용 될 matrix
            1.0f, 0.0f, 0.0f,  0.0f,
            0.0f, 1.0f, 0.0f, -1.5f,
            0.0f, 0.0f, 1.0f,  0.0f
        },
    };

    // Back-face Culling Disable
    // 앞면인지 뒷면인지 상관하지 않고 삼각형끼리 충돌 시, shader 호출
    // instance는 BLAS 를 참조
    VkAccelerationStructureInstanceKHR instance0 {
        .instanceCustomIndex = 100,     // shader에서 사용 가능한 custom instance index value (gl_InstanceCustomIndexEXT)
        .mask = 0xFF,
        .instanceShaderBindingTableRecordOffset = 0,
        .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        .accelerationStructureReference = vkGlobal.blasAddress
    };

    VkAccelerationStructureInstanceKHR instances[] = { instance0, instance0 };
    instances[0].transform = transforms[0];
    instances[1].transform = transforms[1];
    instances[1].instanceShaderBindingTableRecordOffset = 2;
    // 1이 아닌 2인 이유는 BLAS의 geometry가 instance당 2개이므로, 2번째 instance의 SBT record 시작 index가 2가 됨
    // = 첫 번째 instance의 SBT record set는 index 0, 1을 가지게 됨

    unsigned int instancesSize = sizeof(instances);
    unsigned int instanceCount = 2;

    // TLAS의 input이 되는 대상 버퍼들 생성
    Buffer instanceBuffer(
        static_cast<VkDeviceSize>(instancesSize),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
    );
    instanceBuffer.bindMemory(
        new Memory(
            instanceBuffer,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        )
    );
    instanceBuffer.write(instances);

    // TLAS 생성 시작
    // geometry type BLAS: VK_GEOMETRY_TYPE_TRIANGLES_KHR
    // geometry type TLAS: VK_GEOMETRY_TYPE_INSTANCES_KHR
    VkAccelerationStructureGeometryKHR instanceData {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry = {
            .instances = {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                .data = { .deviceAddress = getDeviceAddress(instanceBuffer) }
            }
        },
        // 불투명한 물체이므로 OPAQUE
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR
    };

    // TLAS 빌드시 필요한 용량을 얻기 위한 정보 세팅
    // BLAS는 하나 이상의 geometry로 구성되어 geometry Count가 1이 아닐 수 있음
    // TLAS는 여러 개의 instance로 이루어지고, 여러 개의 instance가 하나의 geometry로 구성
    // vulkan spec 에서도 geometryCount는 1 이어야 한다고 명시되어 있음
    VkAccelerationStructureBuildGeometryInfoKHR tlasBuildInfo {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount = 1,
        .pGeometries = &instanceData
    };
    VkAccelerationStructureBuildSizesInfoKHR tlasBuildSizeInfo { .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };

    // TLAS 빌드에 필요한 용량 획득
    // AS Build Device를 GPU로 지정 (Vulkan에서는 CPU에서도 가능, DX12는 GPU만 가능)
    vkGlobal.vkGetAccelerationStructureBuildSizesKHR(
        App::device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &tlasBuildInfo, &instanceCount, &tlasBuildSizeInfo
    );

    // TLAS가 저장 된 실제 버퍼의 사이즈 = blasBuildSizeInfo.accelerationStructureSize
    // GPU에서 빌드되므로 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    vkGlobal.tlasBuffer.create(
        tlasBuildSizeInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
    );
    vkGlobal.tlasBuffer.bindMemory(
        new Memory(
            vkGlobal.tlasBuffer,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        )
    );

    // 빌드 과정에 추가적으로 필요한 용량 확보를 위한 버퍼
    // GPU에서 빌드되므로 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    // 빌드가 끝나면 필요 없으므로 local 선언
    Buffer scratchBuffer(
        tlasBuildSizeInfo.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    );
    scratchBuffer.bindMemory(
        new Memory(
            scratchBuffer,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        )
    );

    // TLAS에 대한 핸들과, 실제 TLAS가 저장되어 있는 곳 (버퍼) 이 존재
    // 이 단계에서 TLAS의 핸들 획득
    VkAccelerationStructureCreateInfoKHR asInfo {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = vkGlobal.tlasBuffer,
        .size = tlasBuildSizeInfo.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
    };
    // TLAS 핸들 생성 및 해당 핸들값 저장
    vkGlobal.vkCreateAccelerationStructureKHR(App::device(), &asInfo, nullptr, &vkGlobal.tlas);

    // TLAS build
    // GPU상에서 이루어지므로, Command Buffer 사용
    VkCommandBufferBeginInfo commandBufferBeginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    vkResetCommandBuffer(vkGlobal.commandBuffer, 0);
    if (vkBeginCommandBuffer(vkGlobal.commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
        cout << "[ERROR] vkBeginCommandBuffer() from createTLAS()" << endl;
    {
        tlasBuildInfo.dstAccelerationStructure = vkGlobal.tlas;
        tlasBuildInfo.scratchData.deviceAddress = getDeviceAddress(scratchBuffer);

        VkAccelerationStructureBuildRangeInfoKHR tlasBuildRangeInfo = { .primitiveCount = instanceCount };
        VkAccelerationStructureBuildRangeInfoKHR* tlasBuildRangeInfos[] = { &tlasBuildRangeInfo };

        // 여러 개의 TLAS를 한 번의 호출로 동시에 생성할 수 있다는 점 인지
        vkGlobal.vkCmdBuildAccelerationStructuresKHR(vkGlobal.commandBuffer, 1, &tlasBuildInfo, tlasBuildRangeInfos);
    }
    if (vkEndCommandBuffer(vkGlobal.commandBuffer) != VK_SUCCESS)
        cout << "[ERROR] vkEndCommandBuffer() from createTLAS()" << endl;

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkGlobal.commandBuffer,
    };
    if (vkQueueSubmit(App::device().queue(), 1, &submitInfo, nullptr) != VK_SUCCESS)
        cout << "[ERROR] vkQueueSubmit() from createTLAS()" << endl;

    vkQueueWaitIdle(App::device().queue());
}
void createOutImage() {
    /*
     * swapchain image format은 VK_FORMAT_B8G8R8A8_SRGB 사용 (표준에 가까움)
     * outImage의 image format은 VK_FORMAT_R8G8B8A8_UNORM 사용
     * ray tracing을 수행할 땐 VK_IMAGE_USAGE_STORAGE_BIT flag가 필요한데 VK_FORMAT_B8G8R8A8_SRGB에서는 이를 지원하지 않음
     *
     * 따라서, image view의 component 설정에서 이 둘을 바꿔주는 방식 사용
     * SRGB와 UNORM의 색상 차이가 존재하는데 일단은 넘어가고 나중에 생각
    */
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    // VK_IMAGE_USAGE_SAMPLED_BIT for gui
    vkGlobal.outImage.create(
        VK_IMAGE_TYPE_2D, format, { WINDOW_WIDTH, WINDOW_HEIGHT, 1 },
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );
    vkGlobal.outImage.bindMemory(
        new Memory(
            vkGlobal.outImage,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        )
    );

    vkGlobal.outImageView = createImageView(
        vkGlobal.outImage, format, { },
        /* {
            .r = VK_COMPONENT_SWIZZLE_B,    // out image의 r 정보를 view image에서는 b 정보로 사용
            .b = VK_COMPONENT_SWIZZLE_R     // out image의 b 정보를 view image에서는 r 정보로 사용
        }, */
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,        // mipmap level
            .layerCount = 1         // entire data (no array data)
        }
    );

    const VkCommandBufferBeginInfo commandBufferBeginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    vkResetCommandBuffer(vkGlobal.commandBuffer, 0);
    if (vkBeginCommandBuffer(vkGlobal.commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
        cout << "[ERROR]: vkBeginCommandBuffer() from createOutImage()" << endl;
    {
        setImageLayout(
            vkGlobal.outImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,        // mipmap level
                .layerCount = 1         // entire data (no array data)
            }
        );
    }
    if (vkEndCommandBuffer(vkGlobal.commandBuffer) != VK_SUCCESS)
        cout << "[ERROR]: vkEndCommandBuffer() from createOutImage()" << endl;

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkGlobal.commandBuffer
    };

    if (vkQueueSubmit(App::device().queue(), 1, &submitInfo, nullptr) != VK_SUCCESS)
        cout << "[ERROR]: vkQueueSubmit() from createOutImage()" << endl;

    vkQueueWaitIdle(App::device().queue());
}
void createDescSetLayouts() {
    // 3가지 shader 중에서 ray gen shader의 uniform set 0에 대한 bindings descriptor
    // VK_SHADER_STAGE_ALL 보단 특정 shader에만 지정해주면 최적화에 유리
    // uniform은 각각 1개씩 존재하므로 descriptorCount = 1
    // shader가 다르더라도, 동일한 set에 binding이 중복되면 안됨

    vector<vector<VkDescriptorSetLayoutBinding>> descSetLayouts = {
        {   // raygen shader (set = 0)
            {   // TLAS
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
            },
            {   // out image
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR
            },
            {   // UBO
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR
            }
        },
        {   // closest hit shader (set = 1)
            {   // vertex SSBO
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
            },
            {   // index SSBO
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
            },
            {   // texture sampler
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
            },
            {   // random sample SSBO
                .binding = 3,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
            }
        }
    };
    vkGlobal.descSetLayouts.resize(descSetLayouts.size());

    for (size_t i = 0; i < vkGlobal.descSetLayouts.size(); ++i) {
        VkDescriptorSetLayoutCreateInfo descSetLayoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<unsigned int>(descSetLayouts[i].size()),
            .pBindings = descSetLayouts[i].data()
        };

        if (vkCreateDescriptorSetLayout(App::device(), &descSetLayoutInfo, nullptr, &vkGlobal.descSetLayouts[i]) != VK_SUCCESS)
            cout << "[ERROR]: vkCreateDescriptorSetLayout() " << "idx: " << i << endl;
    }
}
void createDescSets() {
    // descriptor set은 descriptor set layout에 해당하는 value들을 저장
    // desc set layout이 설계도 역할, desc set은 설계도대로 만든 것

    // desc set은 desc pool 내에 존재하므로 desc pool 부터 생성
    vector<VkDescriptorPoolSize> descPoolSizes = {
        // desc set 0
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },       // TLAS
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE             , 1 },       // out image
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER            , 1 },       // UBO

        // desc set 1
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER            , 3 },       // vertices, indices, random samples
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER    , 1 }        // texture sampler
    };

    unsigned int descSetCount = static_cast<unsigned int>(vkGlobal.descSetLayouts.size());
    vkGlobal.descSets.resize(descSetCount);

    VkDescriptorPoolCreateInfo descPoolInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = descSetCount,
        .poolSizeCount = static_cast<unsigned int>(descPoolSizes.size()),
        .pPoolSizes = descPoolSizes.data()
    };

    if (vkCreateDescriptorPool(App::device(), &descPoolInfo, nullptr, &vkGlobal.descPool))
        cout << "[ERROR]: vkCreateDescriptorPool()" << endl;

    VkDescriptorSetAllocateInfo descSetAllocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vkGlobal.descPool,
        .descriptorSetCount = descSetCount,
        .pSetLayouts = vkGlobal.descSetLayouts.data()
    };

    if (vkAllocateDescriptorSets(App::device(), &descSetAllocInfo, vkGlobal.descSets.data()) != VK_SUCCESS)
        cout << "[ERROR]: vkAllocateDescriptorSets()" << endl;

    // set 0, binding 0 (TLAS)
    VkWriteDescriptorSetAccelerationStructureKHR tlasInfo {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &vkGlobal.tlas
    };
    VkWriteDescriptorSet writeBinding0 {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = &tlasInfo,
        .dstSet = vkGlobal.descSets[0],
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
    };

    // set 0, binding 1 (out image)
    VkDescriptorImageInfo imageInfo {
        .imageView = vkGlobal.outImageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };
    VkWriteDescriptorSet writeBinding1 {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = vkGlobal.descSets[0],
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &imageInfo
    };

    // set 0, binding 2 (ubo)
    VkDescriptorBufferInfo bufferInfo {
        .buffer = vkGlobal.uniformBuffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };
    VkWriteDescriptorSet writeBinding2 {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = vkGlobal.descSets[0],
        .dstBinding = 2,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo
    };

    // set 1, binding 0 (vertex ssbo)
    VkDescriptorBufferInfo vertexBufferInfo {
        .buffer = vkGlobal.vertexBuffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };
    VkWriteDescriptorSet writeBinding3 {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = vkGlobal.descSets[1],
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &vertexBufferInfo
    };

    // set 1, binding 1 (index ssbo)
    VkDescriptorBufferInfo indexBufferInfo {
        .buffer = vkGlobal.indexBuffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };
    VkWriteDescriptorSet writeBinding4 {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = vkGlobal.descSets[1],
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &indexBufferInfo
    };

    // set 1, binding 2 (texture sampler)
    VkDescriptorImageInfo textureImageSamplerInfo {
        .sampler = vkGlobal.textureSampler,
        .imageView = vkGlobal.textureImageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    VkWriteDescriptorSet writeBinding5 {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = vkGlobal.descSets[1],
        .dstBinding = 2,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &textureImageSamplerInfo
    };

    // set 1, binding 3 (random samples SSBO)
    VkDescriptorBufferInfo samplesInfo {
        .buffer = vkGlobal.samples,
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };
    VkWriteDescriptorSet writeBinding6 {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = vkGlobal.descSets[1],
        .dstBinding = 3,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &samplesInfo
    };

    // binding 수행
    vector<VkWriteDescriptorSet> writeInfos = {
        writeBinding0, writeBinding1, writeBinding2,
        writeBinding3, writeBinding4, writeBinding5, writeBinding6
    };
    vkUpdateDescriptorSets(App::device(), static_cast<unsigned int>(writeInfos.size()), writeInfos.data(), 0, nullptr);

    /* Issue
     *   [VUID-VkWriteDescriptorSet-descriptorType-00336] WARNING
     *
     *   If descriptorType is VK_DESCRIPTOR_TYPE_STORAGE_IMAGE or VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
     *   the imageView member of each element of pImageInfo must have been created with the identity swizzle.
    */

    // 위의 Issue는 임시로 Miss Shader, cHit Shader에서 RGB -> BGR 해주는 것으로 해결 (ImageView Component 사용 X)
}
void createRayTracingPipelineLayout() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<unsigned int>(vkGlobal.descSetLayouts.size()),
        .pSetLayouts = vkGlobal.descSetLayouts.data()
    };

    if (vkCreatePipelineLayout(App::device(), &pipelineLayoutInfo, nullptr, &vkGlobal.rtPipelineLayout) != VK_SUCCESS)
        cout << "[ERROR]: vkCreatePipelineLayout()" << endl;
}
void createRayTracingPipeline() {
    // RT Pipeline에는 1개의 ray gen 셰이더와, 1개 이상의 any hit, miss, closest hit shader 셰이더로 구성될 수 있음
    // stage 개수 == shader module 개수

    ShaderModule raygenModule(VK_SHADER_STAGE_RAYGEN_BIT_KHR, PATH_SHADER_RAYGEN);
    ShaderModule bgMissModule(VK_SHADER_STAGE_MISS_BIT_KHR, PATH_SHADER_MISS_BG);
    ShaderModule aoMissModule(VK_SHADER_STAGE_MISS_BIT_KHR, PATH_SHADER_MISS_AO);
    ShaderModule cHitModule(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, PATH_SHADER_CHIT);

    vector<VkPipelineShaderStageCreateInfo> shaderStageInfos = { raygenModule.info(), bgMissModule.info(), aoMissModule.info(), cHitModule.info() };

    /*
     * stage 개수와 shader group 개수는 다를 수 있음
     * shader group은 세 가지 종류가 존재
     *   1. ray gen group
     *   2. miss group
     *   3. hit group
     *     - hit group은 최대 3가지 shader가 존재 가능 (cHit, anyHit, Intersection)
     *
     * type이 VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR 이면, generalShader에 shader stage index 지정
     *   -> 그 외의 shader는 VK_SHADER_UNUSED_KHR로 지정 (하지 않을 시 오류, vulkan spec 참조)
     *   == type이 VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR 이면 raygen Shader 혹은 miss Shader 라는 의미
     *
     * type이 VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR 이면, cHitShader 혹은 anyHitShader에 하나 이상 shader stage index 지정
     *   -> 그 외의 shader는 VK_SHADER_UNUSED_KHR로 지정 (하지 않을 시 오류, vulkan spec 참조)
     *   == type이 VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR 이면 cHit Shader 혹은 anyHit Shader 라는 의미
     */
    vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroupInfos = {
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = 0,                                                     // ray gen module
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR
        },
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = 1,                                                     // bg miss module
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR
        },
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader = 2,                                                     // ao miss module
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR
        },
        {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = 3,                                                  // closest hit module
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR
        }
    };

    VkRayTracingPipelineCreateInfoKHR rtPipelineInfo {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .stageCount = static_cast<unsigned int>(shaderStageInfos.size()),
        .pStages = shaderStageInfos.data(),
        .groupCount = static_cast<unsigned int>(shaderGroupInfos.size()),
        .pGroups = shaderGroupInfos.data(),
        .maxPipelineRayRecursionDepth = 1,
        .layout = vkGlobal.rtPipelineLayout
    };

    if (vkGlobal.vkCreateRayTracingPipelinesKHR(App::device(), nullptr, nullptr, 1, &rtPipelineInfo, nullptr, &vkGlobal.rtPipeline) != VK_SUCCESS)
        cout << "[ERROR]: vkCreateRayTracingPipelinesKHR()" << endl;
}
void createShaderBindingTable() {
    // SBT Buffer는 Desc를 통하지 않아도 shader에서 접근 가능한 특별한 Buffer

    const unsigned int handleSize = SHADER_GROUP_HANDLE_SIZE;   // 32
    const unsigned int groupCount = 4;                          // raygen 1, miss 2, closest hit 1
    const unsigned int geometryCount = 4;                       // object 4개

    // rayGen, miss, hit group의 핸들값 저장
    vector<ShaderGroupHandle> handles(groupCount);
    vkGlobal.vkGetRayTracingShaderGroupHandlesKHR(App::device(), vkGlobal.rtPipeline, 0, groupCount, handleSize * groupCount, handles.data());

    ShaderGroupHandle raygenHandle = handles[0];
    ShaderGroupHandle bgMissHandle = handles[1];
    ShaderGroupHandle aoMissHandle = handles[2];
    ShaderGroupHandle cHitHandle   = handles[3];

    // value를 alignment의 배수로 만들어서 반환 -> alignment는 2의 거듭제곱수
    auto alignFunc = [](auto value, auto align) -> decltype(value) { return (value + (decltype(value))align - 1) & ~((decltype(value))align - 1); };

    // raygen Shader는 size와 stride 크기가 같아야 함
    unsigned long long raygenStride = alignFunc(handleSize, vkGlobal.rtPipelineProps.shaderGroupHandleAlignment);
    vkGlobal.rayGenSBT = { 0, raygenStride, raygenStride };

    // miss + ao miss
    unsigned long long missStride = alignFunc(handleSize, vkGlobal.rtPipelineProps.shaderGroupHandleAlignment);
    vkGlobal.missSBT = { 0, missStride, missStride * 2 };

    // Geometry (사각형) 마다 한 개의 레코드를 부여할 것이므로, geometryCount만큼 곱해줌
    unsigned long long hitStride = alignFunc(handleSize + sizeof(ClosestHitCustomData), vkGlobal.rtPipelineProps.shaderGroupHandleAlignment);
    vkGlobal.hitSBT = { 0, hitStride, hitStride * geometryCount };

    // 하나의 SBT Buffer 내에 3개의 SBT가 있고, 그 내부에 ray gen, miss, hit group이 존재
    // 따라서, ray gen SBT의 크기 뒤에 miss SBT가 존재. 그러나 align을 지켜야 하므로 align func로 offset을 구해줌
    // 위와 동일한 이유로 hit SBT는 miss offset + miss SBT size
    unsigned long long missOffset = alignFunc(vkGlobal.rayGenSBT.size, vkGlobal.rtPipelineProps.shaderGroupBaseAlignment);
    unsigned long long hitOffset = alignFunc(missOffset + vkGlobal.missSBT.size, vkGlobal.rtPipelineProps.shaderGroupBaseAlignment);

    // 따라서, 전체 SBT 크기는 hit offset + hit SBT size
    // 여기서 align을 한번 더 해주지 않는 이유는 이미 이전에 align을 지켰으므로 겹칠 일도 없고,
    // shader binding table (buffer) 은 align되어 있지 않아도 정상 동작
    VkDeviceSize sbtSize = hitOffset + vkGlobal.hitSBT.size;

    vkGlobal.sbtBuffer.create(
        sbtSize,
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    );
    Memory* sbtBufMemory = new Memory(
        vkGlobal.sbtBuffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    vkGlobal.sbtBuffer.bindMemory(sbtBufMemory);

    // 밑의 sbtAddress가 shader group base alignment의 배수라고 가정하고 작성 -> 굳이 align 하지 않음
    VkDeviceAddress sbtAddress = getDeviceAddress(vkGlobal.sbtBuffer);

    vkGlobal.rayGenSBT.deviceAddress = sbtAddress;
    vkGlobal.missSBT.deviceAddress = sbtAddress + missOffset;
    vkGlobal.hitSBT.deviceAddress = sbtAddress + hitOffset;

    // 각 record의 가장 앞단에는 handle 필요
    // 하드코딩 한 이유는 memcpy보다 빠르고, 예제니까 명확히 알면 좋을 것
    unsigned char* dst{ };
    vkMapMemory(App::device(), *sbtBufMemory, 0, sbtSize, 0, (void**)&dst);
    {
        *(ShaderGroupHandle*)(dst) = raygenHandle;                                   // SBT 가장 앞부분에 ray gen group handle 32 byte
        *(ShaderGroupHandle*)(dst + missOffset + 0 * missStride) = bgMissHandle;     // ray gen group handle 뒤에 bg miss group handle 32 byte
        *(ShaderGroupHandle*)(dst + missOffset + 1 * missStride) = aoMissHandle;     // bg miss group handle 뒤에 ao miss group handle 32 byte

        // 이후 각 geometry마다 hit group handle 32 byte + custom data를 반복 -> 총 4개의 records
        *(ShaderGroupHandle*   )(dst + hitOffset + 0 * hitStride             ) = cHitHandle;
        *(ClosestHitCustomData*)(dst + hitOffset + 0 * hitStride + handleSize) = { 0.6f, 0.1f, 0.2f }; // Deep Red Wine
        *(ShaderGroupHandle*   )(dst + hitOffset + 1 * hitStride             ) = cHitHandle;
        *(ClosestHitCustomData*)(dst + hitOffset + 1 * hitStride + handleSize) = { 0.1f, 0.8f, 0.4f }; // Emerald Green
        *(ShaderGroupHandle*   )(dst + hitOffset + 2 * hitStride             ) = cHitHandle;
        *(ClosestHitCustomData*)(dst + hitOffset + 2 * hitStride + handleSize) = { 0.9f, 0.7f, 0.1f }; // Golden Yellow
        *(ShaderGroupHandle*   )(dst + hitOffset + 3 * hitStride             ) = cHitHandle;
        *(ClosestHitCustomData*)(dst + hitOffset + 3 * hitStride + handleSize) = { 0.3f, 0.6f, 0.9f }; // Dawn Sky Blue
    }
    vkUnmapMemory(App::device(), *sbtBufMemory);

    // 위 하드코딩을 더 간결하게 할 방법은 stride를 하나로 통일하는 방식 사용
    // 혹은, SBT 자체를 3개를 만드는 것 (현재는 하나의 SBT 버퍼 내에 모두 집어넣어서 사용하는 방식)
    //   -> 하나의 연속된 물리 메모리 내에 offset으로 분리해서 3개의 record가 존재
}
void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreCreateInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    if ((vkCreateSemaphore(App::device(), &semaphoreCreateInfo, nullptr, &vkGlobal.availableSemaphore) != VK_SUCCESS) or
        (vkCreateSemaphore(App::device(), &semaphoreCreateInfo, nullptr, &vkGlobal.renderDoneSemaphore) != VK_SUCCESS) or
        (vkCreateFence(App::device(), &fenceCreateInfo, nullptr, &vkGlobal.fence)))
        cout << "[ERROR]: createSyncObjects()" << endl;
}

void guiInit() {
    ImGui::CreateContext();
    {
        // ImGUI window docking settings
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // split docking region (viewport & details)
    }
    ImGui_ImplGlfw_InitForVulkan(App::window(), true);

    {   // descriptor pool for GUI
        vector<VkDescriptorPoolSize> guiDescPoolSizes = {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 }
        };
        // default image sampler 1
        // viewport image sampler 1

        VkDescriptorPoolCreateInfo guiDescPoolInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .poolSizeCount = static_cast<unsigned int>(guiDescPoolSizes.size()),
            .pPoolSizes = guiDescPoolSizes.data()
        };
        for (const auto& poolSize: guiDescPoolSizes)
            guiDescPoolInfo.maxSets += poolSize.descriptorCount;

        vkCreateDescriptorPool(App::device(), &guiDescPoolInfo, nullptr, &vkGlobal.guiDescPool);
    }
    {   // render pass for GUI
        VkAttachmentDescription attachmentDesc {
            .format = vkGlobal.swapChainImageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        };
        VkAttachmentReference attachmentRef {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };

        VkSubpassDescription subpass {
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentRef
        };
        VkSubpassDependency subpassDependency {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        };

        VkRenderPassCreateInfo guiRenderPassInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &attachmentDesc,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &subpassDependency
        };

        vkCreateRenderPass(App::device(), &guiRenderPassInfo, nullptr, &vkGlobal.guiRenderPass);

        // frame buffer for GUI
        for (const auto& imageView: vkGlobal.swapChainImageViews) {
            VkFramebufferCreateInfo framebufferInfo {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = vkGlobal.guiRenderPass,
                .attachmentCount = 1,
                .pAttachments = &imageView,
                .width = WINDOW_WIDTH,
                .height = WINDOW_HEIGHT,
                .layers = 1
            };

            VkFramebuffer frameBuffer;
            if (vkCreateFramebuffer(App::device(), &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS)
                cout << "[ERROR]: vkCreateFramebuffer()" << endl;

            vkGlobal.framebuffers.push_back(frameBuffer);
        }
    }

    VkSurfaceCapabilitiesKHR surfaceCAP;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(App::device(), App::window(), &surfaceCAP);

    ImGui_ImplVulkan_InitInfo guiInitInfo {
        .Instance = App::instance(),
        .PhysicalDevice = App::device(),
        .Device = App::device(),
        .QueueFamily = App::device().queueFamilyIndex(),
        .Queue = App::device().queue(),
        .DescriptorPool = vkGlobal.guiDescPool,
        .RenderPass = vkGlobal.guiRenderPass,
        .MinImageCount = surfaceCAP.minImageCount + 1,
        .ImageCount = surfaceCAP.minImageCount + 1,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT
    };
    ImGui_ImplVulkan_Init(&guiInitInfo);
    ImGui_ImplVulkan_CreateFontsTexture();

    // set out image to viewport window
    vkGlobal.guiDescSet = ImGui_ImplVulkan_AddTexture(vkGlobal.textureSampler, vkGlobal.outImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
void guiTerminate() {
    ImGui_ImplVulkan_RemoveTexture(vkGlobal.guiDescSet);

    ImGui_ImplVulkan_DestroyFontsTexture();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void createDockingSpace() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoTitleBar            | ImGuiWindowFlags_NoResize           |
        ImGuiWindowFlags_NoMove                | ImGuiWindowFlags_NoScrollbar        |
        ImGuiWindowFlags_NoScrollWithMouse     | ImGuiWindowFlags_NoCollapse         |
        ImGuiWindowFlags_NoBackground          | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoInputs           |
        ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("MainDockRegion", nullptr, windowFlags);

    static ImGuiID mainDockRegion = ImGui::GetID("MainDockRegion");
    ImGui::DockSpace(mainDockRegion, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void render() {
    const VkCommandBufferBeginInfo commandBufferBeginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    const VkStridedDeviceAddressRegionKHR callSBT{ };

    const VkImageSubresourceRange subResourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    const VkImageCopy copyRegion = {
        .srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .extent = { WINDOW_WIDTH, WINDOW_HEIGHT, 1 }
    };

    const VkClearValue clearColor = { .color = { 0.0f, 0.0f, 0.0f, 1.0f } };

    if (VkResult errCode = vkWaitForFences(App::device(), 1, &vkGlobal.fence, VK_TRUE, UINT64_MAX); (errCode != VK_SUCCESS))
        cout << "[ERROR " << errCode << "]: vkWaitForFences() from render()" << endl;

    vkResetFences(App::device(), 1, &vkGlobal.fence);

    unsigned int imgIndex{ };
    if (VkResult errCode = vkAcquireNextImageKHR(App::device(), vkGlobal.swapChain, UINT64_MAX, vkGlobal.availableSemaphore, nullptr, &imgIndex); (errCode != VK_SUCCESS))
        cout << "[ERROR " << errCode << "]: vkAcquireNextImageKHR() from render()" << endl;

    vkResetCommandBuffer(vkGlobal.commandBuffer, 0);
    if (vkBeginCommandBuffer(vkGlobal.commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
        cout << "[ERROR]: vkBeginCommandBuffer() from render()" << endl;
    {   // begin command buffer와 end command buffer 사이에 들어가는 내용들을 encoding 이라 할 것임
        vkCmdBindPipeline(vkGlobal.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, vkGlobal.rtPipeline);
        vkCmdBindDescriptorSets(
            vkGlobal.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
            vkGlobal.rtPipelineLayout, 0, static_cast<unsigned int>(vkGlobal.descSets.size()),
            vkGlobal.descSets.data(), 0, 0
        );

        vkGlobal.vkCmdTraceRaysKHR(
            vkGlobal.commandBuffer,
            &vkGlobal.rayGenSBT, &vkGlobal.missSBT, &vkGlobal.hitSBT,
            &callSBT, WINDOW_WIDTH, WINDOW_HEIGHT, 1
        );

        // Ray Shooting 이후, 복사를 위한 Layout으로 변경
        // copy image from
        setImageLayout(
            vkGlobal.outImage, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            subResourceRange
        );
        // copy image to
        setImageLayout(
            vkGlobal.swapChainImages[imgIndex], VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            subResourceRange
        );

        vkCmdCopyImage(
            vkGlobal.commandBuffer,
            vkGlobal.outImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vkGlobal.swapChainImages[imgIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copyRegion
        );

        // gui 렌더링을 위해 image layout 변경
        setImageLayout(
            vkGlobal.swapChainImages[imgIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            subResourceRange
        );

        // outImage layout change for gui render
        setGUIImageLayout(
            vkGlobal.outImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            subResourceRange
        );

        VkRenderPassBeginInfo renderPassBeginInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = vkGlobal.guiRenderPass,
            .framebuffer = vkGlobal.framebuffers[imgIndex],
            .renderArea = { .extent = vkGlobal.swapChainImageExtent },
            .clearValueCount = 1,
            .pClearValues = &clearColor
        };

        vkCmdBeginRenderPass(vkGlobal.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            createDockingSpace();
            // ImGui::ShowDemoWindow();

            {
                ImGui::Begin("Viewport");
                ImGui::Image((ImTextureID)vkGlobal.guiDescSet, ImGui::GetMainViewport()->Size);
                ImGui::End();
            }
            {
                ImGui::Begin("Inspector");
                ImGui::End();
            }
            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkGlobal.commandBuffer);
        }
        vkCmdEndRenderPass(vkGlobal.commandBuffer);

        // outImage layout restore
        setGUIImageLayout(
            vkGlobal.outImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            subResourceRange
        );

        // copy작업 후, present와 다음 frame 작업을 위한 원상복구
        setImageLayout(
            vkGlobal.outImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL,
            subResourceRange
        );

        // gui 렌더링 후엔 이미지 레이아웃이 present로 변경 됨 (renderpass attachment final layout 설정)
        /* setImageLayout(
            vkGlobal.swapChainImages[imgIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            subResourceRange
        ); */
    }
    if (vkEndCommandBuffer(vkGlobal.commandBuffer) != VK_SUCCESS)
        cout << "[ERROR]: vkEndCommandBuffer() from render()" << endl;

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vkGlobal.availableSemaphore,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkGlobal.commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &vkGlobal.renderDoneSemaphore
    };

    if (VkResult errCode = vkQueueSubmit(App::device().queue(), 1, &submitInfo, vkGlobal.fence); (errCode != VK_SUCCESS))
        cout << "[ERROR " << errCode << "]: vkQueueSubmit() from render()" << endl;

    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vkGlobal.renderDoneSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &vkGlobal.swapChain,
        .pImageIndices = &imgIndex
    };

    vkQueuePresentKHR(App::device().queue(), &presentInfo);
}

int main() {
    App::init(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Raytracing");
    model.load(PATH_MODEL_BUNNY);

    loadDeviceExtensionFunctions();
    createSwapChain();
    createCommandBuffers();
    createTexture();
    createTextureSampler();
    createRandomSamples();
    createBuffers();
    createBLAS();
    createTLAS();
    createOutImage();
    createDescSetLayouts();
    createDescSets();
    createRayTracingPipelineLayout();
    createRayTracingPipeline();
    createShaderBindingTable();
    createSyncObjects();

    guiInit();

    while (!glfwWindowShouldClose(App::window())) {
        glfwPollEvents();
        render();
    }

    vkDeviceWaitIdle(App::device());
    guiTerminate();
    App::terminate();
    return 0;
}