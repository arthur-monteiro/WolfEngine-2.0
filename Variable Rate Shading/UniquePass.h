#pragma once

#include <chrono>
#include <iostream>
#include <vector>

#include <Buffer.h>
#include <CommandBuffer.h>
#include <CommandRecordBase.h>
#include <DescriptorSet.h>
#include <DescriptorSetLayout.h>
#include <FrameBuffer.h>
#include <Image.h>
#include <Mesh.h>
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
	void createDepthImage(const Wolf::InitializationContext& context);
	void createPipeline(uint32_t width, uint32_t height);

private:
	std::unique_ptr<Wolf::RenderPass> m_renderPass;
	std::unique_ptr<Wolf::Image> m_depthImage;
	std::vector<std::unique_ptr<Wolf::FrameBuffer>> m_frameBuffers;

	/* Pipeline */
	std::unique_ptr<Wolf::ShaderParser> m_vertexShaderParser;
	std::unique_ptr<Wolf::ShaderParser> m_fragmentShaderParser;
	std::unique_ptr<Wolf::Pipeline> m_pipeline;
	uint32_t m_swapChainWidth;
	uint32_t m_swapChainHeight;

	std::unique_ptr<Wolf::Mesh> m_rectangle;
	std::unique_ptr<Wolf::Image> m_shadingRateImage;

	/* Resources*/
	std::unique_ptr<Wolf::Image> m_texture;
	std::unique_ptr<Wolf::Sampler> m_sampler;

	std::unique_ptr<Wolf::DescriptorSetLayout> m_descriptorSetLayout;
	std::unique_ptr<Wolf::DescriptorSet> m_descriptorSet;
};