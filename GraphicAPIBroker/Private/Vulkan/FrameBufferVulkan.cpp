#include "FrameBufferVulkan.h"

#include <Debug.h>

#include "RenderPassVulkan.h"
#include "Vulkan.h"

Wolf::FrameBufferVulkan::FrameBufferVulkan(const RenderPass& renderPass, const std::vector<Attachment>& attachments)
{
	std::vector<VkImageView> imageViewAttachments(attachments.size());
	for (uint32_t i = 0; i < attachments.size(); ++i)
	{
		imageViewAttachments[i] = static_cast<VkImageView>(attachments[i].imageView);
	}

	VkFramebufferCreateInfo frameBufferInfo = {};
	frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferInfo.renderPass = static_cast<const RenderPassVulkan*>(&renderPass)->getRenderPass();
	frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	frameBufferInfo.pAttachments = imageViewAttachments.data();
	frameBufferInfo.width = attachments[0].extent.width;
	frameBufferInfo.height = attachments[0].extent.height;
	frameBufferInfo.layers = 1;

	if (vkCreateFramebuffer(g_vulkanInstance->getDevice(), &frameBufferInfo, nullptr, &m_frameBuffer) != VK_SUCCESS)
		Debug::sendError("Failed to create frame buffer");
}

Wolf::FrameBufferVulkan::~FrameBufferVulkan()
{
	vkDestroyFramebuffer(g_vulkanInstance->getDevice(), m_frameBuffer, nullptr);
}
