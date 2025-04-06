#include "UniformBuffer.h"

#include <Configuration.h>
#include <RuntimeContext.h>

// TEMP
#include <vulkan/vulkan_core.h>

#include "Buffer.h"

Wolf::UniformBuffer::UniformBuffer(uint64_t size)
{
	m_buffers.resize(g_configuration->getMaxCachedFrames());

	for (ResourceUniqueOwner<Buffer>& buffer : m_buffers)
	{
		buffer.reset(Buffer::createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	}
}

void Wolf::UniformBuffer::transferCPUMemory(const void* data, uint64_t srcSize, uint64_t srcOffset) const
{
	uint32_t bufferIdx = g_runtimeContext->getCurrentCPUFrameNumber() % g_configuration->getMaxCachedFrames();
	m_buffers[bufferIdx]->transferCPUMemory(data, srcSize, srcOffset);
}
