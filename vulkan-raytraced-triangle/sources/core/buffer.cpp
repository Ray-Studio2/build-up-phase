#include "core/buffer.hpp"
#include "core/memory.hpp"
#include "core/app.hpp"

#include <cstring>          // memcpy

/* -------- Constructors & Destructor -------- */
Buffer::Buffer() { }
Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage) { create(size, usage); }
Buffer::~Buffer() noexcept {
    if (*mMemory) delete mMemory;
    vkDestroyBuffer(App::device(), mHandle, nullptr);
}
/* ------------------------------------------- */

/* ---------- Operator Overloadings ---------- */
Buffer::operator bool() const noexcept { return mIsCreated; }
Buffer::operator VkBuffer() const noexcept { return mHandle; }
/* ------------------------------------------- */

/* Return
 *   buffer 생성 성공시 true
 *   buffer 생성 실패시 false
*/
bool Buffer::create(VkDeviceSize size, VkBufferUsageFlags usage) {
    if (!mIsCreated) {
        mInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage
        };

        if (vkCreateBuffer(App::device(), &mInfo, nullptr, &mHandle) == VK_SUCCESS)
            mIsCreated = true;
    }

    return mIsCreated;
}
void Buffer::write(const void* data) {
    void* dst{ };

    vkMapMemory(App::device(), *mMemory, 0, mInfo.size, 0, &dst);
    memcpy(dst, data, mInfo.size);
    vkUnmapMemory(App::device(), *mMemory);
}

void Buffer::bindMemory(Memory* memory, uint32 offset) {
    if (vkBindBufferMemory(App::device(), mHandle, *memory, offset) == VK_SUCCESS)
        mMemory = memory;
}

/* ----------------- Getters ----------------- */
VkDeviceSize Buffer::size() const noexcept { return mInfo.size; }
VkBufferUsageFlags Buffer::usage() const noexcept { return mInfo.usage; }
/* ------------------------------------------- */