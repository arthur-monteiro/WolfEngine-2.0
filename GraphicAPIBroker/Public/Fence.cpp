#include "Fence.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/FenceVulkan.h"
#endif

Wolf::Fence* Wolf::Fence::createFence(bool startSignaled)
{
#ifdef WOLF_VULKAN
	return new FenceVulkan(startSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0);
#else
	return nullptr;
#endif
}