#pragma once

#include "vulkan/vulkan.h"

class Memory;

class Image {
    Image(const Image&) = delete;
    Image(Image&&) noexcept = delete;

    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) noexcept = delete;

    private:
        using uint32 = unsigned int;

    public:
        Image();
        Image(VkImageType imgType, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage);
        ~Image() noexcept;

        explicit operator bool() const noexcept;
        operator VkImage() const noexcept;

        bool create(VkImageType imgType, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage);

        void bindMemory(Memory* memory, uint32 offset = { });

    private:
        bool mIsCreated{ };

        VkImageCreateInfo mInfo{ };
        VkImage mHandle{ };

        Memory* mMemory{ };
};