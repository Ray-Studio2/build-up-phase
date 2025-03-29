#pragma once

#include "vulkan/vulkan.h"

/* TODO
 *   - extension, layer 관리 -> instance create info 관리
 *   - info 구조체 보관 여부 결정
 *   - glfwGetRequiredInstanceExtensions() 함수 처리
*/

class Instance {
    Instance(const Instance&) = delete;
    Instance(Instance&&) noexcept = delete;

    Instance& operator=(const Instance&) = delete;
    Instance& operator=(Instance&&) noexcept = delete;

    private:
        using uint32 = unsigned int;

    public:
        Instance();
        Instance(const char* appName, uint32 apiVer, uint32 appVer = { }, const char* engineName = { }, uint32 engineVer = { });
        ~Instance() noexcept;

        explicit operator bool() const noexcept;
        operator VkInstance() const noexcept;

        bool create(const char* appName, uint32 apiVer, uint32 appVer = { }, const char* engineName = { }, uint32 engineVer = { });

    private:
        static bool mIsCreated;
        static VkInstance mHandle;

    #ifdef DEBUG
        static VkDebugUtilsMessengerEXT mDebugMessenger;
    #endif
};