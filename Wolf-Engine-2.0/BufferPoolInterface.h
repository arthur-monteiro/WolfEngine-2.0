#pragma once

#include <cstdint>

#include <Buffer.h>

namespace Wolf
{
    class BufferPoolInterface
    {
    public:
        virtual ~BufferPoolInterface() = default;

        struct BufferPoolInstance
        {
            uint32_t m_bufferIdx;
            uint32_t m_bufferOffset;
            uint32_t m_bufferSize;
        };
        [[nodiscard]] virtual BufferPoolInstance allocate(uint32_t requestedSize, Buffer::BufferUsageFlags usageFlags, uint32_t itemSize) = 0;
        virtual void deallocate(const BufferPoolInstance& bufferPoolInstance) = 0;

        virtual ResourceNonOwner<Buffer> getBuffer(const BufferPoolInstance& bufferPoolInstance) = 0;
    };
}
