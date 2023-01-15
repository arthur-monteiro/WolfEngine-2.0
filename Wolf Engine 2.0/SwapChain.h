#pragma once

#include <iostream>

#include "Fence.h"
#include "Image.h"
#include "Semaphore.h"

namespace Wolf
{
	class SwapChain
	{
	public:
		SwapChain(GLFWwindow* window);
		~SwapChain();

		void synchroniseCPUFromGPU(uint32_t currentFrameGPU);
		uint32_t getCurrentImage(uint32_t currentFrameGPU);
		void present(VkSemaphore waitSemaphore, uint32_t imageIndex);

		void recreate(GLFWwindow* window);

		// Getters
		Image* getImage(uint32_t index) const { return m_images[index].get(); }
		uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }
		const Semaphore* getImageAvailableSemaphore(uint32_t index) const { return m_imageAvailableSemaphores[index].get(); }
		VkFence getFrameFence(uint32_t index) const { return m_frameFences[index]->getFence(); }

	private:
		void initialize(GLFWwindow* window);

	private:
		VkSwapchainKHR m_swapChain;
		std::vector<std::unique_ptr<Image>> m_images;
		bool m_invertColors = false;

		std::vector<std::unique_ptr<Semaphore>> m_imageAvailableSemaphores;
		std::vector<std::unique_ptr<Fence>> m_frameFences;
	};
}
