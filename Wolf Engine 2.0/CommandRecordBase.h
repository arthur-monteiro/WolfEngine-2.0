#pragma once

#include <memory>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <CommandBuffer.h>
#include <GraphicAPIManager.h>
#include <Semaphore.h>

struct GLFWwindow;

namespace Wolf
{
	class CameraList;
	class DescriptorSet;
	class Image;
	class RenderMeshList;

	struct InitializationContext
	{
		uint32_t swapChainWidth;
		uint32_t swapChainHeight;
		VkFormat swapChainFormat;
		VkFormat depthFormat;
		uint32_t swapChainImageCount;
		std::vector<Image*> swapChainImages;
		Image* userInterfaceImage;
	};

	struct RecordContext
	{
		uint32_t currentFrameIdx;
		uint32_t commandBufferIdx; 
		uint32_t swapChainImageIdx;
		Image* swapchainImage;
#ifndef __ANDROID__
		GLFWwindow* glfwWindow;
#endif
		CameraList* cameraList;
		const void* gameContext;
		RenderMeshList* renderMeshList;
		const DescriptorSet* bindlessDescriptorSet;
	};

	struct SubmitContext
	{
		uint32_t currentFrameIdx;
		uint32_t commandBufferIdx;
		const Semaphore* swapChainImageAvailableSemaphore;
		const Semaphore* userInterfaceImageAvailableSemaphore;
		Fence* frameFence;
		GraphicAPIManager* graphicAPIManager;
	};

	class CommandRecordBase
	{
	public:
		virtual ~CommandRecordBase() = default;
		virtual void initializeResources(const InitializationContext& context) = 0;
		virtual void resize(const InitializationContext& context) = 0;
		virtual void record(const RecordContext& context) = 0;
		virtual void submit(const SubmitContext& context) = 0;

		[[nodiscard]] virtual const Semaphore* getSemaphore() const { return m_semaphore.get(); }

	protected:
		std::unique_ptr<CommandBuffer> m_commandBuffer;
		std::unique_ptr<Semaphore> m_semaphore;
	};
}
