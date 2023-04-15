#include "Fence.h"

#include "Debug.h"
#include "Vulkan.h"

Wolf::Fence::Fence(VkFenceCreateFlags createFlags)
{
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = createFlags;
	if (vkCreateFence(g_vulkanInstance->getDevice(), &fenceInfo, nullptr, &m_fence) != VK_SUCCESS)
		Debug::sendCriticalError("Error creating fence");
}

Wolf::Fence::~Fence()
{
	vkDestroyFence(g_vulkanInstance->getDevice(), m_fence, nullptr);
}

void Wolf::Fence::waitForFence() const
{
	vkWaitForFences(g_vulkanInstance->getDevice(), 1, &m_fence, VK_TRUE, UINT64_MAX);
}

void Wolf::Fence::resetFence() const
{
	vkResetFences(g_vulkanInstance->getDevice(), 1, &m_fence);
}
