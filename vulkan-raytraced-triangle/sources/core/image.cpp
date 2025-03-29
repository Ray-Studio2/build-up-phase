#include "core/image.hpp"
#include "core/memory.hpp"
#include "core/app.hpp"

/* -------- Constructors & Destructor -------- */
Image::Image() { }
Image::Image(VkImageType imgType, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage) { create(imgType, format, extent, usage); }
Image::~Image() noexcept {
    if (*mMemory) delete mMemory;
    vkDestroyImage(App::device(), mHandle, nullptr);
}
/* ------------------------------------------- */

/* ---------- Operator Overloadings ---------- */
Image::operator bool() const noexcept { return mIsCreated; }
Image::operator VkImage() const noexcept { return mHandle; }
/* ------------------------------------------- */

/* Return
 *   image 생성 성공시 true
 *   image 생성 실패시 false
*/
bool Image::create(VkImageType imgType, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage) {
    if (!mIsCreated) {
        // initialLayout은 VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED 두 가지만 사용가능 (spec 참조)
        mInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = imgType,
            .format = format,
            .extent = extent,
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        if (vkCreateImage(App::device(), &mInfo, nullptr, &mHandle) == VK_SUCCESS)
            mIsCreated = true;
    }

    return mIsCreated;
}

void Image::bindMemory(Memory* memory, uint32 offset) {
    if (vkBindImageMemory(App::device(), mHandle, *memory, offset) == VK_SUCCESS)
        mMemory = memory;
}