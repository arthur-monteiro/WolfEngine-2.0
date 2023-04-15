#pragma once

#include "Semaphore.h"
#include "Vulkan.h"

namespace Wolf
{
	enum class QueueType { GRAPHIC, COMPUTE, TRANSFER, RAY_TRACING };

	class CommandBuffer
	{
	public:
		CommandBuffer(QueueType queueType, bool isTransient);
		~CommandBuffer();

		void beginCommandBuffer(uint32_t index) const;
		void endCommandBuffer(uint32_t index) const;
		void submit(uint32_t index, const std::vector<const Semaphore*>& waitSemaphores, const std::vector<VkSemaphore>& signalSemaphores, VkFence fence) const;

		// Getter
	public:
		[[nodiscard]] VkCommandBuffer getCommandBuffer(uint32_t index) const { return m_commandBuffers[index]; }

	private:
		std::vector<VkCommandBuffer> m_commandBuffers;
		VkCommandPool m_usedCommandPool;
		QueueType m_queueType;
		bool m_oneTimeSubmit;
	};
}
