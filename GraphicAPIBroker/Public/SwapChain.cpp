#include "SwapChain.h"

#include "Fence.h"

#ifdef WOLF_USE_VULKAN
#include "../Private/Vulkan/SwapChainVulkan.h"
#endif

Wolf::SwapChain* Wolf::SwapChain::createSwapChain(VkExtent2D extent)
{
#ifdef WOLF_USE_VULKAN
	return new SwapChainVulkan(extent);
#else
	return nullptr;
#endif
}

void Wolf::SwapChain::synchroniseCPUFromGPU(uint32_t currentFrame) const
{
	m_frameFences[currentFrame % m_frameFences.size()]->waitForFence();
	m_frameFences[currentFrame % m_frameFences.size()]->resetFence();
}