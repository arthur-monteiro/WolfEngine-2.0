#include "RenderPass.h"

#include "Debug.h"
#include "Vulkan.h"

Wolf::RenderPass::RenderPass(const std::vector<Attachment>& attachments)
{
	std::vector<VkAttachmentReference> colorAttachmentRefs;
	VkAttachmentReference depthAttachmentRef; bool useDepthAttachement = false;
	std::vector<VkAttachmentReference> resolveAttachmentRefs;

	// Attachment descriptions
	std::vector<VkAttachmentDescription> attachmentDescriptions(attachments.size());
	for (int i(0); i < attachments.size(); ++i)
	{
		attachmentDescriptions[i].format = attachments[i].format;
		attachmentDescriptions[i].samples = attachments[i].sampleCount;
		attachmentDescriptions[i].loadOp = attachments[i].loadOperation;
		attachmentDescriptions[i].storeOp = attachments[i].storeOperation;
		attachmentDescriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[i].finalLayout = attachments[i].finalLayout;

		VkAttachmentReference ref;
		ref.attachment = i;

		if ((attachments[i].usageType & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) && (attachments[i].usageType & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT))
		{
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			resolveAttachmentRefs.push_back(ref);
		}
		else if (attachments[i].usageType & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachmentRefs.push_back(ref);
		}
		else if (attachments[i].usageType & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachmentRef = ref;
			useDepthAttachement = true;
		}
	}

	// Subpass
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
	subpass.pColorAttachments = colorAttachmentRefs.data();
	if (useDepthAttachement)
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
	if (!resolveAttachmentRefs.empty())
		subpass.pResolveAttachments = resolveAttachmentRefs.data();

	// Dependencies
	std::vector<VkSubpassDependency> dependencies(!colorAttachmentRefs.empty() ? 1 : 2);
	if (!colorAttachmentRefs.empty())
	{
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = 0;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	else
	{
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	}

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.pAttachments = attachmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(g_vulkanInstance->getDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		Debug::sendError("Error : create render pass");

	m_extent = attachments[0].extent;

}

Wolf::RenderPass::~RenderPass()
{
	vkDestroyRenderPass(g_vulkanInstance->getDevice(), m_renderPass, nullptr);
}

void Wolf::RenderPass::beginRenderPass(VkFramebuffer frameBuffer, const std::vector<VkClearValue>& clearValues, VkCommandBuffer commandBuffer)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = frameBuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_extent;

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Wolf::RenderPass::endRenderPass(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}
