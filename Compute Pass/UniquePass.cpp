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
	m_commandBuffer.reset(CommandBuffer::createCommandBuffer(QueueType::COMPUTE, false /* isTransient */));
	m_semaphore.reset(Semaphore::createSemaphore(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT));

	m_computeShaderParser.reset(new ShaderParser("Shaders/shader.comp"));

	m_descriptorSetLayoutGenerator.addStorageImage(VK_SHADER_STAGE_COMPUTE_BIT, 0);
	m_descriptorSetLayout.reset(DescriptorSetLayout::createDescriptorSetLayout(m_descriptorSetLayoutGenerator.getDescriptorLayouts()));

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
	m_commandBuffer->beginCommandBuffer();

	const uint32_t outputImageIdx = context.swapChainImageIdx;

	context.swapchainImage->transitionImageLayout(*m_commandBuffer, { VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT });

	m_commandBuffer->bindDescriptorSet(m_descriptorSets[outputImageIdx].get(), 0, *m_pipeline);
	m_commandBuffer->bindPipeline(m_pipeline.get());

	const VkExtent3D dispatchGroups = { 16, 16, 1 };
	const uint32_t groupSizeX = m_swapChainWidth % dispatchGroups.width != 0 ? m_swapChainWidth / dispatchGroups.width + 1 : m_swapChainWidth / dispatchGroups.width;
	const uint32_t groupSizeY = m_swapChainHeight % dispatchGroups.height != 0 ? m_swapChainHeight / dispatchGroups.height + 1 : m_swapChainHeight / dispatchGroups.height;
	m_commandBuffer->dispatch(groupSizeX, groupSizeY, dispatchGroups.depth);

	context.swapchainImage->transitionImageLayout(*m_commandBuffer, { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });

	m_commandBuffer->endCommandBuffer();
}

void UniquePass::submit(const SubmitContext& context)
{
	const std::vector waitSemaphores{ context.swapChainImageAvailableSemaphore };
	const std::vector<const Semaphore*> signalSemaphores{ m_semaphore.get() };
	m_commandBuffer->submit(waitSemaphores, signalSemaphores, context.frameFence);

	if (m_computeShaderParser->compileIfFileHasBeenModified())
	{
		context.graphicAPIManager->waitIdle();
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
	computeShaderCreateInfo.stage = COMPUTE;

	std::vector<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts;
	descriptorSetLayouts.emplace_back(m_descriptorSetLayout.get());
	m_pipeline.reset(Pipeline::createComputePipeline(computeShaderCreateInfo, descriptorSetLayouts));

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
			m_descriptorSets[i].reset(DescriptorSet::createDescriptorSet(*m_descriptorSetLayout));
		m_descriptorSets[i]->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
	}
}
