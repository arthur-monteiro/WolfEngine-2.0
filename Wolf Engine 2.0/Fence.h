#pragma once

#include <vulkan/vulkan_core.h>

namespace Wolf
{
	class Fence
	{
	public:
		Fence(VkFenceCreateFlags createFlags);
		Fence(const Fence&) = delete;
		~Fence();

		void waitForFence() const;
		void resetFence() const;

		[[nodiscard]] VkFence getFence() const { return m_fence; }

	private:
		VkFence m_fence;
	};
}
