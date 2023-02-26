#pragma once

#include <chrono>
#include <iostream>
#include <vector>

#include <Buffer.h>
#include <CommandBuffer.h>
#include <CommandRecordBase.h>
#include <DescriptorSet.h>
#include <DescriptorSetLayout.h>
#include <DescriptorSetLayoutGenerator.h>
#include <FrameBuffer.h>
#include <Image.h>
#include <Pipeline.h>
#include <RenderPass.h>
#include <Sampler.h>
#include <ShaderParser.h>

class UniquePass : public Wolf::CommandRecordBase
{
public:
	void initializeResources(const Wolf::InitializationContext& context) override;
	void resize(const Wolf::InitializationContext& context) override;
	void record(const Wolf::RecordContext& context) override;
	void submit(const Wolf::SubmitContext& context) override;

private:
	void createPipeline(uint32_t width, uint32_t height);
	void createDescriptorSets(const Wolf::InitializationContext& context);

private:
	/* Pipeline */
	std::unique_ptr<Wolf::ShaderParser> m_computeShaderParser;
	std::unique_ptr<Wolf::Pipeline> m_pipeline;
	uint32_t m_swapChainWidth;
	uint32_t m_swapChainHeight;

	Wolf::DescriptorSetLayoutGenerator m_descriptorSetLayoutGenerator;
	std::unique_ptr<Wolf::DescriptorSetLayout> m_descriptorSetLayout;
	std::vector<std::unique_ptr<Wolf::DescriptorSet>> m_descriptorSets;
};