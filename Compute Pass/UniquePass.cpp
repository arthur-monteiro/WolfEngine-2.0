#include "UniquePass.h"

#include <filesystem>
#include <fstream>

#include <DescriptorSetGenerator.h>
#include <DescriptorSetLayoutGenerator.h>
#include <Image.h>
#include <ImageFileLoader.h>

using namespace Wolf;

void UniquePass::initializeResources(const InitializationContext& context)
{
	m_commandBuffer.reset(new CommandBuffer(QueueType::COMPUTE, false /* isTransient */));
	m_semaphore.reset(new Semaphore(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT));

	m_computeShaderParser.reset(new ShaderParser("Shaders/shader.comp"));

	m_descriptorSetLayoutGenerator.addStorageImage(VK_SHADER_STAGE_COMPUTE_BIT, 0);
	m_descriptorSetLayout.reset(new DescriptorSetLayout(m_descriptorSetLayoutGenerator.getDescriptorLayouts()));

	std::vector<Image*> outputImages;
	buildOutputImages(context, outputImages);

	m_descriptorSets.resize(outputImages.size());
	createDescriptorSets(outputImages);

	createPipeline(context.swapChainWidth, context.swapChainHeight);
}

void UniquePass::resize(const InitializationContext& context)
{
	std::vector<Image*> outputImages;
	buildOutputImages(context, outputImages);
	createDescriptorSets(outputImages);

	m_swapChainWidth = context.swapChainWidth;
	m_swapChainHeight = context.swapChainHeight;
}

void UniquePass::record(const RecordContext& context)
{
	/* Command buffer record */
	m_commandBuffer->beginCommandBuffer(context.commandBufferIdx);

	const uint32_t outputImageIdx = context.swapChainImageIdx;

	context.swapchainImage->transitionImageLayout(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), { VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT });

	vkCmdBindDescriptorSets(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->getPipelineLayout(), 0, 1, m_descriptorSets[outputImageIdx]->getDescriptorSet(), 0, nullptr);

	vkCmdBindPipeline(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->getPipeline());

	const VkExtent3D dispatchGroups = { 16, 16, 1 };
	const uint32_t groupSizeX = m_swapChainWidth % dispatchGroups.width != 0 ? m_swapChainWidth / dispatchGroups.width + 1 : m_swapChainWidth / dispatchGroups.width;
	const uint32_t groupSizeY = m_swapChainHeight % dispatchGroups.height != 0 ? m_swapChainHeight / dispatchGroups.height + 1 : m_swapChainHeight / dispatchGroups.height;
	vkCmdDispatch(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), groupSizeX, groupSizeY, dispatchGroups.depth);

	context.swapchainImage->transitionImageLayout(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });

	m_commandBuffer->endCommandBuffer(context.commandBufferIdx);
}

void UniquePass::submit(const SubmitContext& context)
{
	const std::vector waitSemaphores{ context.swapChainImageAvailableSemaphore };
	const std::vector signalSemaphores{ m_semaphore->getSemaphore() };
	m_commandBuffer->submit(context.commandBufferIdx, waitSemaphores, signalSemaphores, context.frameFence);

	if (m_computeShaderParser->compileIfFileHasBeenModified())
	{
		vkDeviceWaitIdle(context.device);
		createPipeline(m_swapChainWidth, m_swapChainHeight);
	}
}

void UniquePass::buildOutputImages(const InitializationContext& context, std::vector<Wolf::Image*>& outputImages) const
{
	outputImages.resize(context.swapChainImageCount);
	for (uint32_t i = 0; i < outputImages.size(); ++i)
	{
		outputImages[i] = context.swapChainImages[i];
	}
}

void UniquePass::createPipeline(uint32_t width, uint32_t height)
{
	// Compute shader parser
	std::vector<char> computeShaderCode;
	m_computeShaderParser->readCompiledShader(computeShaderCode);

	ShaderCreateInfo computeShaderCreateInfo;
	computeShaderCreateInfo.shaderCode = computeShaderCode;
	computeShaderCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(1);
	descriptorSetLayouts[0] = m_descriptorSetLayout->getDescriptorSetLayout();
	m_pipeline.reset(new Pipeline(computeShaderCreateInfo, descriptorSetLayouts));

	m_swapChainWidth = width;
	m_swapChainHeight = height;
}

void UniquePass::createDescriptorSets(const std::vector<Image*>& outputImages)
{
	for (uint32_t i = 0; i < outputImages.size(); ++i)
	{
		DescriptorSetGenerator descriptorSetGenerator(m_descriptorSetLayoutGenerator.getDescriptorLayouts());
		DescriptorSetGenerator::ImageDescription image(VK_IMAGE_LAYOUT_GENERAL, outputImages[i]->getDefaultImageView());
		descriptorSetGenerator.setImage(0, image);

		if(!m_descriptorSets[i])
			m_descriptorSets[i].reset(new DescriptorSet(m_descriptorSetLayout->getDescriptorSetLayout(), UpdateRate::NEVER));
		m_descriptorSets[i]->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
	}
}
