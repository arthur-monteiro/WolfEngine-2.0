#include "UniquePass.h"

#include <filesystem>
#include <fstream>

#include <Attachment.h>
#include <Image.h>
#include <FrameBuffer.h>

#include "RenderMeshList.h"
#include "Vertex2D.h"
#include "Vertex2DTextured.h"

using namespace Wolf;

void UniquePass::initializeResources(const InitializationContext& context)
{
	createDepthImage(context);

	Attachment depth({ context.swapChainWidth, context.swapChainHeight }, context.depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		m_depthImage->getDefaultImageView());
	Attachment color({ context.swapChainWidth, context.swapChainHeight }, context.swapChainFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, nullptr);

	m_renderPass.reset(new RenderPass({ depth, color }));

	m_commandBuffer.reset(new CommandBuffer(QueueType::GRAPHIC, false /* isTransient */));

	m_frameBuffers.resize(context.swapChainImageCount);
	for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
	{
		color.imageView = context.swapChainImages[i]->getDefaultImageView();
		m_frameBuffers[i].reset(new Framebuffer(m_renderPass->getRenderPass(), { depth, color }));
	}

	m_semaphore.reset(new Semaphore(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT));

	m_triangleVertexShaderParser.reset(new ShaderParser("Shaders/shader.vert"));
	m_triangleFragmentShaderParser.reset(new ShaderParser("Shaders/shader.frag"));

	m_userInterfaceVertexShaderParser.reset(new ShaderParser("Shaders/UI.vert"));
	m_userInterfaceFragmentShaderParser.reset(new ShaderParser("Shaders/UI.frag"));

	// Triangle resources
	{
		m_triangleUniformBuffer.reset(new Buffer(sizeof(glm::vec3), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, UpdateRate::NEVER));
		glm::vec3 defaultColor(0.0f, 1.0f, 0.0f);
		m_triangleUniformBuffer->transferCPUMemory(&defaultColor, sizeof(glm::vec3));

		DescriptorSetLayoutGenerator triangleDescriptorSetLayoutGenerator;
		triangleDescriptorSetLayoutGenerator.addUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, 0);
		m_triangleDescriptorSetLayout.reset(new DescriptorSetLayout(triangleDescriptorSetLayoutGenerator.getDescriptorLayouts()));

		DescriptorSetGenerator descriptorSetGenerator(triangleDescriptorSetLayoutGenerator.getDescriptorLayouts());
		descriptorSetGenerator.setBuffer(0, *m_triangleUniformBuffer);

		m_triangleDescriptorSet.reset(new DescriptorSet(m_triangleDescriptorSetLayout->getDescriptorSetLayout(), UpdateRate::NEVER));
		m_triangleDescriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
	}

	// UI resources
	{
		m_sampler.reset(new Sampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 1.0f, VK_FILTER_LINEAR));
		m_userInterfaceDescriptorSetLayoutGenerator.addCombinedImageSampler(VK_SHADER_STAGE_FRAGMENT_BIT, 0);
		m_userInterfaceDescriptorSetLayout.reset(new DescriptorSetLayout(m_userInterfaceDescriptorSetLayoutGenerator.getDescriptorLayouts()));

		DescriptorSetGenerator descriptorSetGenerator(m_userInterfaceDescriptorSetLayoutGenerator.getDescriptorLayouts());
		descriptorSetGenerator.setCombinedImageSampler(0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, context.userInterfaceImage->getDefaultImageView(), *
		                                               m_sampler);

		m_userInterfaceDescriptorSet.reset(new DescriptorSet(m_userInterfaceDescriptorSetLayout->getDescriptorSetLayout(), UpdateRate::NEVER));
		m_userInterfaceDescriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
	}

	createPipelines(context.swapChainWidth, context.swapChainHeight);

	// Load triangle
	{
		std::vector<Vertex2D> vertices =
		{
			{ glm::vec2(0.0f, -0.75f) }, // top
			{ glm::vec2(-0.75f, 0.75f) }, // bot left
			{ glm::vec2(0.75f, 0.75f) } // bot right
		};

		std::vector<uint32_t> indices =
		{
			0, 1, 2
		};

		m_triangle.reset(new Mesh(vertices, indices));
	}

	// Load fullscreen rectangle
	{
		std::vector<Vertex2DTextured> vertices =
		{
			{ glm::vec2(-1.0f, 1.0f), glm::vec2(0.0f, 1.0f) }, // top left
			{ glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 1.0f) }, // top right
			{ glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f) }, // bot left
			{ glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 0.0f) }, // bot right
		};

		std::vector<uint32_t> indices =
		{
			0, 1, 2,
			2, 1, 3
		};

		m_fullScreenRectangle.reset(new Mesh(vertices, indices));
	}
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

	createPipelines(context.swapChainWidth, context.swapChainHeight);

	DescriptorSetGenerator descriptorSetGenerator(m_userInterfaceDescriptorSetLayoutGenerator.getDescriptorLayouts());
	descriptorSetGenerator.setCombinedImageSampler(0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, context.userInterfaceImage->getDefaultImageView(), *
	                                               m_sampler);
	m_userInterfaceDescriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
}

void UniquePass::record(const RecordContext& context)
{
	const uint32_t frameBufferIdx = context.swapChainImageIdx;

	m_commandBuffer->beginCommandBuffer(context.commandBufferIdx);

	std::vector<VkClearValue> clearValues(2);
	clearValues[0] = {{{1.0f}}};
	clearValues[1] = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
	m_renderPass->beginRenderPass(m_frameBuffers[frameBufferIdx]->getFramebuffer(), clearValues, m_commandBuffer->getCommandBuffer(context.commandBufferIdx));

	/* Triangle */
	vkCmdBindPipeline(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_GRAPHICS, m_trianglePipeline->getPipeline());
	vkCmdBindDescriptorSets(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_GRAPHICS, m_trianglePipeline->getPipelineLayout(), 0, 1,
		m_triangleDescriptorSet->getDescriptorSet(), 0, nullptr);
	m_triangle->draw(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), RenderMeshList::NO_CAMERA_IDX);

	/* UI */
	vkCmdBindPipeline(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_GRAPHICS, m_userInterfacePipeline->getPipeline());
	vkCmdBindDescriptorSets(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_GRAPHICS, m_userInterfacePipeline->getPipelineLayout(), 0, 1, 
		m_userInterfaceDescriptorSet->getDescriptorSet(), 0, nullptr);
	m_fullScreenRectangle->draw(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), RenderMeshList::NO_CAMERA_IDX);

	m_renderPass->endRenderPass(m_commandBuffer->getCommandBuffer(context.commandBufferIdx));

	m_commandBuffer->endCommandBuffer(context.commandBufferIdx);
}

void UniquePass::submit(const SubmitContext& context)
{
	const std::vector waitSemaphores{ context.swapChainImageAvailableSemaphore, context.userInterfaceImageAvailableSemaphore };
	const std::vector signalSemaphores{ m_semaphore->getSemaphore() };
	m_commandBuffer->submit(context.commandBufferIdx, waitSemaphores, signalSemaphores, context.frameFence);

	bool anyShaderModified = m_triangleVertexShaderParser->compileIfFileHasBeenModified();
	if (m_triangleFragmentShaderParser->compileIfFileHasBeenModified())
		anyShaderModified = true;

	if (anyShaderModified)
	{
		vkDeviceWaitIdle(context.device);
		createPipelines(m_swapChainWidth, m_swapChainHeight);
	}
}

void UniquePass::setTriangleColor(const glm::vec3& color) const
{
	m_triangleUniformBuffer->transferCPUMemory(&color, sizeof(glm::vec3));
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

void UniquePass::createPipelines(uint32_t width, uint32_t height)
{
	// Triangle
	{
		RenderingPipelineCreateInfo pipelineCreateInfo;
		pipelineCreateInfo.renderPass = m_renderPass->getRenderPass();

		// Programming stages
		pipelineCreateInfo.shaderCreateInfos.resize(2);
		m_triangleVertexShaderParser->readCompiledShader(pipelineCreateInfo.shaderCreateInfos[0].shaderCode);
		pipelineCreateInfo.shaderCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		m_triangleFragmentShaderParser->readCompiledShader(pipelineCreateInfo.shaderCreateInfos[1].shaderCode);
		pipelineCreateInfo.shaderCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

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

		// Resources
		std::vector descriptorSetLayouts = { m_triangleDescriptorSetLayout->getDescriptorSetLayout() };
		pipelineCreateInfo.descriptorSetLayouts = descriptorSetLayouts;

		// Color Blend
		std::vector blendModes = { RenderingPipelineCreateInfo::BLEND_MODE::OPAQUE };
		pipelineCreateInfo.blendModes = blendModes;

		m_trianglePipeline.reset(new Pipeline(pipelineCreateInfo));
	}

	// UI
	{
		RenderingPipelineCreateInfo pipelineCreateInfo;
		pipelineCreateInfo.renderPass = m_renderPass->getRenderPass();

		// Programming stages
		pipelineCreateInfo.shaderCreateInfos.resize(2);
		m_userInterfaceVertexShaderParser->readCompiledShader(pipelineCreateInfo.shaderCreateInfos[0].shaderCode);
		pipelineCreateInfo.shaderCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		m_userInterfaceFragmentShaderParser->readCompiledShader(pipelineCreateInfo.shaderCreateInfos[1].shaderCode);
		pipelineCreateInfo.shaderCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		// IA
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		Vertex2DTextured::getAttributeDescriptions(attributeDescriptions, 0);
		pipelineCreateInfo.vertexInputAttributeDescriptions = attributeDescriptions;

		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0] = {};
		Vertex2DTextured::getBindingDescription(bindingDescriptions[0], 0);
		pipelineCreateInfo.vertexInputBindingDescriptions = bindingDescriptions;

		// Viewport
		pipelineCreateInfo.extent = { width, height };

		// Resources
		std::vector descriptorSetLayouts = { m_userInterfaceDescriptorSetLayout->getDescriptorSetLayout() };
		pipelineCreateInfo.descriptorSetLayouts = descriptorSetLayouts;

		// Color Blend
		std::vector blendModes = { RenderingPipelineCreateInfo::BLEND_MODE::TRANS_ALPHA };
		pipelineCreateInfo.blendModes = blendModes;

		m_userInterfacePipeline.reset(new Pipeline(pipelineCreateInfo));
	}

	m_swapChainWidth = width;
	m_swapChainHeight = height;
}