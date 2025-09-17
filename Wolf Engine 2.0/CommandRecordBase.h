#pragma once

#include <memory>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <CommandBuffer.h>
#include <Formats.h>
#include <GPUSemaphore.h>
#include <GraphicAPIManager.h>

struct GLFWwindow;

namespace Wolf
{
	class CameraList;
	class DescriptorSet;
	class Image;
	class LightManager;
	class RenderMeshList;
	class Timer;

	struct InitializationContext
	{
		uint32_t swapChainWidth = 0;
		uint32_t swapChainHeight = 0;
		Format swapChainFormat = Format::UNDEFINED;
		Format depthFormat = Format::UNDEFINED;
		uint32_t swapChainImageCount = 0;
		std::vector<Image*> swapChainImages;
		Image* userInterfaceImage = nullptr;
	};

	struct RecordContext
	{
		uint32_t currentFrameIdx = 0;
		uint32_t swapChainImageIdx = 0;
		Image* swapchainImage = nullptr;
#ifndef __ANDROID__
		GLFWwindow* glfwWindow = nullptr;
#endif
		CameraList* cameraList = nullptr;
		const void* gameContext = nullptr;
		ResourceNonOwner<RenderMeshList> renderMeshList;
		const DescriptorSet* bindlessDescriptorSet = nullptr;
		ResourceNonOwner<LightManager> lightManager;
		const Timer* globalTimer = nullptr;

		RecordContext(const ResourceNonOwner<LightManager>& lightManager, const ResourceNonOwner<RenderMeshList>& renderMeshList)
			: renderMeshList(renderMeshList), lightManager(lightManager) {}
	};

	struct SubmitContext
	{
		uint32_t currentFrameIdx;
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
