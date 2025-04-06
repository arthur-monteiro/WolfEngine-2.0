#pragma once

#include <vulkan/vulkan_core.h>

#include "../../Public/SwapChain.h"

struct GLFWwindow;

namespace Wolf
{
	class Semaphore;

	class SwapChainVulkan : public SwapChain
	{
	public:
		SwapChainVulkan(Extent2D extent);
		~SwapChainVulkan() override;
		
		uint32_t getCurrentImage(uint32_t currentFrameGPU) const override;
		void present(const Semaphore* waitSemaphore, uint32_t imageIndex) const override;

		void resetAllFences() override;
		void recreate(Extent2D extent) override;
	
	private:
		void initialize(VkExtent2D extent);

		VkSwapchainKHR m_swapChain;
	};
}
