#pragma once

#include "core/types.hpp"

/**
 * Resource -> Buffer, Image 클래스에서 상속
 *   ResourceType은 VkBuffer_T 혹은 VkImage_T
 *   ResourceFlag는 Usage::Buffer 혹은 Usage::Image
 */
template <typename ResourceType, typename ResourceFlag>
class Resource {
    Resource(const Resource&) = delete;
    Resource(Resource&&) noexcept = delete;

    Resource& operator=(const Resource&) = delete;
    Resource& operator=(Resource&&) noexcept = delete;

    public:
        Resource();
        virtual ~Resource() noexcept;

        virtual void destroy() = 0;

        operator bool()              const noexcept;
        operator VkDeviceMemory_T*() const noexcept;

        virtual operator ResourceType*() const noexcept = 0;

    protected:
        bool mIsCreated = { };

        ResourceFlag     mResourceFlag = { };
        Property::Memory mMemoryFlag   = { };

        ResourceType*     mResourceHandle = { };
        VkDeviceMemory_T* mMemoryHandle   = { };
};

template <typename ResourceType, typename ResourceFlag> Resource<ResourceType, ResourceFlag>::Resource() { }
template <typename ResourceType, typename ResourceFlag> Resource<ResourceType, ResourceFlag>::~Resource() noexcept { }

template <typename ResourceType, typename ResourceFlag>
Resource<ResourceType, ResourceFlag>::operator bool() const noexcept { return mIsCreated; }
template <typename ResourceType, typename ResourceFlag>
Resource<ResourceType, ResourceFlag>::operator VkDeviceMemory_T*() const noexcept { return mMemoryHandle; }