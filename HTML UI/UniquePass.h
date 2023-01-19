#pragma once

#include <iostream>
#include <vector>

#include <Buffer.h>
#include <CommandBuffer.h>
#include <DescriptorSetGenerator.h>
#include <DescriptorSetLayout.h>
#include <DescriptorSetLayoutGenerator.h>
#include <FrameBuffer.h>
#include <Image.h>
#include <Mesh.h>
#include <PassBase.h>
#include <Pipeline.h>
#include <RenderPass.h>
#include <ShaderParser.h>

class UniquePass : public Wolf::PassBase
{
public:
	void initializeResources(const Wolf::InitializationContext& context) override;
	void resize(const Wolf::InitializationContext& context) override;
	void record(const Wolf::RecordContext& context) override;
	void submit(const Wolf::SubmitContext& context) override;

	const Wolf::Semaphore* getSemaphore() const { return m_semaphore.get(); }

private:
	void createDepthImage(const Wolf::InitializationContext& context);
	void createPipelines(uint32_t width, uint32_t height);

private:
	std::unique_ptr<Wolf::RenderPass> m_renderPass;
	std::unique_ptr<Wolf::Image> m_depthImage;

	std::unique_ptr<Wolf::CommandBuffer> m_commandBuffer;
	std::vector<std::unique_ptr<Wolf::Framebuffer>> m_frameBuffers;

	std::unique_ptr<Wolf::Semaphore> m_semaphore;

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

	std::unique_ptr<Wolf::Sampler> m_sampler;
	std::unique_ptr<Wolf::DescriptorSetLayout> m_userInterfaceDescriptorSetLayout;
	std::unique_ptr<Wolf::DescriptorSet> m_userInterfaceDescriptorSet;
	Wolf::DescriptorSetLayoutGenerator descriptorSetLayoutGenerator;

	std::vector<unsigned char> m_userInterfaceBufferCPU;
};