#pragma once

#include <iostream>
#ifdef __ANDROID__
#include <android/native_window.h>
#include <android/native_window_jni.h>
#endif

#include "Fence.h"
#include "Image.h"
#include "Semaphore.h"

namespace Wolf
{
	class SwapChain
	{
	public:
#ifndef __ANDROID__
		SwapChain(GLFWwindow* window);
#else
		SwapChain(ANativeWindow* window);
#endif
		~SwapChain();

		void synchroniseCPUFromGPU(uint32_t currentFrameGPU);
		uint32_t getCurrentImage(uint32_t currentFrameGPU);
		void present(VkSemaphore waitSemaphore, uint32_t imageIndex);

#ifndef __ANDROID__
		void recreate(GLFWwindow* window);
#else
		void recreate();
#endif

		// Getters
		Image* getImage(uint32_t index) const { return m_images[index].get(); }
		uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }
		const Semaphore* getImageAvailableSemaphore(uint32_t index) const { return m_imageAvailableSemaphores[index].get(); }
		VkFence getFrameFence(uint32_t index) const { return m_frameFences[index]->getFence(); }

	private:
#ifndef __ANDROID__
		void initialize(GLFWwindow* window);
#else
		void initialize(ANativeWindow* window);
#endif

	private:
		VkSwapchainKHR m_swapChain;
		std::vector<std::unique_ptr<Image>> m_images;
		bool m_invertColors = false;

		std::vector<std::unique_ptr<Semaphore>> m_imageAvailableSemaphores;
		std::vector<std::unique_ptr<Fence>> m_frameFences;
	};
}
