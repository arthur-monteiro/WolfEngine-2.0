#pragma once

#include <Buffer.h>
#include <ResourceUniqueOwner.h>

#include "DynamicStableArray.h"
#include "BufferPoolInterface.h"
#include "DynamicResourceUniqueOwnerArray.h"

namespace Wolf
{
    class DefaultMeshBufferPool : public BufferPoolInterface
    {
    public:
        struct PoolSize
        {
            uint32_t m_minimumPoolSize;
            uint32_t m_itemSize;
            uint32_t m_bufferUsageFlags;
        };
        DefaultMeshBufferPool(const std::vector<PoolSize>& poolSizes);
        ~DefaultMeshBufferPool() override = default;

        [[nodiscard]] BufferPoolInstance allocate(uint32_t requestedSize, Buffer::BufferUsageFlags usageFlags, uint32_t itemSize) override;
        void deallocate(const BufferPoolInstance& bufferPoolInstance) override;

        ResourceNonOwner<Buffer> getBuffer(const BufferPoolInstance& bufferPoolInstance) override;

    private:
        uint32_t createBufferIfNotExists(uint32_t minimumSize, Buffer::BufferUsageFlags usageFlags, uint32_t vertexSize);

        std::vector<PoolSize> m_poolSizes;

        class OwningBuffer
        {
        public:
            OwningBuffer(uint32_t bufferSize, Buffer::BufferUsageFlags usageFlags, uint32_t vertexSize);
            uint32_t allocate(uint32_t requestedSize);

            Buffer::BufferUsageFlags getBufferUsageFlags() const { return m_bufferUsageFlags; };
            uint32_t getVertexSize() const { return m_vertexSize; };
            ResourceNonOwner<Buffer> getBuffer() { return m_buffer.createNonOwnerResource();}

        private:
            float getUsage() const;

            ResourceUniqueOwner<Buffer> m_buffer;
            Buffer::BufferUsageFlags m_bufferUsageFlags;
            uint32_t m_vertexSize;

            std::mutex m_mutex;
            uint32_t m_currentOffset = 0;
        };
        DynamicResourceUniqueOwnerArray<OwningBuffer, 16> m_buffers;
        std::mutex m_buffersMutex;
    };
}
