#include "RenderPassVulkan.h"

#include <Debug.h>

#include "../../Public/Image.h"
#include "FormatsVulkan.h"
#include "Vulkan.h"

Wolf::RenderPassVulkan::RenderPassVulkan(const std::vector<Attachment>& attachments)
{
#ifdef WOLF_VULKAN_1_2
	std::vector<VkAttachmentReference2> colorAttachmentRefs;
	VkAttachmentReference2 depthAttachmentRef; bool useDepthAttachment = false;
	std::vector<VkAttachmentReference2> resolveAttachmentRefs;
	VkAttachmentReference2 shadingRateAttachmentRef; bool useShadingRateAttachment = false;
#else
	std::vector<VkAttachmentReference> colorAttachmentRefs;
	VkAttachmentReference depthAttachmentRef; bool useDepthAttachment = false;
	std::vector<VkAttachmentReference> resolveAttachmentRefs;
#endif

	// Attachment descriptions
#ifdef WOLF_VULKAN_1_2
	std::vector<VkAttachmentDescription2> attachmentDescriptions;
#else
	std::vector<VkAttachmentDescription> attachmentDescriptions;
#endif
	
	for (int i(0); i < attachments.size(); ++i)
	{
		attachmentDescriptions.resize(attachmentDescriptions.size() + 1);
#ifdef WOLF_VULKAN_1_2
		attachmentDescriptions[i].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
#endif
		attachmentDescriptions[i].format = wolfFormatToVkFormat(attachments[i].format);
		attachmentDescriptions[i].samples = wolfSampleCountFlagBitsToVkSampleCountFlagBits(attachments[i].sampleCount);
		attachmentDescriptions[i].loadOp = wolfAttachmentLoadOpToVkAttachmentLoadOp(attachments[i].loadOperation);
		attachmentDescriptions[i].storeOp = wolfAttachmentStoreOpToVkAttachmentStoreOp(attachments[i].storeOperation);
		attachmentDescriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[i].initialLayout = attachments[i].initialLayout;
		attachmentDescriptions[i].finalLayout = attachments[i].finalLayout;
#ifdef WOLF_VULKAN_1_2
		attachmentDescriptions[i].pNext = nullptr;
#endif

#ifdef WOLF_VULKAN_1_2
		VkAttachmentReference2 ref{};
		ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
#else
		VkAttachmentReference ref{};
#endif
		
		ref.attachment = i;

		if ((attachments[i].usageType & ImageUsageFlagBits::COLOR_ATTACHMENT) && (attachments[i].usageType & ImageUsageFlagBits::TRANSIENT_ATTACHMENT))
		{
#ifdef WOLF_VULKAN_1_2
			ref.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
#endif
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			resolveAttachmentRefs.push_back(ref);
		}
		else if (attachments[i].usageType & ImageUsageFlagBits::COLOR_ATTACHMENT)
		{
#ifdef WOLF_VULKAN_1_2
			ref.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
#endif
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachmentRefs.push_back(ref);
		}
		else if (attachments[i].usageType & ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT)
		{
#ifdef WOLF_VULKAN_1_2
			ref.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
#endif
			ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachmentRef = ref;
			useDepthAttachment = true;
		}
#ifdef WOLF_VULKAN_1_2
		else if(attachments[i].usageType & ImageUsageFlagBits::FRAGMENT_SHADING_RATE_ATTACHMENT)
		{
			ref.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ref.layout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
			useShadingRateAttachment = true;
			shadingRateAttachmentRef = ref;
		}
#endif
	}

	// Subpass
#ifdef WOLF_VULKAN_1_2
	VkSubpassDescription2 subpass = {};
	subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
#else
	VkSubpassDescription subpass = {};
#endif

	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
	subpass.pColorAttachments = colorAttachmentRefs.data();
	if (useDepthAttachment)
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
	if (!resolveAttachmentRefs.empty())
		subpass.pResolveAttachments = resolveAttachmentRefs.data();

#ifdef WOLF_VULKAN_1_2
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
#endif

	// Dependencies
#ifdef WOLF_VULKAN_1_2
	const std::vector<VkSubpassDependency2> dependencies(0);
#else
	const std::vector<VkSubpassDependency> dependencies(0);
#endif
	

#ifdef WOLF_VULKAN_1_2
	VkRenderPassCreateInfo2 renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
#else
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
#endif
	
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
	renderPassInfo.pAttachments = attachmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();
	renderPassInfo.pNext = nullptr;

#ifdef WOLF_VULKAN_1_2
	if (vkCreateRenderPass2(g_vulkanInstance->getDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
#else
	if (vkCreateRenderPass(g_vulkanInstance->getDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
#endif
		Debug::sendError("Error : create render pass");

	m_extent = attachments[0].extent;
}

Wolf::RenderPassVulkan::~RenderPassVulkan()
{
	vkDestroyRenderPass(g_vulkanInstance->getDevice(), m_renderPass, nullptr);
}

VkAttachmentLoadOp Wolf::RenderPassVulkan::wolfAttachmentLoadOpToVkAttachmentLoadOp(AttachmentLoadOp attachmentLoadOp)
{
	switch (attachmentLoadOp)
	{
		case AttachmentLoadOp::LOAD:
			return VK_ATTACHMENT_LOAD_OP_LOAD;
		case AttachmentLoadOp::CLEAR:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;
		case AttachmentLoadOp::DONT_CARE:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		default:
			Debug::sendCriticalError("Unhandled attachment load op");
			return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
	}
}

VkAttachmentStoreOp Wolf::RenderPassVulkan::wolfAttachmentStoreOpToVkAttachmentStoreOp(AttachmentStoreOp attachmentStoreOp)
{
	switch (attachmentStoreOp) {
		case AttachmentStoreOp::STORE:
			return VK_ATTACHMENT_STORE_OP_STORE;
		case AttachmentStoreOp::DONT_CARE:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		case AttachmentStoreOp::NONE:
			return VK_ATTACHMENT_STORE_OP_NONE;
		default:
			Debug::sendCriticalError("Unhandled attachment store op");
			return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
	}
}
