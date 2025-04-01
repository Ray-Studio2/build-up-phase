#include "core/image.hpp"
#include "core/app.hpp"

#include <stdexcept>
#include <cstring>          // memcpy
#include <span>
#include "vulkan/vulkan.h"

// TODO: fix multiple definition of findMemoryType function (with Buffer)
namespace ImgMemFunc {
    /**
     * Return
     *   physical device에서 memProperty를 충족하는 memory type의 index 반환
     *
     *   지원되는 memory type이 없다면 ~0U 반환
     */
    unsigned int findMemoryType(unsigned int memType, uint32 memProperty) {
        VkPhysicalDeviceMemoryProperties gpuMemSpec;
        vkGetPhysicalDeviceMemoryProperties(App::device(), &gpuMemSpec);

        unsigned int memTypeIdx{ };
        for (const auto& [memPropFlag, _]: std::span<VkMemoryType>(gpuMemSpec.memoryTypes, gpuMemSpec.memoryTypeCount)) {
            if ((memType & (1 << memTypeIdx)) and ((memPropFlag & memProperty) == memProperty))
                return memTypeIdx;

            ++memTypeIdx;
        }

        return ~0U;
    }
    /**
     * Return
     *   memory 할당 성공시 true  반환
     *   memory 할당 실패시 false 반환
     */
    VkDeviceMemory allocMemory(VkImage imgHandle, Usage::Image imgUsage, Property::Memory memProperty) {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(App::device(), imgHandle, &memRequirements);

        uint32 memTypeIndex = findMemoryType(memRequirements.memoryTypeBits, static_cast<uint32>(memProperty));
        if (memTypeIndex == ~0U)
            throw std::runtime_error("[ERROR]: Failed to find suitable memory type!");

        VkMemoryAllocateInfo info {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = memTypeIndex
        };

        VkDeviceMemory memHandle = { };
        vkAllocateMemory(App::device(), &info, nullptr, &memHandle);

        return memHandle;
    }
}

/* -------- Constructors & Destructor -------- */
Image::Image()
    : Resource{}
{ }
Image::~Image() noexcept { destroy(); }
/* ------------------------------------------- */

/* ------------ Override Functions ----------- */
void Image::destroy() {
    if (mIsCreated) {
        vkFreeMemory(App::device(), mMemoryHandle, nullptr);
        vkDestroyImage(App::device(), mResourceHandle, nullptr);
    }
}
/* ------------------------------------------- */

/* ---------- Operator Overloadings ---------- */
Image::operator VkImage_T*() const noexcept { return mResourceHandle; }
/* ------------------------------------------- */

/**
 * Return
 *   image 생성 성공시 true  반환
 *   image 생성 실패시 false 반환
*/
bool Image::create(int32 type, Type::ImageFormat format, Type::Extent3D extent, Usage::Image imgUsage, Property::Memory memProperty) {
    if (!mIsCreated) {
        mResourceFlag = imgUsage;
        mFormat       = format;

        mMemoryFlag   = memProperty;

        // initialLayout은 Layout::Image::UNDEFINED 혹은 Layout::Image::PREINITIALIZED 만 사용 가능 (vulkan spec)
        VkImageCreateInfo info {
            .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType     = static_cast<VkImageType>(type),
            .format        = static_cast<VkFormat>(format),
            .extent        = { extent.width, extent.height, extent.depth },
            .mipLevels     = 1,
            .arrayLayers   = 1,
            .samples       = VK_SAMPLE_COUNT_1_BIT,
            .tiling        = VK_IMAGE_TILING_OPTIMAL,
            .usage         = static_cast<VkImageUsageFlags>(imgUsage),
            .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = static_cast<VkImageLayout>(Layout::Image::UNDEFINED)
        };

        if (vkCreateImage(App::device(), &info, nullptr, &mResourceHandle) != VK_SUCCESS)
            throw std::runtime_error("[ERROR]: Failed to create image!");

        if ((mMemoryHandle = ImgMemFunc::allocMemory(mResourceHandle, mResourceFlag, mMemoryFlag)); mMemoryHandle == nullptr)
            throw std::runtime_error("[ERROR]: Failed to allocate memory!");

        if (vkBindImageMemory(App::device(), mResourceHandle, mMemoryHandle, 0) != VK_SUCCESS)
            throw std::runtime_error("[ERROR]: Failed to bind memory!");

        mIsCreated = true;
    }

    return mIsCreated;
}

// TODO: VkImageSubresourceRange (struct)
void Image::setLayout(VkCommandBuffer_T* commandBuf, Layout::Image newLayout) {
    VkImageMemoryBarrier imgMemBarrier {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = static_cast<VkImageLayout>(mLayout),
        .newLayout           = static_cast<VkImageLayout>(newLayout),
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,     // resource의 queue family 소유권 이전과 관련
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,     // resource의 queue family 소유권 이전과 관련
        .image               = mResourceHandle,
        .subresourceRange    = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,                                // mipmap level
            .layerCount = 1                                 // entire data (no array data)
        }
    };

    // TODO: VkAccessFlagBits (uint32)
    if      (mLayout == Layout::Image::TRANSFER_SRC_OPTIMAL) imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    else if (mLayout == Layout::Image::TRANSFER_DST_OPTIMAL) imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    if      (newLayout == Layout::Image::TRANSFER_SRC_OPTIMAL) imgMemBarrier.dstAccessMask     = VK_ACCESS_TRANSFER_READ_BIT;
    else if (newLayout == Layout::Image::TRANSFER_DST_OPTIMAL) imgMemBarrier.dstAccessMask     = VK_ACCESS_TRANSFER_WRITE_BIT;
    else if (newLayout == Layout::Image::SHADER_READ_ONLY_OPTIMAL) imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    else if (newLayout == Layout::Image::COLOR_ATTACHMENT_OPTIMAL) imgMemBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    mLayout = newLayout;

    // TODO: VkPipelineStageFlagBits (uint32)
    vkCmdPipelineBarrier(
        commandBuf,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        1, &imgMemBarrier
    );
}



/* ----------------- Getters ----------------- */
Type::ImageFormat Image::format() const noexcept { return mFormat; }
/* ------------------------------------------- */