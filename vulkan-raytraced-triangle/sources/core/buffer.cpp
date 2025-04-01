#include "core/buffer.hpp"
#include "core/app.hpp"

#include <stdexcept>
#include <cstring>          // memcpy
#include <span>
#include "vulkan/vulkan.h"

// TODO: fix multiple definition of findMemoryType function (with Image)
namespace BufMemFunc {
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
    VkDeviceMemory allocMemory(VkBuffer bufHandle, Usage::Buffer bufUsage, Property::Memory memProperty) {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(App::device(), bufHandle, &memRequirements);

        uint32 memTypeIndex = findMemoryType(memRequirements.memoryTypeBits, static_cast<uint32>(memProperty));
        if (memTypeIndex == ~0U)
            throw std::runtime_error("[ERROR]: Failed to find suitable memory type!");

        VkMemoryAllocateInfo info {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = memTypeIndex
        };

        // Usage::Buffer::SHADER_DEVICE_ADDRESS 가 있으면
        // 아래의 next flag 처리를 해야 GPU상의 memory 주소를 찾을 수 있음
        if (bufUsage & Usage::Buffer::SHADER_DEVICE_ADDRESS) {
            VkMemoryAllocateFlagsInfo memAllocFlagsInfo {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
                .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR
            };

            // next chain 연결
            info.pNext = &memAllocFlagsInfo;
        }

        VkDeviceMemory memHandle = { };
        vkAllocateMemory(App::device(), &info, nullptr, &memHandle);

        return memHandle;
    }
}

/* -------- Constructors & Destructor -------- */
Buffer::Buffer()
    : Resource{}
{ }
Buffer::~Buffer() noexcept { destroy(); }
/* ------------------------------------------- */

/* ------------ Override Functions ----------- */
void Buffer::destroy() {
    if (mIsCreated) {
        vkFreeMemory(App::device(), mMemoryHandle, nullptr);
        vkDestroyBuffer(App::device(), mResourceHandle, nullptr);
    }
}
/* ------------------------------------------- */

/* ---------- Operator Overloadings ---------- */
Buffer::operator VkBuffer_T*() const noexcept { return mResourceHandle; }
/* ------------------------------------------- */

/**
 * Return
 *   buffer 생성 성공시 true  반환
 *   buffer 생성 실패시 false 반환
 */
bool Buffer::create(uint64 size, Usage::Buffer bufUsage, Property::Memory memProperty) {
    if (!mIsCreated) {
        mResourceFlag = bufUsage;
        mMemoryFlag   = memProperty;

        VkBufferCreateInfo info {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size  = size,
            .usage = static_cast<VkBufferUsageFlags>(bufUsage)
        };

        if (vkCreateBuffer(App::device(), &info, nullptr, &mResourceHandle) != VK_SUCCESS)
            throw std::runtime_error("[ERROR]: Failed to create buffer!");

        if ((mMemoryHandle = BufMemFunc::allocMemory(mResourceHandle, mResourceFlag, mMemoryFlag)); mMemoryHandle == nullptr)
            throw std::runtime_error("[ERROR]: Failed to allocate memory!");

        if (vkBindBufferMemory(App::device(), mResourceHandle, mMemoryHandle, 0) != VK_SUCCESS)
            throw std::runtime_error("[ERROR]: Failed to bind memory!");

        mIsCreated = true;
    }

    return mIsCreated;
}

// TODO: exception
void Buffer::write(const void* data, uint64 size, uint64 offset) {
    if (mIsCreated) {
        void* dst{ };

        vkMapMemory(App::device(), mMemoryHandle, offset, size, 0, &dst);
        memcpy(dst, data, size);
        vkUnmapMemory(App::device(), mMemoryHandle);
    }
}