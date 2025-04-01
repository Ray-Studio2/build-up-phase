#include "core/imageView.hpp"
#include "core/image.hpp"
#include "core/app.hpp"

#include <stdexcept>
#include "vulkan/vulkan.h"

/* -------- Constructors & Destructor -------- */
ImageView::ImageView() { }
ImageView::~ImageView() noexcept { destroy(); }
/* ------------------------------------------- */

void ImageView::destroy() { vkDestroyImageView(App::device(), mHandle, nullptr); }

/* ---------- Operator Overloadings ---------- */
ImageView::operator           bool() const noexcept { return mIsCreated; }
ImageView::operator VkImageView_T*() const noexcept { return mHandle;    }
/* ------------------------------------------- */

/**
 * TODO LIST
 *   VkImageSubresourceRange (struct)
 *   VkComponentMapping      (struct)
 */
/**
 * Return
 *   imageView 생성 성공시 true  반환
 *   imageView 생성 실패시 false 반환
 */
bool ImageView::create(const Image& image) {
    if (!mIsCreated) {
        VkImageViewCreateInfo info {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image            = image,
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = static_cast<VkFormat>(image.format()),
            .components       = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,        // mipmap level
                .layerCount = 1         // entire data (no array data)
            }
        };

        if (vkCreateImageView(App::device(), &info, nullptr, &mHandle) != VK_SUCCESS)
            throw std::runtime_error("[ERROR]: Failed to create imageView!");

        mIsCreated = true;
    }

    return mIsCreated;
}