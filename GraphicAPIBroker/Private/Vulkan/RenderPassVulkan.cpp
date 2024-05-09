#include "RenderPassVulkan.h"

#include <Debug.h>

#include "Vulkan.h"

Wolf::RenderPassVulkan::RenderPassVulkan(const std::vector<Attachment>& attachments)
{
	std::vector<VkAttachmentReference2> colorAttachmentRefs;
	VkAttachmentReference2 depthAttachmentRef; bool useDepthAttachment = false;
	std::vector<VkAttachmentReference2> resolveAttachmentRefs;
	VkAttachmentReference2 shadingRateAttachmentRef; bool useShadingRateAttachment = false;

	// Attachment descriptions
	std::vector<VkAttachmentDescription2> attachmentDescriptions;
	for (int i(0); i < attachments.size(); ++i)
	{
		attachmentDescriptions.resize(attachmentDescriptions.size() + 1);
		attachmentDescriptions[i].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
		attachmentDescriptions[i].format = attachments[i].format;
		attachmentDescriptions[i].samples = attachments[i].sampleCount;
		attachmentDescriptions[i].loadOp = attachments[i].loadOperation;
		attachmentDescriptions[i].storeOp = attachments[i].storeOperation;
		attachmentDescriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[i].initialLayout = attachments[i].initialLayout;
		attachmentDescriptions[i].finalLayout = attachments[i].finalLayout;
		attachmentDescriptions[i].pNext = nullptr;

		VkAttachmentReference2 ref{};
		ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
		ref.attachment = i;

		if ((attachments[i].usageType & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) && (attachments[i].usageType & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT))
		{
			ref.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			resolveAttachmentRefs.push_back(ref);
		}
		else if (attachments[i].usageType & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			ref.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachmentRefs.push_back(ref);
		}
		else if (attachments[i].usageType & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			ref.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachmentRef = ref;
			useDepthAttachment = true;
		}
		else if(attachments[i].usageType & VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)
		{
			ref.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ref.layout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
			useShadingRateAttachment = true;
			shadingRateAttachmentRef = ref;
		}
	}

	// Subpass
	VkSubpassDescription2 subpass = {};
	subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
	subpass.pColorAttachments = colorAttachmentRefs.data();
	if (useDepthAttachment)
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
	if (!resolveAttachmentRefs.empty())
		subpass.pResolveAttachments = resolveAttachmentRefs.data();

	VkFragmentShadingRateAttachmentInfoKHR shadingRateAttachmentInfo{};
	if (useShadingRateAttachment)
	{
		shadingRateAttachmentInfo.sType = VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
		shadingRateAttachmentInfo.pFragmentShadingRateAttachment = &shadingRateAttachmentRef;
		shadingRateAttachmentInfo.shadingRateAttachmentTexelSize = { 16, 16 };
		subpass.pNext = &shadingRateAttachmentInfo;
	}
	else
	{
		subpass.pNext = nullptr;
	}

	// Dependencies
	const std::vector<VkSubpassDependency2> dependencies(0);

	VkRenderPassCreateInfo2 renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.pAttachments = attachmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();
	renderPassInfo.pNext = nullptr;

	if (vkCreateRenderPass2(g_vulkanInstance->getDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		Debug::sendError("Error : create render pass");

	m_extent = attachments[0].extent;
}

Wolf::RenderPassVulkan::~RenderPassVulkan()
{
	vkDestroyRenderPass(g_vulkanInstance->getDevice(), m_renderPass, nullptr);
}