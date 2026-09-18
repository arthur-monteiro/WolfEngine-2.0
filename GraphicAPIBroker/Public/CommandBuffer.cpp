#include "CommandBuffer.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/CommandBufferVulkan.h"
#endif

Wolf::CommandBuffer* Wolf::CommandBuffer::createCommandBuffer(QueueType queueType, bool isTransient, const std::string& name, bool preRecord)
{
#ifdef WOLF_VULKAN
	return new CommandBufferVulkan(queueType, isTransient, name, preRecord);
#else
	return nullptr;
#endif
}

uint32_t Wolf::CommandBuffer::getDrawIndexedIndirectCommandStructureSize()
{
#ifdef WOLF_VULKAN
	return CommandBufferVulkan::getDrawIndexedIndirectCommandStructureSize();
#else
	Debug::sendCriticalError("Not implemented");
	return -1;
#endif
}

uint32_t Wolf::CommandBuffer::getDrawMeshTasksIndirectCommandStructureSize()
{
#ifdef WOLF_VULKAN
	return CommandBufferVulkan::getDrawMeshTasksIndirectCommandStructureSize();
#else
	Debug::sendCriticalError("Not implemented");
	return -1;
#endif
}

uint32_t Wolf::CommandBuffer::getDispatchIndirectCommandStructureSize()
{
#ifdef WOLF_VULKAN
	return CommandBufferVulkan::getDispatchIndirectCommandStructureSize();
#else
	Debug::sendCriticalError("Not implemented");
	return -1;
#endif
}
