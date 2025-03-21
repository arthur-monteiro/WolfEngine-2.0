#include "CommandBuffer.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/CommandBufferVulkan.h"
#endif

Wolf::CommandBuffer* Wolf::CommandBuffer::createCommandBuffer(QueueType queueType, bool isTransient)
{
#ifdef WOLF_VULKAN
	return new CommandBufferVulkan(queueType, isTransient);
#else
	return nullptr;
#endif
}