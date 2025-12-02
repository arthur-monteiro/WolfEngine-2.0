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
		SwapChainVulkan(const SwapChainCreateInfo& swapChainCreateInfo);
		~SwapChainVulkan() override;
		
		uint32_t acquireNextImage(uint32_t currentFrameGPU) const override;
		void present(const Semaphore* waitSemaphore, uint32_t imageIndex) const override;

		void resetAllFences() override;
		void recreate(Extent2D extent) override;
	
	private:
		void initialize(const SwapChainCreateInfo& swapChainCreateInfo);
		VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, Wolf::SwapChain::SwapChainCreateInfo::ColorSpace colorSpace, Format format);

		VkSwapchainKHR m_swapChain;
		Format m_format;
	};
}
