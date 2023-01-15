#pragma once

#include <iostream>
#include <vector>

#include <Buffer.h>
#include <CommandBuffer.h>
#include <DescriptorSetGenerator.h>
#include <FrameBuffer.h>
#include <Image.h>
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
	void createPipeline(uint32_t width, uint32_t height);
	void readFile(std::vector<char>& output, const std::string& filename);

private:
	std::unique_ptr<Wolf::RenderPass> m_renderPass;
	std::unique_ptr<Wolf::Image> m_depthImage;

	std::unique_ptr<Wolf::CommandBuffer> m_commandBuffer;
	std::vector<std::unique_ptr<Wolf::Framebuffer>> m_frameBuffers;

	std::unique_ptr<Wolf::Semaphore> m_semaphore;

	std::unique_ptr<Wolf::Pipeline> m_pipeline;
	uint32_t m_swapChainWidth;
	uint32_t m_swapChainHeight;

	std::unique_ptr<Wolf::Buffer> m_vertexBuffer;
	std::unique_ptr<Wolf::Buffer> m_indexBuffer;

	std::unique_ptr<Wolf::ShaderParser> m_vertexShaderParser;
	std::unique_ptr<Wolf::ShaderParser> m_fragmentShaderParser;
};