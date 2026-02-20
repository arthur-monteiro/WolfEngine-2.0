#include "UniquePass.h"

#include <filesystem>
#include <fstream>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <DescriptorSetGenerator.h>
#include <DescriptorSetLayoutGenerator.h>
#include <Image.h>
#include <ImageFileLoader.h>

using namespace Wolf;

void UniquePass::initializeResources(const InitializationContext& context)
{
	m_commandBuffer.reset(CommandBuffer::createCommandBuffer(QueueType::COMPUTE, false /* isTransient */));
	createSemaphores(context, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, true);

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

	const uint32_t outputImageIdx = context.m_swapChainImageIdx;

	context.m_swapchainImage->transitionImageLayout(*m_commandBuffer, { ImageLayout::GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT });

	m_commandBuffer->bindDescriptorSet(m_descriptorSets[outputImageIdx].get(), 0, *m_pipeline);
	m_commandBuffer->bindPipeline(m_pipeline.get());

	const VkExtent3D dispatchGroups = { 16, 16, 1 };
	const uint32_t groupSizeX = m_swapChainWidth % dispatchGroups.width != 0 ? m_swapChainWidth / dispatchGroups.width + 1 : m_swapChainWidth / dispatchGroups.width;
	const uint32_t groupSizeY = m_swapChainHeight % dispatchGroups.height != 0 ? m_swapChainHeight / dispatchGroups.height + 1 : m_swapChainHeight / dispatchGroups.height;
	m_commandBuffer->dispatch(groupSizeX, groupSizeY, dispatchGroups.depth);

	context.m_swapchainImage->transitionImageLayout(*m_commandBuffer, { ImageLayout::PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });

	m_commandBuffer->endCommandBuffer();

	m_lastSwapChainImage = context.m_swapchainImage;
}

void UniquePass::submit(const SubmitContext& context)
{
	const std::vector waitSemaphores{ context.swapChainImageAvailableSemaphore };
	const std::vector<const Semaphore*> signalSemaphores{ getSemaphore(context.swapChainImageIndex) };
	m_commandBuffer->submit(waitSemaphores, signalSemaphores, context.frameFence);

	if (m_computeShaderParser->compileIfFileHasBeenModified())
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
		DescriptorSetGenerator::ImageDescription image(ImageLayout::GENERAL, outputImages[i]->getDefaultImageView());
		descriptorSetGenerator.setImage(0, image);

		if(!m_descriptorSets[i])
			m_descriptorSets[i].reset(DescriptorSet::createDescriptorSet(*m_descriptorSetLayout));
		m_descriptorSets[i]->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
	}
}
