#pragma once

#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include <Buffer.h>
#include <CommandBuffer.h>
#include <CommandRecordBase.h>
#include <DescriptorSetGenerator.h>
#include <DescriptorSetLayout.h>
#include <DescriptorSetLayoutGenerator.h>
#include <FrameBuffer.h>
#include <Image.h>
#include <Mesh.h>
#include <Pipeline.h>
#include <RenderPass.h>
#include <ShaderParser.h>
#include <UltraLight.h>

class UniquePass : public Wolf::CommandRecordBase
{
public:
	void initializeResources(const Wolf::InitializationContext& context) override;
	void resize(const Wolf::InitializationContext& context) override;
	void record(const Wolf::RecordContext& context) override;
	void submit(const Wolf::SubmitContext& context) override;

	void setTriangleColor(const glm::vec3& color) const;

private:
	void createDepthImage(const Wolf::InitializationContext& context);
	void createPipelines(uint32_t width, uint32_t height);

private:
	std::unique_ptr<Wolf::RenderPass> m_renderPass;
	std::unique_ptr<Wolf::Image> m_depthImage;
	std::vector<std::unique_ptr<Wolf::FrameBuffer>> m_frameBuffers;

	std::unique_ptr<Wolf::Pipeline> m_trianglePipeline;
	std::unique_ptr<Wolf::Pipeline> m_userInterfacePipeline;
	uint32_t m_swapChainWidth;
	uint32_t m_swapChainHeight;

	std::unique_ptr<Wolf::Mesh> m_triangle;
	std::unique_ptr<Wolf::Mesh> m_fullScreenRectangle;

	std::unique_ptr<Wolf::ShaderParser> m_triangleVertexShaderParser;
	std::unique_ptr<Wolf::ShaderParser> m_triangleFragmentShaderParser;

	std::unique_ptr<Wolf::ShaderParser> m_userInterfaceVertexShaderParser;
	std::unique_ptr<Wolf::ShaderParser> m_userInterfaceFragmentShaderParser;

	// Triangle resources
	std::unique_ptr<Wolf::Buffer> m_triangleUniformBuffer;
	std::unique_ptr<Wolf::DescriptorSetLayout> m_triangleDescriptorSetLayout;
	std::unique_ptr<Wolf::DescriptorSet> m_triangleDescriptorSet;

	// UI resources
	std::unique_ptr<Wolf::Sampler> m_sampler;
	std::unique_ptr<Wolf::DescriptorSetLayout> m_userInterfaceDescriptorSetLayout;
	std::unique_ptr<Wolf::DescriptorSet> m_userInterfaceDescriptorSet;
	Wolf::DescriptorSetLayoutGenerator m_userInterfaceDescriptorSetLayoutGenerator;

	std::vector<unsigned char> m_userInterfaceBufferCPU;
};