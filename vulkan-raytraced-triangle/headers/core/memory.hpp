#pragma once

#include "vulkan/vulkan.h"

/* TODO
 *   - info 구조체 보관 여부 결정
 *   - create() 정리
 *   - size 지정할 수 있도록 수정
 *
 *   idea) create 과정에서 bind를 해주면 되는 것 아닌가?
*/

class Buffer;
class Image;

class Memory {
    Memory(const Memory&) = delete;
    Memory(Memory&&) noexcept = delete;

    Memory& operator=(const Memory&) = delete;
    Memory& operator=(Memory&&) noexcept = delete;

    public:
        Memory();
        Memory(const Buffer& buffer, VkMemoryPropertyFlags memReqFlags);
        Memory(const Image& image, VkMemoryPropertyFlags memReqFlags);
        ~Memory() noexcept;

        explicit operator bool() const noexcept;
        operator VkDeviceMemory() const noexcept;

        bool create(const Buffer& buffer, VkMemoryPropertyFlags reqMemFlags);
        bool create(const Image& image, VkMemoryPropertyFlags reqMemFlags);

    private:
        bool mIsCreated{ };

        VkDeviceMemory mHandle{ };
};

