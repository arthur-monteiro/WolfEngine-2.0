#pragma once

#include <iostream>
#include <vector>

#include <BottomLevelAccelerationStructure.h>
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
#include <ShaderBindingTable.h>
#include <ShaderParser.h>
#include <TopLevelAccelerationStructure.h>

class UniquePass : public Wolf::CommandRecordBase
{
public:
	void initializeResources(const Wolf::InitializationContext& context) override;
	void resize(const Wolf::InitializationContext& context) override;
	void record(const Wolf::RecordContext& context) override;
	void submit(const Wolf::SubmitContext& context) override;

private:
	void createPipeline(uint32_t width, uint32_t height);

private:

	std::unique_ptr<Wolf::Pipeline> m_pipeline;
	uint32_t m_swapChainWidth;
	uint32_t m_swapChainHeight;

	std::unique_ptr<Wolf::Mesh> m_triangle;

	std::unique_ptr<Wolf::ShaderParser> m_rayGenShaderParser;
	std::unique_ptr<Wolf::ShaderParser> m_rayMissShaderParser;
	std::unique_ptr<Wolf::ShaderParser> m_closestHitShaderParser;

	std::unique_ptr<Wolf::BottomLevelAccelerationStructure> m_blas;
	std::unique_ptr<Wolf::TopLevelAccelerationStructure> m_tlas;
	std::unique_ptr<Wolf::ShaderBindingTable> m_shaderBindingTable;

	std::unique_ptr<Wolf::DescriptorSetLayout> m_descriptorSetLayout;
	std::vector<std::unique_ptr<Wolf::DescriptorSet>> m_descriptorSets;
};