#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Wolf
{
	class CommandPool
	{
	public:
		CommandPool(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlagBits commandPoolType);
		~CommandPool();

		VkCommandPool getCommandPool() const { return m_commandPool; }

	private:
		VkCommandPool m_commandPool;
	};
}
