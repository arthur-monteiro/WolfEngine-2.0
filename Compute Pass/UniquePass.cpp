#include "UniquePass.h"

#include <filesystem>
#include <fstream>

#include <Attachment.h>
#include <DescriptorSetGenerator.h>
#include <DescriptorSetLayoutGenerator.h>
#include <Image.h>
#include <ImageFileLoader.h>
#include <FrameBuffer.h>

using namespace Wolf;

void UniquePass::initializeResources(const InitializationContext& context)
{
	m_commandBuffer.reset(new CommandBuffer(QueueType::GRAPHIC, false /* isTransient */));
	m_semaphore.reset(new Semaphore(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT));

	m_computeShaderParser.reset(new ShaderParser("Shaders/shader.comp"));
;
	m_descriptorSetLayoutGenerator.addStorageImage(VK_SHADER_STAGE_COMPUTE_BIT, 0);
	m_descriptorSetLayout.reset(new DescriptorSetLayout(m_descriptorSetLayoutGenerator.getDescriptorLayouts()));

	m_descriptorSets.resize(context.swapChainImageCount);
	createDescriptorSets(context);

	createPipeline(context.swapChainWidth, context.swapChainHeight);
}

void UniquePass::resize(const Wolf::InitializationContext& context)
{
	createDescriptorSets(context);
	m_swapChainWidth = context.swapChainWidth;
	m_swapChainHeight = context.swapChainHeight;
}

void UniquePass::record(const Wolf::RecordContext& context)
{
	/* Command buffer record */
	uint32_t frameBufferIdx = context.swapChainImageIdx;

	m_commandBuffer->beginCommandBuffer(context.commandBufferIdx);

	context.swapchainImage->transitionImageLayout(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	vkCmdBindDescriptorSets(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->getPipelineLayout(), 0, 1, m_descriptorSets[context.swapChainImageIdx]->getDescriptorSet(), 0, nullptr);

	vkCmdBindPipeline(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->getPipeline());

	VkExtent3D dispatchGroups = { 16, 16, 1 };
	uint32_t groupSizeX = m_swapChainWidth % dispatchGroups.width != 0 ? m_swapChainWidth / dispatchGroups.width + 1 : m_swapChainWidth / dispatchGroups.width;
	uint32_t groupSizeY = m_swapChainHeight % dispatchGroups.height != 0 ? m_swapChainHeight / dispatchGroups.height + 1 : m_swapChainHeight / dispatchGroups.height;
	vkCmdDispatch(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), groupSizeX, groupSizeY, dispatchGroups.depth);

	context.swapchainImage->transitionImageLayout(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

	m_commandBuffer->endCommandBuffer(context.commandBufferIdx);
}

void UniquePass::submit(const Wolf::SubmitContext& context)
{
	std::vector<const Semaphore*> waitSemaphores{ context.imageAvailableSemaphore };
	std::vector<VkSemaphore> signalSemaphores{ m_semaphore->getSemaphore() };
	m_commandBuffer->submit(context.commandBufferIdx, waitSemaphores, signalSemaphores, context.frameFence);

	if (m_computeShaderParser->compileIfFileHasBeenModified())
	{
		vkDeviceWaitIdle(context.device);
		createPipeline(m_swapChainWidth, m_swapChainHeight);
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

void UniquePass::createDescriptorSets(const Wolf::InitializationContext& context)
{
	for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
	{
		DescriptorSetGenerator descriptorSetGenerator(m_descriptorSetLayoutGenerator.getDescriptorLayouts());
		DescriptorSetGenerator::ImageDescription image(VK_IMAGE_LAYOUT_GENERAL, context.swapChainImages[i]->getDefaultImageView());
		descriptorSetGenerator.setImage(0, image);

		if(!m_descriptorSets[i])
			m_descriptorSets[i].reset(new DescriptorSet(m_descriptorSetLayout->getDescriptorSetLayout(), UpdateRate::NEVER));
		m_descriptorSets[i]->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
	}
}
