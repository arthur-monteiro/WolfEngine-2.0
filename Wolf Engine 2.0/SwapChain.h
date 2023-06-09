#pragma once

#ifdef __ANDROID__
#include <android/native_window.h>
#include <android/native_window_jni.h>
#endif
#include <memory>

#include "Fence.h"
#include "Image.h"

struct GLFWwindow;

namespace Wolf
{
	class Semaphore;

	class SwapChain
	{
	public:
#ifndef __ANDROID__
		SwapChain(GLFWwindow* window);
#else
		SwapChain(ANativeWindow* window);
#endif
		~SwapChain();

		void synchroniseCPUFromGPU(uint32_t currentFrameGPU) const;
		uint32_t getCurrentImage(uint32_t currentFrameGPU) const;
		void present(VkSemaphore waitSemaphore, uint32_t imageIndex) const;

#ifndef __ANDROID__
		void recreate(GLFWwindow* window);
#else
		void recreate();
#endif

		// Getters
		[[nodiscard]] Image* getImage(uint32_t index) const { return m_images[index].get(); }
		[[nodiscard]] uint32_t getImageCount() const { return m_images.size(); }
		[[nodiscard]] const Semaphore* getImageAvailableSemaphore(uint32_t index) const { return m_imageAvailableSemaphores[index].get(); }
		[[nodiscard]] VkFence getFrameFence(uint32_t index) const { return m_frameFences[index]->getFence(); }

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
