#pragma once

#include <vulkan/vulkan_core.h>

#include "../../Public/Fence.h"

namespace Wolf
{
	class FenceVulkan : public Fence
	{
	public:
		FenceVulkan(VkFenceCreateFlags createFlags);
		FenceVulkan(const FenceVulkan&) = delete;
		~FenceVulkan() override;

		void waitForFence() const override;
		void resetFence() const override;

		[[nodiscard]] VkFence getFence() const { return m_fence; }

	private:
		VkFence m_fence;
	};
}
