#pragma once

#include "core/types.hpp"

class Image;

class ImageView {
    ImageView(const ImageView&) = delete;
    ImageView(ImageView&&) noexcept = delete;

    ImageView& operator=(const ImageView&) = delete;
    ImageView& operator=(ImageView&&) noexcept = delete;

    public:
        ImageView();
        ~ImageView() noexcept;

        void destroy();

        explicit operator  bool() const noexcept;
        operator VkImageView_T*() const noexcept;

        bool create(const Image& image);

    private:
        bool mIsCreated = { };

        VkImageView_T* mHandle = { };
};