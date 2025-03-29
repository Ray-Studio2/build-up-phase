#pragma once

#include "vulkan/vulkan.h"

/* TODO
 *   - write 함수 size 지정할 수 있도록 수정
 *   - write 함수 Memory Class로 이관 여부 결정
*/

class Memory;

class Buffer {
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept = delete;

    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) noexcept = delete;

    private:
        using uint32 = unsigned int;

    public:
        Buffer();
        Buffer(VkDeviceSize size, VkBufferUsageFlags usage);
        ~Buffer() noexcept;

        explicit operator bool() const noexcept;
        operator VkBuffer() const noexcept;

        bool create(VkDeviceSize size, VkBufferUsageFlags usage);
        void write(const void* data);

        void bindMemory(Memory* memory, uint32 offset = { });

        VkDeviceSize size() const noexcept;
        VkBufferUsageFlags usage() const noexcept;

    private:
        bool mIsCreated{ };

        VkBufferCreateInfo mInfo{ };
        VkBuffer mHandle{ };

        Memory* mMemory{ };
};