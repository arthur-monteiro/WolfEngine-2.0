#include "CommandPool.h"

#include <Debug.h>

#include "Vulkan.h"

Wolf::CommandPool::CommandPool(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlagBits commandPoolType)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndex;
	poolInfo.flags = commandPoolType;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
		Debug::sendCriticalError("Error : create command pool");
}

Wolf::CommandPool::~CommandPool()
{
	vkDestroyCommandPool(g_vulkanInstance->getDevice(), m_commandPool, nullptr);
}
