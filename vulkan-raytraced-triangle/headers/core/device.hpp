#pragma once

#include "vulkan/vulkan.h"

#include <vector>
using std::vector;

/* TODO
 *   - extension, feature 관리 -> device create info 관리
 *   - device, queue info 구조체 보관 여부 결정
 *   - create() 정리
 *   - findQueueFamily() 의 필요한 QUEUE 요구사항 관리 방식 생각
*/

class Device {
    Device(const Device&) = delete;
    Device(Device&&) noexcept = delete;

    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) noexcept = delete;

    private:
        using uint32 = unsigned int;

    public:
        Device();
        Device(const vector<const char*>& reqExtensions);
        ~Device() noexcept;

        explicit operator bool() const noexcept;
        operator VkPhysicalDevice() const noexcept;
        operator VkDevice() const noexcept;

        bool create(const vector<const char*>& reqExtensions);

        VkQueue queue() const noexcept;
        uint32 queueFamilyIndex() const noexcept;

    private:
        bool mIsCreated;
        uint32 mQueueFamilyIdx{ };

        VkPhysicalDevice mPhysicalHandle{ };
        VkDevice         mLogicalHandle { };
        VkQueue          mQueue{ };
};