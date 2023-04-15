#include "FrameBuffer.h"

#include "Debug.h"

Wolf::Framebuffer::Framebuffer(VkRenderPass renderPass, const std::vector<Attachment>& attachments)
{
	std::vector<VkImageView> imageViewAttachments(attachments.size());
	for (uint32_t i = 0; i < attachments.size(); ++i)
	{
		imageViewAttachments[i] = attachments[i].imageView;
	}

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = imageViewAttachments.data();
	framebufferInfo.width = attachments[0].extent.width;
	framebufferInfo.height = attachments[0].extent.height;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(g_vulkanInstance->getDevice(), &framebufferInfo, nullptr, &m_framebuffer) != VK_SUCCESS)
		Debug::sendError("Failed to create framebuffer");
}

Wolf::Framebuffer::~Framebuffer()
{
	vkDestroyFramebuffer(g_vulkanInstance->getDevice(), m_framebuffer, nullptr);
}
