#include "CommandBuffer.h"

#include "Configuration.h"
#include "Debug.h"

Wolf::CommandBuffer::CommandBuffer(QueueType queueType, bool oneTimeSubmit)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

	VkCommandPool commandPool = VK_NULL_HANDLE;
	if (queueType == QueueType::GRAPHIC || queueType == QueueType::TRANSFER)
	{
		if (oneTimeSubmit)
			commandPool = g_vulkanInstance->getGraphicsTransientCommandPool()->getCommandPool();
		else
			commandPool = g_vulkanInstance->getGraphicsCommandPool()->getCommandPool();
	}
	else if (queueType == QueueType::COMPUTE || queueType == QueueType::RAY_TRACING)
	{
		if (oneTimeSubmit)
			commandPool = g_vulkanInstance->getComputeTransientCommandPool()->getCommandPool();
		else
			commandPool = g_vulkanInstance->getComputeCommandPool()->getCommandPool();
	}

	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	uint32_t commandBufferCount = oneTimeSubmit ? 1 : g_configuration->getMaxCachedFrames();;
	allocInfo.commandBufferCount = commandBufferCount;

	m_commandBuffers.resize(commandBufferCount);
	if (vkAllocateCommandBuffers(g_vulkanInstance->getDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
		Debug::sendError("Error : command buffer allocation");

	m_usedCommandPool = commandPool;
	m_queueType = queueType;
	m_oneTimeSubmit = oneTimeSubmit;
}

Wolf::CommandBuffer::~CommandBuffer()
{
	vkFreeCommandBuffers(g_vulkanInstance->getDevice(), m_usedCommandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
}

void Wolf::CommandBuffer::beginCommandBuffer(uint32_t index)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = m_oneTimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(m_commandBuffers[index], &beginInfo);
}

void Wolf::CommandBuffer::endCommandBuffer(uint32_t index)
{
	if (vkEndCommandBuffer(m_commandBuffers[index]) != VK_SUCCESS)
		Debug::sendError("Error : end command buffer");
}

void Wolf::CommandBuffer::submit(uint32_t index, const std::vector<const Wolf::Semaphore*>& waitSemaphores, const std::vector<VkSemaphore>& signalSemaphores, VkFence fence)
{
	VkQueue queue;
	if (m_queueType == QueueType::GRAPHIC || m_queueType == QueueType::TRANSFER)
		queue = g_vulkanInstance->getGraphicsQueue();
	else
		queue = g_vulkanInstance->getComputeQueue();

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	VkCommandBuffer commandBuffer = m_commandBuffers[index];
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.commandBufferCount = 1;

	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
	std::vector<VkSemaphore> semaphores;
	std::vector<VkPipelineStageFlags> stages;
	for (int i(0); i < waitSemaphores.size(); ++i)
	{
		semaphores.push_back(waitSemaphores[i]->getSemaphore());
		stages.push_back(waitSemaphores[i]->getPipelineStage());
	}
	submitInfo.pWaitSemaphores = semaphores.data();
	submitInfo.pWaitDstStageMask = stages.data();

	if (VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence); result != VK_SUCCESS)
	{
		Debug::sendCriticalError("Error : submit to graphics queue");
	}
}
