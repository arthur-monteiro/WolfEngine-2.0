#pragma once

#include <memory>
#include <vector>

#include "Extents.h"
#include "Formats.h"

#ifndef __ANDROID__
struct GLFWwindow;
#endif

namespace Wolf
{
	class Fence;
	class Image;
	class Semaphore;

	class SwapChain
	{
	public:
		struct SwapChainCreateInfo
		{
			Extent2D extent;
			Format format;
			enum class ColorSpace { S_RGB, SC_RGB, PQ, LINEAR } colorSpace = ColorSpace::S_RGB;

#ifdef __ANDROID__
            // On Android, the swapchain is responsible of detecting a resize
            std::function<void(int, int)> m_resizeCallback;
#endif
		};

		static SwapChain* createSwapChain(const SwapChainCreateInfo& swapChainCreateInfo);
		virtual ~SwapChain() = default;

		void synchroniseCPUFromGPU(uint32_t currentFrameGPU) const;
		static constexpr uint32_t NO_IMAGE_IDX = static_cast<uint32_t>(-1);
		virtual uint32_t acquireNextImage(uint32_t currentFrameGPU) const = 0;
		virtual void present(const Semaphore* frameEndedSemaphore, uint32_t imageIndex) const = 0;

		virtual void resetAllFences() = 0;
		virtual void recreate(Extent2D extent) = 0;
		
		[[nodiscard]] Image* getImage(uint32_t index) const { return m_images[index].get(); }
		[[nodiscard]] uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }
		[[nodiscard]] const Semaphore* getImageAvailableSemaphore(uint32_t index) const { return m_imageAvailableSemaphores[index].get(); }
		[[nodiscard]] Fence* getFrameFence(uint32_t index) const { return m_frameFences[index].get(); }
		[[nodiscard]] SwapChainCreateInfo::ColorSpace getColorSpace() const { return m_colorSpace; }
        [[nodiscard]] float getRotationInDegrees() const { return m_rotationInDegrees; }

	protected:
		std::vector<std::unique_ptr<Image>> m_images;

		std::vector<std::unique_ptr<Semaphore>> m_imageAvailableSemaphores;
		std::vector<std::unique_ptr<Fence>> m_frameFences;

		SwapChainCreateInfo::ColorSpace m_colorSpace = SwapChainCreateInfo::ColorSpace::S_RGB;
        float m_rotationInDegrees = 0.0f;

#ifdef __ANDROID__
        std::function<void(int, int)> m_resizeCallback;
#endif
	};
}