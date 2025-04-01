#pragma once

#include "core/interfaces/resource.hpp"

class Buffer final: public Resource<VkBuffer_T, Usage::Buffer> {
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept = delete;

    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) noexcept = delete;

    public:
        Buffer();
        ~Buffer() noexcept override;

        void destroy() override;

        operator VkBuffer_T*() const noexcept override;

        bool create(uint64 size, Usage::Buffer bufUsage, Property::Memory memUsage);

        void write(const void* data, uint64 size, uint64 offset = 0);
};