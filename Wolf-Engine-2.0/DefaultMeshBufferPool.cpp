#include "DefaultMeshBufferPool.h"

#include "vulkan/vulkan_core.h" // TEMP for VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT

Wolf::DefaultMeshBufferPool::DefaultMeshBufferPool(const std::vector<PoolSize>& poolSizes) : m_poolSizes(poolSizes)
{
}

bool Wolf::DefaultMeshBufferPool::hasEnoughSpace(uint32_t requestedSize, Buffer::BufferUsageFlags usageFlags, uint32_t itemSize)
{
    uint32_t bufferIdx = createBufferIfNotExists(requestedSize, usageFlags, itemSize);
    ResourceUniqueOwner<OwningBuffer>& owningBuffer = m_buffers[bufferIdx];

    return owningBuffer->hasEnoughSpace(requestedSize);
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
    if (bufferPoolInstance.m_bufferIdx >= m_buffers.size())
    {
        Debug::sendCriticalError("Buffer idx is invalid");
        return;
    }

    m_buffers[bufferPoolInstance.m_bufferIdx]->deallocate(bufferPoolInstance.m_bufferOffset, bufferPoolInstance.m_bufferSize);
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

    m_freeChunks.push_back({ 0, bufferSize });
}

bool Wolf::DefaultMeshBufferPool::OwningBuffer::hasEnoughSpace(uint32_t requestedSize) const
{
    for (const FreeChunk& freeChunk : m_freeChunks)
    {
        if (freeChunk.m_size >= requestedSize)
        {
            return true;
        }
    }
    return false;
}

uint32_t Wolf::DefaultMeshBufferPool::OwningBuffer::allocate(uint32_t requestedSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (uint32_t freeChunkIdx = 0; freeChunkIdx < m_freeChunks.size(); freeChunkIdx++)
    {
        FreeChunk& freeChunk = m_freeChunks[freeChunkIdx];

        if (freeChunk.m_size >= requestedSize)
        {
            uint32_t allocationOffset = freeChunk.m_offset;

            if (freeChunk.m_size == requestedSize)
            {
                m_freeChunks.erase(m_freeChunks.begin() + freeChunkIdx);
            }
            else
            {
                freeChunk.m_offset += requestedSize;
                freeChunk.m_size -= requestedSize;
            }

            m_totalAllocatedSize += requestedSize;
            return allocationOffset;
        }
    }

    Debug::sendCriticalError("Can't allocate memory, buffer is full or too fragmented. Current usage is " + std::to_string(getUsage()));
    return -1;
}

void Wolf::DefaultMeshBufferPool::OwningBuffer::deallocate(uint32_t offset, uint32_t size)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    FreeChunk newChunk{ offset, size };
    auto it = std::lower_bound(m_freeChunks.begin(), m_freeChunks.end(), newChunk,
        [](const FreeChunk& a, const FreeChunk& b)
        {
            return a.m_offset < b.m_offset;
        });

    auto insertedIt = m_freeChunks.insert(it, newChunk);
    m_totalAllocatedSize -= size;

    // Merge with right
    if (insertedIt + 1 != m_freeChunks.end())
    {
        auto nextIt = insertedIt + 1;
        if (insertedIt->m_offset + insertedIt->m_size == nextIt->m_offset)
        {
            insertedIt->m_size += nextIt->m_size;
            m_freeChunks.erase(nextIt);
        }
    }

    // Merge with left
    if (insertedIt != m_freeChunks.begin())
    {
        auto prevIt = insertedIt - 1;
        if (prevIt->m_offset + prevIt->m_size == insertedIt->m_offset)
        {
            prevIt->m_size += insertedIt->m_size;
            m_freeChunks.erase(insertedIt);
        }
    }
}

float Wolf::DefaultMeshBufferPool::OwningBuffer::getUsage() const
{
    return static_cast<float>(m_totalAllocatedSize) / static_cast<float>(m_buffer->getSize());
}
