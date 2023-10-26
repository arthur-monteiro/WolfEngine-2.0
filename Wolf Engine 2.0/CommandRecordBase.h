#pragma once

#include <stdint.h>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "CommandBuffer.h"
#include "Semaphore.h"

namespace Wolf
{
	class Image;
	class CameraInterface;

	struct InitializationContext
	{
		uint32_t swapChainWidth;
		uint32_t swapChainHeight;
		VkFormat swapChainFormat;
		VkFormat depthFormat;
		uint32_t swapChainImageCount;
		std::vector<Image*> swapChainImages;
		Image* userInterfaceImage;
		CameraInterface* camera;
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
		CameraInterface* camera;
		const void* gameContext;
	};

	struct SubmitContext
	{
		uint32_t currentFrameIdx;
		uint32_t commandBufferIdx;
		const Semaphore* swapChainImageAvailableSemaphore;
		const Semaphore* userInterfaceImageAvailableSemaphore;
		VkFence frameFence;
		VkDevice device;
	};

	class CommandRecordBase
	{
	public:
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
