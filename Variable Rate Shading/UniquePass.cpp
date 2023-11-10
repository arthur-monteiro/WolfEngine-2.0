#include "UniquePass.h"

#include <filesystem>
#include <fstream>

#include <Attachment.h>
#include <DescriptorSetGenerator.h>
#include <DescriptorSetLayoutGenerator.h>
#include <Image.h>
#include <ImageFileLoader.h>
#include <FrameBuffer.h>
#include <ShadingRatePalette.h>

#include "Vertex2DTextured.h"

using namespace Wolf;

void UniquePass::initializeResources(const InitializationContext& context)
{
	createDepthImage(context);

	Attachment depth({ context.swapChainWidth, context.swapChainHeight }, context.depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		m_depthImage->getDefaultImageView());
	Attachment color({ context.swapChainWidth, context.swapChainHeight }, context.swapChainFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, nullptr);

	VkExtent2D vrsImageExtent = { static_cast<uint32_t>(std::ceil(static_cast<float>(context.swapChainWidth) / 16.0f)),static_cast<uint32_t>(std::ceil(static_cast<float>(context.swapChainHeight) / 16.0f)) };

	CreateImageInfo shadingRateImageInfo{};
	shadingRateImageInfo.extent = { vrsImageExtent.width, vrsImageExtent.height, 1 };
	shadingRateImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	shadingRateImageInfo.usage = VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	shadingRateImageInfo.format = VK_FORMAT_R8_UINT;
	shadingRateImageInfo.mipLevelCount = 1;
	m_shadingRateImage.reset(new Image(shadingRateImageInfo));

	std::vector<uint8_t> shadingRatePixels(vrsImageExtent.width * vrsImageExtent.height);
	for (uint32_t y = 0; y < vrsImageExtent.height; y++)
	{
		for (uint32_t x = 0; x < vrsImageExtent.width; x++)
		{
			const float deltaX = static_cast<float>(vrsImageExtent.width) / 2.0f - static_cast<float>(x);
			const float deltaY = (static_cast<float>(vrsImageExtent.height) / 2.0f - static_cast<float>(y)) * (static_cast<float>(vrsImageExtent.width) / static_cast<float>(vrsImageExtent.height));
			const float dist = std::sqrt(deltaX * deltaX + deltaY * deltaY);

			if (dist < 16.0f)
				shadingRatePixels[x + y * vrsImageExtent.width] = SHADING_RATE_1X1;
			else if (dist < 64.0f)
				shadingRatePixels[x + y * vrsImageExtent.width] = SHADING_RATE_2X2;
			else
				shadingRatePixels[x + y * vrsImageExtent.width] = SHADING_RATE_4X4;
		}
	}

	m_shadingRateImage->copyCPUBuffer(shadingRatePixels.data(), { VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR, VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR,
		VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR, 0, 1 });

	Attachment vrsAttachment(vrsImageExtent, shadingRateImageInfo.format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR, VK_ATTACHMENT_STORE_OP_STORE, 
		VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR, m_shadingRateImage->getDefaultImageView());
	vrsAttachment.loadOperation = VK_ATTACHMENT_LOAD_OP_LOAD;
	vrsAttachment.initialLayout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

	m_renderPass.reset(new RenderPass({ depth, color, vrsAttachment }));

	m_commandBuffer.reset(new CommandBuffer(QueueType::GRAPHIC, false /* isTransient */));

	m_frameBuffers.resize(context.swapChainImageCount);
	for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
	{
		color.imageView = context.swapChainImages[i]->getDefaultImageView();
		m_frameBuffers[i].reset(new Framebuffer(m_renderPass->getRenderPass(), { depth, color, vrsAttachment }));
	}

	m_semaphore.reset(new Semaphore(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT));

	DescriptorSetLayoutGenerator descriptorSetLayoutGenerator;
	descriptorSetLayoutGenerator.addCombinedImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	m_descriptorSetLayout.reset(new DescriptorSetLayout(descriptorSetLayoutGenerator.getDescriptorLayouts()));

	ImageFileLoader imageFileLoader("Images/Vincent_Van_Gogh_-_Wheatfield_with_Crows.jpg");
	CreateImageInfo createImageInfo;
	createImageInfo.extent = { (uint32_t)imageFileLoader.getWidth(), (uint32_t)imageFileLoader.getHeight(), 1 };
	createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	createImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	createImageInfo.mipLevelCount = 1;
	createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	m_texture.reset(new Image(createImageInfo));
	m_texture->copyCPUBuffer(imageFileLoader.getPixels(), Image::SampledInFragmentShader());

	m_sampler.reset(new Sampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 1.0f, VK_FILTER_LINEAR));

	DescriptorSetGenerator descriptorSetGenerator(descriptorSetLayoutGenerator.getDescriptorLayouts());
	descriptorSetGenerator.setCombinedImageSampler(0, m_texture->getImageLayout(), m_texture->getDefaultImageView(), *m_sampler);

	m_descriptorSet.reset(new DescriptorSet(m_descriptorSetLayout->getDescriptorSetLayout(), UpdateRate::EACH_FRAME));
	m_descriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());

	m_vertexShaderParser.reset(new ShaderParser("Shaders/shader.vert"));
	m_fragmentShaderParser.reset(new ShaderParser("Shaders/shader.frag"));

	createPipeline(context.swapChainWidth, context.swapChainHeight);

	// Load triangle
	std::vector<Vertex2DTextured> vertices =
	{
		{ glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f) }, // top left
		{ glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 0.0f) }, // top right
		{ glm::vec2(-1.0f, 1.0f), glm::vec2(0.0f, 1.0f) }, // bot left
		{ glm::vec2(1.0f, 1.0f),glm::vec2(1.0f, 1.0f) } // bot right
	};

	std::vector<uint32_t> indices =
	{
		0, 2, 1,
		2, 3, 1
	};

	m_rectangle.reset(new Mesh(vertices, indices));
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
	/* Command buffer record */
	const uint32_t frameBufferIdx = context.swapChainImageIdx;

	m_commandBuffer->beginCommandBuffer(context.commandBufferIdx);

	std::vector<VkClearValue> clearValues(2);
	clearValues[0] = { {{1.0f}} };
	clearValues[1] = { {{0.1f, 0.1f, 0.1f, 1.0f}} };
	m_renderPass->beginRenderPass(m_frameBuffers[frameBufferIdx]->getFramebuffer(), clearValues, m_commandBuffer->getCommandBuffer(context.commandBufferIdx));

	vkCmdBindPipeline(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->getPipeline());

	vkCmdBindDescriptorSets(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->getPipelineLayout(), 0, 1, m_descriptorSet->getDescriptorSet(context.commandBufferIdx), 0, nullptr);

	VkExtent2D fragmentExtent{ 1, 1 };
	VkFragmentShadingRateCombinerOpKHR combiners[2];
	combiners[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
	combiners[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
	vkCmdSetFragmentShadingRateKHR(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), &fragmentExtent, combiners);

	m_rectangle->draw(m_commandBuffer->getCommandBuffer(context.commandBufferIdx));

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
	pipelineCreateInfo.shaderCreateInfos.resize(2);
	m_vertexShaderParser->readCompiledShader(pipelineCreateInfo.shaderCreateInfos[0].shaderCode);
	pipelineCreateInfo.shaderCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	m_fragmentShaderParser->readCompiledShader(pipelineCreateInfo.shaderCreateInfos[1].shaderCode);
	pipelineCreateInfo.shaderCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

	// IA
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	Vertex2DTextured::getAttributeDescriptions(attributeDescriptions, 0);
	pipelineCreateInfo.vertexInputAttributeDescriptions = attributeDescriptions;

	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0] = {};
	Vertex2DTextured::getBindingDescription(bindingDescriptions[0], 0);
	pipelineCreateInfo.vertexInputBindingDescriptions = bindingDescriptions;

	// Resources
	std::vector descriptorSetLayouts = { m_descriptorSetLayout->getDescriptorSetLayout() };
	pipelineCreateInfo.descriptorSetLayouts = descriptorSetLayouts;

	// Viewport
	pipelineCreateInfo.extent = { width, height };

	// Color Blend
	std::vector blendModes = { RenderingPipelineCreateInfo::BLEND_MODE::OPAQUE };
	pipelineCreateInfo.blendModes = blendModes;

	m_pipeline.reset(new Pipeline(pipelineCreateInfo));

	m_swapChainWidth = width;
	m_swapChainHeight = height;
}