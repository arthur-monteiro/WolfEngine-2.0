#include "DepthPassBase.h"

#include "Debug.h"
#include "FrameBuffer.h"
#include "Image.h"
#include "Timer.h"
#include "Pipeline.h"
#include "RenderPass.h"

void Wolf::DepthPassBase::initializeResources(const InitializationContext& context)
{
	createDepthImage(context);

	std::vector<Attachment> attachments;
	getAttachments(context, attachments);

	m_renderPass.reset(RenderPass::createRenderPass(attachments));
	m_frameBuffer.reset(FrameBuffer::createFrameBuffer(*m_renderPass, attachments));
}

void Wolf::DepthPassBase::resize(const InitializationContext& context)
{
	m_renderPass->setExtent({ getWidth(), getHeight() });

	createDepthImage(context);

	std::vector<Attachment> attachments;
	getAttachments(context, attachments);

	m_frameBuffer.reset(FrameBuffer::createFrameBuffer(*m_renderPass, attachments));
}

void Wolf::DepthPassBase::record(const RecordContext& context)
{
	const CommandBuffer& commandBuffer = getCommandBuffer(context);

	std::vector<VkClearValue> clearValues(1);
	clearValues[0] = {{{1.0f}}};
	commandBuffer.beginRenderPass(*m_renderPass, *m_frameBuffer, clearValues);

	recordDraws(context);

	commandBuffer.endRenderPass();
}

void Wolf::DepthPassBase::getAttachments(const InitializationContext& context, std::vector<Attachment>& attachments)
{
	Attachment depth({ getWidth(), getHeight() }, context.depthFormat, VK_SAMPLE_COUNT_1_BIT, getFinalLayout(), VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, m_depthImage->getDefaultImageView());
	attachments = { depth };
}

void Wolf::DepthPassBase::createDepthImage(const InitializationContext& context)
{
	CreateImageInfo depthImageCreateInfo;
	depthImageCreateInfo.format = context.depthFormat;
	depthImageCreateInfo.extent.width = getWidth();
	depthImageCreateInfo.extent.height = getHeight();
	depthImageCreateInfo.extent.depth = 1;
	depthImageCreateInfo.mipLevelCount = 1;
	depthImageCreateInfo.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | getAdditionalUsages();
	m_depthImage.reset(Image::createImage(depthImageCreateInfo));
}