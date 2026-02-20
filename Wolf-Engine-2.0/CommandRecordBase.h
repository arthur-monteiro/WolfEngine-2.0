#pragma once

#include <memory>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <CommandBuffer.h>
#include <Formats.h>
#include <GPUSemaphore.h>
#include <GraphicAPIManager.h>
#include <RuntimeContext.h>
#include <SwapChain.h>

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
		SwapChain::SwapChainCreateInfo::ColorSpace swapChainColorSpace;
		Image* userInterfaceImage = nullptr;
	};

	struct RecordContext
	{
		uint32_t m_currentFrameIdx = 0;
		uint32_t m_swapChainImageIdx = 0;
		Image* m_swapchainImage = nullptr;
        float m_swapchainRotation = 0.0f;
#ifndef __ANDROID__
		GLFWwindow* m_glfwWindow = nullptr;
#endif
		CameraList* m_cameraList = nullptr;
		const void* m_gameContext = nullptr;
		ResourceNonOwner<RenderMeshList> m_renderMeshList;
		const DescriptorSet* m_materialGPUManagerDescriptorSet = nullptr;
		ResourceNonOwner<LightManager> m_lightManager;
		const Timer* m_globalTimer = nullptr;
		GraphicAPIManager* m_graphicAPIManager;
		bool* m_invalidateFrame;

		RecordContext(const ResourceNonOwner<LightManager>& lightManager, const ResourceNonOwner<RenderMeshList>& renderMeshList)
			: m_renderMeshList(renderMeshList), m_lightManager(lightManager) {}
	};

	struct SubmitContext
	{
		uint32_t currentFrameIdx;
		const Semaphore* swapChainImageAvailableSemaphore;
		const Semaphore* userInterfaceImageAvailableSemaphore;
		Fence* frameFence;
		GraphicAPIManager* graphicAPIManager;
		uint32_t swapChainImageIndex;
	};

	class CommandRecordBase
	{
	public:
		virtual ~CommandRecordBase() = default;
		virtual void initializeResources(const InitializationContext& context) = 0;
		virtual void resize(const InitializationContext& context) = 0;
		virtual void record(const RecordContext& context) = 0;
		virtual void submit(const SubmitContext& context) = 0;

		[[nodiscard]] virtual Semaphore* getSemaphore(uint32_t swapChainImageIdx) const { return m_semaphores[swapChainImageIdx % m_semaphores.size()].get(); }

	protected:
		void createSemaphores(const InitializationContext& context, uint32_t pipelineStage, bool isFinalPass);

		std::unique_ptr<CommandBuffer> m_commandBuffer;

	private:
		std::vector<std::unique_ptr<Semaphore>> m_semaphores;
	};
}
