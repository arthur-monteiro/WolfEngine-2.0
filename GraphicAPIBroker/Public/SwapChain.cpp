#include "SwapChain.h"

#include "Fence.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/SwapChainVulkan.h"
#endif

Wolf::SwapChain* Wolf::SwapChain::createSwapChain(const SwapChainCreateInfo& swapChainCreateInfo)
{
#ifdef WOLF_VULKAN
	return new SwapChainVulkan(swapChainCreateInfo);
#else
	return nullptr;
#endif
}

void Wolf::SwapChain::synchroniseCPUFromGPU(uint32_t currentFrame) const
{
	m_frameFences[currentFrame % m_frameFences.size()]->waitForFence();
	m_frameFences[currentFrame % m_frameFences.size()]->resetFence();
}