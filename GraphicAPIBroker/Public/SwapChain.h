#pragma once

#include <memory>
#include <vector>
// TEMP
#include <vulkan/vulkan_core.h>

struct GLFWwindow;

namespace Wolf
{
	class Fence;
	class Image;
	class Semaphore;

	class SwapChain
	{
	public:
		static SwapChain* createSwapChain(VkExtent2D extent);
		virtual ~SwapChain() = default;

		void synchroniseCPUFromGPU(uint32_t currentFrameGPU) const;
		virtual uint32_t getCurrentImage(uint32_t currentFrameGPU) const = 0;
		virtual void present(const Semaphore* waitSemaphore, uint32_t imageIndex) const = 0;


		virtual void recreate(VkExtent2D extent) = 0;
		
		[[nodiscard]] Image* getImage(uint32_t index) const { return m_images[index].get(); }
		[[nodiscard]] uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }
		[[nodiscard]] const Semaphore* getImageAvailableSemaphore(uint32_t index) const { return m_imageAvailableSemaphores[index].get(); }
		[[nodiscard]] Fence* getFrameFence(uint32_t index) const { return m_frameFences[index].get(); }

	protected:
		std::vector<std::unique_ptr<Image>> m_images;
		bool m_invertColors = false;

		std::vector<std::unique_ptr<Semaphore>> m_imageAvailableSemaphores;
		std::vector<std::unique_ptr<Fence>> m_frameFences;
	};
}
