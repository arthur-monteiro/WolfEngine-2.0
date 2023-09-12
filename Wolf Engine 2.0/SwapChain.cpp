#include "SwapChain.h"

#include "Configuration.h"
#include "Debug.h"
#include "Image.h"
#include "Fence.h"
#include "Semaphore.h"
#include "SwapChainSupportDetails.h"
#include "Vulkan.h"
#include "VulkanHelper.h"
#include "Window.h"

VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}

	return availableFormats[0];
}

VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
	return bestMode;

	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			bestMode = availablePresentMode;
	}

	return bestMode;
}

#ifndef __ANDROID__
VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else
	{
		int width, height;
		glfwGetWindowSize(window, &width, &height);

		VkExtent2D actualExtent =
		{
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}
#endif

#ifdef __ANDROID__
Wolf::SwapChain::SwapChain(ANativeWindow* window)
{
	initialize(window);
}
#else
Wolf::SwapChain::SwapChain(GLFWwindow* window)
{
	initialize(window);
}
#endif

#ifdef __ANDROID__
void Wolf::SwapChain::initialize(ANativeWindow* window)
#else
void Wolf::SwapChain::initialize(GLFWwindow* window)
#endif
{
	SwapChainSupportDetails swapChainSupport;
	querySwapChainSupport(swapChainSupport, g_vulkanInstance->getPhysicalDevice(), g_vulkanInstance->getSurface());

	const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapChainSupport.formats);
	const VkPresentModeKHR presentMode = choosePresentMode(swapChainSupport.presentModes);
#ifdef __ANDROID__
	int32_t width = ANativeWindow_getWidth(window);
    int32_t height = ANativeWindow_getHeight(window);
    VkExtent2D extent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};
    //extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    //extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
#else
	VkExtent2D extent = chooseExtent(swapChainSupport.capabilities, window);
#endif

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		imageCount = swapChainSupport.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = g_vulkanInstance->getSurface();

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

	const QueueFamilyIndices& indices = g_vulkanInstance->getQueueFamilyIndices();
	const uint32_t queueFamilyIndices[] = { static_cast<uint32_t>(indices.graphicsFamily), static_cast<uint32_t>(indices.presentFamily) };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(g_vulkanInstance->getDevice(), &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
		Debug::sendCriticalError("Error : swapchain creation");

	// Get number of images
	vkGetSwapchainImagesKHR(g_vulkanInstance->getDevice(), m_swapChain, &imageCount, nullptr);

	m_images.resize(imageCount);
	std::vector<VkImage> temporarySwapChainImages(imageCount);

	vkGetSwapchainImagesKHR(g_vulkanInstance->getDevice(), m_swapChain, &imageCount, temporarySwapChainImages.data());

	VkFormatProperties swapchainFormatProperties;
	vkGetPhysicalDeviceFormatProperties(g_vulkanInstance->getPhysicalDevice(), surfaceFormat.format, &swapchainFormatProperties);

	if (!(swapchainFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
	{
		m_invertColors = true;
		//surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
	}

	for (size_t i(0); i < imageCount; ++i)
	{
		m_images[i].reset(new Image(temporarySwapChainImages[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, extent));
	}

	m_imageAvailableSemaphores.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		m_imageAvailableSemaphores[i].reset(new Semaphore(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT));
	}

	m_frameFences.resize(g_configuration->getMaxCachedFrames());
	for (uint32_t i = 0; i < g_configuration->getMaxCachedFrames(); ++i)
	{
		m_frameFences[i].reset(new Fence(0));
	}
}

Wolf::SwapChain::~SwapChain()
{
	vkDestroySwapchainKHR(g_vulkanInstance->getDevice(), m_swapChain, nullptr);
}

void Wolf::SwapChain::synchroniseCPUFromGPU(uint32_t currentFrame) const
{
	m_frameFences[currentFrame % m_frameFences.size()]->waitForFence();
	m_frameFences[currentFrame % m_frameFences.size()]->resetFence();
}

uint32_t Wolf::SwapChain::getCurrentImage(uint32_t currentFrame) const
{
	uint32_t imageIndex;
	const VkResult result = vkAcquireNextImageKHR(g_vulkanInstance->getDevice(), m_swapChain, std::numeric_limits<uint64_t>::max(), m_imageAvailableSemaphores[currentFrame % m_imageAvailableSemaphores.size()]->getSemaphore(), VK_NULL_HANDLE, &imageIndex);

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		Debug::sendCriticalError("Error : can't acquire image");
	}

	return imageIndex;
}

void Wolf::SwapChain::present(VkSemaphore waitSemaphore, uint32_t imageIndex) const
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	if (waitSemaphore)
	{
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &waitSemaphore;
	}
	else
	{
		presentInfo.waitSemaphoreCount = 0;
		presentInfo.pWaitSemaphores = nullptr;
	}

	const VkSwapchainKHR swapChains[] = { m_swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	const VkResult result = vkQueuePresentKHR(g_vulkanInstance->getPresentQueue(), &presentInfo);

	/*if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		recreateSwapChain();
	else */if (result != VK_SUCCESS)
		Debug::sendCriticalError("Can't present image");
}

#ifdef __ANDROID__
void Wolf::SwapChain::recreate()
#else
void Wolf::SwapChain::recreate(GLFWwindow* window)
#endif
{
	vkDestroySwapchainKHR(g_vulkanInstance->getDevice(), m_swapChain, nullptr);
	m_images.clear();
	m_frameFences.clear();

#ifdef __ANDROID__
	//initialize();
#else
	initialize(window);
#endif
}