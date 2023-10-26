#include "UniquePass.h"

#include <filesystem>
#include <fstream>
#include <stb_image_write.h>

#include <Attachment.h>
#include <Image.h>
#include <FrameBuffer.h>

#include "Vertex2D.h"

using namespace Wolf;

void UniquePass::initializeResources(const InitializationContext& context)
{
	const VkFormat outputFormat = context.swapChainFormat;

	createDepthImage(context);

	Attachment depth({ context.swapChainWidth, context.swapChainHeight }, context.depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		m_depthImage->getDefaultImageView());
	Attachment color({ context.swapChainWidth, context.swapChainHeight }, outputFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, nullptr);

	m_renderPass.reset(new RenderPass({ depth, color }));

	m_commandBuffer.reset(new CommandBuffer(QueueType::GRAPHIC, false /* isTransient */));
	
	m_frameBuffers.resize(context.swapChainImageCount);
	for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
	{
		color.imageView = context.swapChainImages[i]->getDefaultImageView();
		m_frameBuffers[i].reset(new Framebuffer(m_renderPass->getRenderPass(), { depth, color }));
	}

	m_semaphore.reset(new Semaphore(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT));

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

	m_vertexBuffer.reset(new Buffer(sizeof(Vertex2D) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, UpdateRate::NEVER));
	m_vertexBuffer->transferCPUMemoryWithStagingBuffer(vertices.data(), sizeof(Vertex2D) * vertices.size());

	m_indexBuffer.reset(new Buffer(sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, UpdateRate::NEVER));
	m_indexBuffer->transferCPUMemoryWithStagingBuffer(indices.data(), sizeof(uint16_t) * indices.size());
}

void UniquePass::resize(const InitializationContext& context)
{
	createDepthImage(context);
	m_renderPass->setExtent({ context.swapChainWidth, context.swapChainHeight });

	m_frameBuffers.clear();
	m_frameBuffers.resize(context.swapChainImageCount);

	Attachment depth({ context.swapChainWidth, context.swapChainHeight }, context.depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		m_depthImage->getDefaultImageView());
	Attachment color({ context.swapChainWidth, context.swapChainHeight }, context.swapChainFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, nullptr);
	for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
	{
		color.imageView = context.swapChainImages[i]->getDefaultImageView();
		m_frameBuffers[i].reset(new Framebuffer(m_renderPass->getRenderPass(), { depth, color }));
	}

	createPipeline(context.swapChainWidth, context.swapChainHeight);
}

void UniquePass::record(const RecordContext& context)
{
	const uint32_t frameBufferIdx = context.swapChainImageIdx;

	m_commandBuffer->beginCommandBuffer(context.commandBufferIdx);

	std::vector<VkClearValue> clearValues(2);
	clearValues[0] = {{{1.0f}}};
	clearValues[1] = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
	m_renderPass->beginRenderPass(m_frameBuffers[frameBufferIdx]->getFramebuffer(), clearValues, m_commandBuffer->getCommandBuffer(context.commandBufferIdx));

	vkCmdBindPipeline(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->getPipeline());

	constexpr VkDeviceSize offsets[1] = { 0 };
	const VkBuffer vertexBuffer = m_vertexBuffer->getBuffer();
	vkCmdBindVertexBuffers(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), 0, 1, &vertexBuffer, offsets);
	vkCmdBindIndexBuffer(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);

	vkCmdDrawIndexed(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), 3, 1, 0, 0, 0);

	m_renderPass->endRenderPass(m_commandBuffer->getCommandBuffer(context.commandBufferIdx));

	m_commandBuffer->endCommandBuffer(context.commandBufferIdx);
}

void UniquePass::submit(const SubmitContext& context)
{
	const std::vector waitSemaphores{ context.swapChainImageAvailableSemaphore };
	const std::vector signalSemaphores{ m_semaphore->getSemaphore() };
	m_commandBuffer->submit(context.commandBufferIdx, waitSemaphores, signalSemaphores, context.frameFence);

	bool anyShaderModified = m_vertexShaderParser->compileIfFileHasBeenModified();
	if (m_fragmentShaderParser->compileIfFileHasBeenModified())
		anyShaderModified = true;

	if (anyShaderModified)
	{
		vkDeviceWaitIdle(context.device);
		createPipeline(m_swapChainWidth, m_swapChainHeight);
	}
}

void UniquePass::createDepthImage(const InitializationContext& context)
{
	CreateImageInfo depthImageCreateInfo;
	depthImageCreateInfo.format = context.depthFormat;
	depthImageCreateInfo.extent.width = context.swapChainWidth;
	depthImageCreateInfo.extent.height = context.swapChainHeight;
	depthImageCreateInfo.extent.depth = 1;
	depthImageCreateInfo.mipLevelCount = 1;
	depthImageCreateInfo.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	m_depthImage.reset(new Image(depthImageCreateInfo));
}

void UniquePass::createPipeline(uint32_t width, uint32_t height)
{
	RenderingPipelineCreateInfo pipelineCreateInfo;
	pipelineCreateInfo.renderPass = m_renderPass->getRenderPass();

	// Programming stages
	std::vector<char> vertexShaderCode;
	m_vertexShaderParser->readCompiledShader(vertexShaderCode);
	std::vector<char> fragmentShaderCode;
	m_fragmentShaderParser->readCompiledShader(fragmentShaderCode);

	std::vector<ShaderCreateInfo> shaders(2);
	shaders[0].shaderCode = vertexShaderCode;
	shaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaders[1].shaderCode = fragmentShaderCode;
	shaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

	pipelineCreateInfo.shaderCreateInfos = shaders;

	// IA
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	Vertex2D::getAttributeDescriptions(attributeDescriptions, 0);
	pipelineCreateInfo.vertexInputAttributeDescriptions = attributeDescriptions;

	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0] = {};
	Vertex2D::getBindingDescription(bindingDescriptions[0], 0);
	pipelineCreateInfo.vertexInputBindingDescriptions = bindingDescriptions;

	// Viewport
	pipelineCreateInfo.extent = { width, height };

	// Color Blend
	std::vector blendModes = { RenderingPipelineCreateInfo::BLEND_MODE::OPAQUE };
	pipelineCreateInfo.blendModes = blendModes;

	m_pipeline.reset(new Pipeline(pipelineCreateInfo));

	m_swapChainWidth = width;
	m_swapChainHeight = height;
}