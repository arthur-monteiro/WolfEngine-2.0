#include "FenceVulkan.h"

#include <Debug.h>

#include "Vulkan.h"

Wolf::FenceVulkan::FenceVulkan(VkFenceCreateFlags createFlags)
{
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = createFlags;
	if (vkCreateFence(g_vulkanInstance->getDevice(), &fenceInfo, nullptr, &m_fence) != VK_SUCCESS)
		Debug::sendCriticalError("Error creating fence");
}

Wolf::FenceVulkan::~FenceVulkan()
{
	vkDestroyFence(g_vulkanInstance->getDevice(), m_fence, nullptr);
}

void Wolf::FenceVulkan::waitForFence() const
{
	vkWaitForFences(g_vulkanInstance->getDevice(), 1, &m_fence, VK_TRUE, UINT64_MAX);
}

void Wolf::FenceVulkan::resetFence() const
{
	vkResetFences(g_vulkanInstance->getDevice(), 1, &m_fence);
}
