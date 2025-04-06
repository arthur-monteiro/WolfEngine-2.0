#include "CommandBuffer.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/CommandBufferVulkan.h"
#endif

Wolf::CommandBuffer* Wolf::CommandBuffer::createCommandBuffer(QueueType queueType, bool isTransient, bool preRecord)
{
#ifdef WOLF_VULKAN
	return new CommandBufferVulkan(queueType, isTransient, preRecord);
#else
	return nullptr;
#endif
}