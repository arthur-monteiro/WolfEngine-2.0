#include "RenderPass.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/RenderPassVulkan.h"
#endif

Wolf::RenderPass* Wolf::RenderPass::createRenderPass(const std::vector<Attachment>& attachments)
{
#ifdef WOLF_VULKAN
	return new RenderPassVulkan(attachments);
#else
	return nullptr;
#endif
}

void Wolf::RenderPass::setExtent(const Extent2D& extent)
{
	m_extent = extent;
	for (const auto& callback : m_callbackWhenExtentChanged)
	{
		callback(this);
	}
}
