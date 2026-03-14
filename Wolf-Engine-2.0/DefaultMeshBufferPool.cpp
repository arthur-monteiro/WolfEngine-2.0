#include "DefaultMeshBufferPool.h"

#include "vulkan/vulkan_core.h" // TEMP for VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT

Wolf::DefaultMeshBufferPool::DefaultMeshBufferPool(const std::vector<PoolSize>& poolSizes) : m_poolSizes(poolSizes)
{
}

Wolf::BufferPoolInterface::BufferPoolInstance Wolf::DefaultMeshBufferPool::allocate(uint32_t requestedSize, Buffer::BufferUsageFlags usageFlags, uint32_t itemSize)
{
    uint32_t bufferIdx = createBufferIfNotExists(requestedSize, usageFlags, itemSize);
    ResourceUniqueOwner<OwningBuffer>& owningBuffer = m_buffers[bufferIdx];

    BufferPoolInstance r{};
    r.m_bufferIdx = bufferIdx;
    r.m_bufferOffset = owningBuffer->allocate(requestedSize);
    r.m_bufferSize = requestedSize;

    return r;
}

void Wolf::DefaultMeshBufferPool::deallocate(const BufferPoolInstance& bufferPoolInstance)
{
    // TODO
}

Wolf::ResourceNonOwner<Wolf::Buffer> Wolf::DefaultMeshBufferPool::getBuffer(const BufferPoolInstance& bufferPoolInstance)
{
    return m_buffers[bufferPoolInstance.m_bufferIdx]->getBuffer();
}

uint32_t Wolf::DefaultMeshBufferPool::createBufferIfNotExists(uint32_t minimumSize, Buffer::BufferUsageFlags usageFlags, uint32_t vertexSize)
{
    for (uint32_t i = 0; i < m_buffers.size(); i++)
    {
        if ((m_buffers[i]->getBufferUsageFlags() & usageFlags) == usageFlags && m_buffers[i]->getVertexSize() == vertexSize)
        {
            // TODO: check if buffer is not full
            return i;
        }
    }

    PoolSize* poolSize = nullptr;
    for (PoolSize& storedPoolSize : m_poolSizes)
    {
        if (storedPoolSize.m_itemSize == vertexSize && storedPoolSize.m_bufferUsageFlags == usageFlags)
        {
            poolSize = &storedPoolSize;
        }
    }

    uint32_t bufferSize = std::max(minimumSize, poolSize ? poolSize->m_minimumPoolSize : 0);

    m_buffersMutex.lock();
    uint32_t index = m_buffers.size();
    m_buffers.emplace_back(new OwningBuffer(bufferSize, usageFlags, vertexSize));
    m_buffersMutex.unlock();

    return index;
}

Wolf::DefaultMeshBufferPool::OwningBuffer::OwningBuffer(uint32_t bufferSize, Buffer::BufferUsageFlags usageFlags, uint32_t vertexSize)
{
    Debug::sendInfo("DefaultMeshBufferPool: Creating new buffer, usage flags is " + std::to_string(usageFlags) + ", vertex size is " + std::to_string(vertexSize) + " bytes");

    m_buffer.reset(Buffer::createBuffer(bufferSize, usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
    m_buffer->setName("Default mesh buffer pool usage " + std::to_string(usageFlags) + " vertex size " + std::to_string(vertexSize) + " (DefaultMeshBufferPool::OwningBuffer::m_buffer)");
    m_buffer->registerUsageCallback([this]() { return getUsage(); });
    m_bufferUsageFlags = usageFlags;
    m_vertexSize = vertexSize;
}

uint32_t Wolf::DefaultMeshBufferPool::OwningBuffer::allocate(uint32_t requestedSize)
{
    m_mutex.lock();
    uint32_t allocationOffset = m_currentOffset;
    m_currentOffset += requestedSize;
    m_mutex.unlock();

    if (m_currentOffset > m_buffer->getSize())
    {
        Debug::sendCriticalError("Can't allocate memory, buffer is full");
    }

    return allocationOffset;
}

float Wolf::DefaultMeshBufferPool::OwningBuffer::getUsage() const
{
    return static_cast<float>(m_currentOffset) / static_cast<float>(m_buffer->getSize());
}
