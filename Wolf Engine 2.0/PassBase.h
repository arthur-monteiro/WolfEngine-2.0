#pragma once

#include <stdint.h>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Semaphore.h"

namespace Wolf
{
	class Image;

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
		GLFWwindow* glfwWindow;
	};

	struct SubmitContext
	{
		uint32_t currentFrameIdx;
		uint32_t commandBufferIdx;
		const Semaphore* imageAvailableSemaphore;
		VkFence frameFence;
		VkDevice device;
	};

	class PassBase
	{
	public:
		virtual void initializeResources(const InitializationContext& context) = 0;
		virtual void resize(const InitializationContext& context) = 0;
		virtual void record(const RecordContext& context) = 0;
		virtual void submit(const SubmitContext& context) = 0;
	};
}
