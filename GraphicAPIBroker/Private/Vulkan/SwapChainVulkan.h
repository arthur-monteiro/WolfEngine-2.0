#pragma once

#include "../../Public/SwapChain.h"


struct GLFWwindow;

namespace Wolf
{
	class Semaphore;

	class SwapChainVulkan : public SwapChain
	{
	public:
		SwapChainVulkan(VkExtent2D extent);
		~SwapChainVulkan() override;
		
		uint32_t getCurrentImage(uint32_t currentFrameGPU) const override;
		void present(const Semaphore* waitSemaphore, uint32_t imageIndex) const override;

		void recreate(VkExtent2D extent) override;
	
	private:
		void initialize(VkExtent2D extent);

		VkSwapchainKHR m_swapChain;
	};
}
