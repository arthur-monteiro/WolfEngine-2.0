#include "UniquePass.h"

#include <filesystem>
#include <fstream>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <Attachment.h>
#include <Image.h>
#include <FrameBuffer.h>

#include "Vertex2D.h"

using namespace Wolf;

void UniquePass::initializeResources(const InitializationContext& context)
{
	const Format outputFormat = context.swapChainFormat;

	createDepthImage(context);

	Attachment depth({ context.swapChainWidth, context.swapChainHeight }, context.depthFormat, SAMPLE_COUNT_1, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, AttachmentStoreOp::DONT_CARE, Wolf::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT,
		m_depthImage->getDefaultImageView());
	Attachment color({ context.swapChainWidth, context.swapChainHeight }, outputFormat, SAMPLE_COUNT_1, ImageLayout::PRESENT_SRC_KHR, AttachmentStoreOp::STORE, Wolf::ImageUsageFlagBits::COLOR_ATTACHMENT, nullptr);

	m_renderPass.reset(RenderPass::createRenderPass({ depth, color }));

	m_commandBuffer.reset(CommandBuffer::createCommandBuffer(QueueType::GRAPHIC, false /* isTransient */));
	
	m_frameBuffers.resize(context.swapChainImageCount);
	for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
	{
		color.imageView = context.swapChainImages[i]->getDefaultImageView();
		m_frameBuffers[i].reset(FrameBuffer::createFrameBuffer(*m_renderPass, { depth, color }));
	}

	createSemaphores(context, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, true);

	m_vertexShaderParser.reset(new ShaderParser("Shaders/shader.vert"));
	m_fragmentShaderParser.reset(new ShaderParser("Shaders/shader.frag"));

	createPipeline(context.swapChainWidth, context.swapChainHeight);

	// Load triangle
	std::vector<Vertex2D> vertices =
	{
		{ glm::vec2(0.0f, -0.75f) }, // top
		{ glm::vec2(-0.75f, 0.75f) }, // bot left
		{ glm::vec2(0.75f, 0.75f) } // bot right
	};

	std::vector<uint16_t> indices =
	{
		0, 1, 2
	};

	m_vertexBuffer.reset(Buffer::createBuffer(sizeof(Vertex2D) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	m_vertexBuffer->transferCPUMemoryWithStagingBuffer(vertices.data(), sizeof(Vertex2D) * vertices.size());

	m_indexBuffer.reset(Buffer::createBuffer(sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	m_indexBuffer->transferCPUMemoryWithStagingBuffer(indices.data(), sizeof(uint16_t) * indices.size());
}

void UniquePass::resize(const InitializationContext& context)
{
	createDepthImage(context);
	m_renderPass->setExtent({ context.swapChainWidth, context.swapChainHeight });

	m_frameBuffers.clear();
	m_frameBuffers.resize(context.swapChainImageCount);

	Attachment depth({ context.swapChainWidth, context.swapChainHeight }, context.depthFormat, SAMPLE_COUNT_1, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, AttachmentStoreOp::DONT_CARE, Wolf::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT,
		m_depthImage->getDefaultImageView());
	Attachment color({ context.swapChainWidth, context.swapChainHeight }, context.swapChainFormat, SAMPLE_COUNT_1, ImageLayout::PRESENT_SRC_KHR, AttachmentStoreOp::STORE, Wolf::ImageUsageFlagBits::COLOR_ATTACHMENT, nullptr);
	for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
	{
		color.imageView = context.swapChainImages[i]->getDefaultImageView();
		m_frameBuffers[i].reset(FrameBuffer::createFrameBuffer(*m_renderPass, { depth, color }));
	}

	createPipeline(context.swapChainWidth, context.swapChainHeight);
}

void UniquePass::record(const RecordContext& context)
{
	const uint32_t frameBufferIdx = context.swapChainImageIdx;

	m_commandBuffer->beginCommandBuffer();

	std::vector<ClearValue> clearValues(2);
	clearValues[0] = {{{1.0f}}};
	clearValues[1] = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
	m_commandBuffer->beginRenderPass(*m_renderPass, *m_frameBuffers[frameBufferIdx], clearValues);

	m_commandBuffer->bindPipeline(m_pipeline.get());

	m_commandBuffer->bindVertexBuffer(*m_vertexBuffer);
	m_commandBuffer->bindIndexBuffer(*m_indexBuffer, IndexType::U16);

	m_commandBuffer->drawIndexed(3, 1, 0, 0, 0);

	m_commandBuffer->endRenderPass();

	m_commandBuffer->endCommandBuffer();

	m_lastSwapChainImage = context.swapchainImage;
}

void UniquePass::submit(const SubmitContext& context)
{
	const std::vector<const Semaphore*> waitSemaphores{ context.swapChainImageAvailableSemaphore };
	const std::vector<const Semaphore*> signalSemaphores{ getSemaphore(context.swapChainImageIndex) };
	m_commandBuffer->submit(waitSemaphores, signalSemaphores, context.frameFence);

	bool anyShaderModified = m_vertexShaderParser->compileIfFileHasBeenModified();
	if (m_fragmentShaderParser->compileIfFileHasBeenModified())
		anyShaderModified = true;

	if (anyShaderModified)
	{
		context.graphicAPIManager->waitIdle();
		createPipeline(m_swapChainWidth, m_swapChainHeight);
	}

	if (m_screenshotRequested)
	{
		context.graphicAPIManager->waitIdle();

		std::vector<uint8_t> fullImageData;
		m_lastSwapChainImage->exportToBuffer(fullImageData);

		Wolf::Extent3D swapchainExtent = m_lastSwapChainImage->getExtent();

		uint32_t channelCount = 4;
		stbi_write_png("screenshot.png", swapchainExtent.width, swapchainExtent.height, channelCount, fullImageData.data(), swapchainExtent.width * channelCount);

		m_screenshotRequested = false;
	}
}

void UniquePass::requestScreenshot()
{
	m_screenshotRequested = true;
}

void UniquePass::createDepthImage(const InitializationContext& context)
{
	CreateImageInfo depthImageCreateInfo;
	depthImageCreateInfo.format = context.depthFormat;
	depthImageCreateInfo.extent.width = context.swapChainWidth;
	depthImageCreateInfo.extent.height = context.swapChainHeight;
	depthImageCreateInfo.extent.depth = 1;
	depthImageCreateInfo.mipLevelCount = 1;
	depthImageCreateInfo.aspectFlags = Wolf::ImageAspectFlagBits::DEPTH;
	depthImageCreateInfo.usage = Wolf::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT;
	m_depthImage.reset(Image::createImage(depthImageCreateInfo));
}

void UniquePass::createPipeline(uint32_t width, uint32_t height)
{
	RenderingPipelineCreateInfo pipelineCreateInfo;
	pipelineCreateInfo.renderPass = m_renderPass.get();

	// Programming stages
	pipelineCreateInfo.shaderCreateInfos.resize(2);
	m_vertexShaderParser->readCompiledShader(pipelineCreateInfo.shaderCreateInfos[0].shaderCode);
	pipelineCreateInfo.shaderCreateInfos[0].stage = VERTEX;
	m_fragmentShaderParser->readCompiledShader(pipelineCreateInfo.shaderCreateInfos[1].shaderCode);
	pipelineCreateInfo.shaderCreateInfos[1].stage = FRAGMENT;

	// IA
	std::vector<Wolf::VertexInputAttributeDescription> attributeDescriptions;
	Vertex2D::getAttributeDescriptions(attributeDescriptions, 0);
	pipelineCreateInfo.vertexInputAttributeDescriptions = attributeDescriptions;

	std::vector<Wolf::VertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0] = {};
	Vertex2D::getBindingDescription(bindingDescriptions[0], 0);
	pipelineCreateInfo.vertexInputBindingDescriptions = bindingDescriptions;

	// Viewport
	pipelineCreateInfo.extent = { width, height };

	// Color Blend
	std::vector blendModes = { RenderingPipelineCreateInfo::BLEND_MODE::OPAQUE };
	pipelineCreateInfo.blendModes = blendModes;

	m_pipeline.reset(Pipeline::createRenderingPipeline(pipelineCreateInfo));

	m_swapChainWidth = width;
	m_swapChainHeight = height;
}