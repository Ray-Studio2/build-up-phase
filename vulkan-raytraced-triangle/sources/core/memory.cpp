#include "core/memory.hpp"
#include "core/buffer.hpp"
#include "core/image.hpp"
#include "core/app.hpp"

#include <iostream>
#include <span>
using std::cout, std::endl;
using std::span;

constexpr unsigned int MEMORY_TYPE_NOT_FOUND = ~0U;

/* Return
 *   physical device 내에서 reqMemFlags를 충족하면서,
 *   resource에 대해 지원되는 memory type이 있으면 그 memory type의 index
 *
 *   지원되는 memory type을 찾을 수 없다면 MEMORY_TYPE_NOT_FOUND (~0U)
*/
unsigned int findMemoryType(unsigned int memType, VkMemoryPropertyFlags reqMemFlags) {
    VkPhysicalDeviceMemoryProperties gpuMemSpec;
    vkGetPhysicalDeviceMemoryProperties(App::device(), &gpuMemSpec);

    unsigned int memTypeIdx{ };
    for (const auto& [memPropFlag, _]: span<VkMemoryType>(gpuMemSpec.memoryTypes, gpuMemSpec.memoryTypeCount)) {
        if ((memType & (1 << memTypeIdx)) and ((memPropFlag & reqMemFlags) == reqMemFlags))
            return memTypeIdx;

        ++memTypeIdx;
    }

    return MEMORY_TYPE_NOT_FOUND;
}

/* -------- Constructors & Destructor -------- */
Memory::Memory() { }
Memory::Memory(const Buffer& buffer, VkMemoryPropertyFlags memReqFlags) { create(buffer, memReqFlags); }
Memory::Memory(const Image& image, VkMemoryPropertyFlags memReqFlags) { create(image, memReqFlags); }
Memory::~Memory() noexcept { vkFreeMemory(App::device(), mHandle, nullptr); }
/* ------------------------------------------- */

/* ---------- Operator Overloadings ---------- */
Memory::operator bool() const noexcept { return mIsCreated; }
Memory::operator VkDeviceMemory() const noexcept { return mHandle; }
/* ------------------------------------------- */

/* Return
 *   memory 할당 성공시 true
 *   memory 할당 실패시 false
*/
bool Memory::create(const Buffer& buffer, VkMemoryPropertyFlags reqMemFlags) {
    if (!mIsCreated) {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(App::device(), buffer, &memRequirements);

        unsigned int memTypeIndex = findMemoryType(memRequirements.memoryTypeBits, reqMemFlags);
        if (memTypeIndex == MEMORY_TYPE_NOT_FOUND)
            cout << "[ERROR]: Memory Type Not Found" << endl;

        else {
            VkMemoryAllocateInfo memAllocInfo {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memRequirements.size,
                .memoryTypeIndex = memTypeIndex
            };

            // VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT 가 켜져 있는 경우
            // 이 설정을 해주는 것으로 GPU상의 버퍼 주소를 찾을 수 있음 -> device address가 필요한 경우에 사용
            if (buffer.usage() & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
                VkMemoryAllocateFlagsInfo memAllocFlagsInfo {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
                    .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR
                };

                // Next Chain 연결
                memAllocInfo.pNext = &memAllocFlagsInfo;
            }

            if (vkAllocateMemory(App::device(), &memAllocInfo, nullptr, &mHandle) == VK_SUCCESS)
                mIsCreated = true;
        }
    }

    return mIsCreated;
}
/* Return
 *   memory 할당 성공시 true
 *   memory 할당 실패시 false
*/
bool Memory::create(const Image& image, VkMemoryPropertyFlags reqMemFlags) {
    if (!mIsCreated) {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(App::device(), image, &memRequirements);

        unsigned int memTypeIndex = findMemoryType(memRequirements.memoryTypeBits, reqMemFlags);
        if (memTypeIndex == MEMORY_TYPE_NOT_FOUND)
            cout << "[ERROR]: Memory Type Not Found" << endl;

        else {
            VkMemoryAllocateInfo memAllocInfo {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = memRequirements.size,
                .memoryTypeIndex = memTypeIndex
            };

            if (vkAllocateMemory(App::device(), &memAllocInfo, nullptr, &mHandle) == VK_SUCCESS)
                mIsCreated = true;
        }
    }

    return mIsCreated;
}