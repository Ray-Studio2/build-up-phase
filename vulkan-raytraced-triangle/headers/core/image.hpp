#pragma once

#include "core/interfaces/resource.hpp"

class Image: public Resource<VkImage_T, Usage::Image> {
    Image(const Image&) = delete;
    Image(Image&&) noexcept = delete;

    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) noexcept = delete;

    public:
        Image();
        ~Image() noexcept override;

        void destroy() override;

        operator VkImage_T*() const noexcept override;

        bool create(
            int32 type, Type::ImageFormat format, Type::Extent3D extent,
            Usage::Image imgUsage, Property::Memory memProperty
        );

        void setLayout(VkCommandBuffer_T* commandBuf, Layout::Image newLayout);

        Type::ImageFormat format() const noexcept;

    protected:
        Type::ImageFormat mFormat = { };
        Layout::Image     mLayout = { };
};