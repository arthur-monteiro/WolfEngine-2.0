#include "ReadableBuffer.h"

#include <Configuration.h>

// TEMP
#include <vulkan/vulkan_core.h>

Wolf::ReadableBuffer::ReadableBuffer(uint64_t size, uint32_t additionalUsageFlags)
{
    uint32_t usageFlags =  additionalUsageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_buffers.resize(g_configuration->getMaxCachedFrames());
    for (ResourceUniqueOwner<Buffer>& buffer : m_buffers)
    {
        buffer.reset(Buffer::createBuffer(size, usageFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
    }
}
