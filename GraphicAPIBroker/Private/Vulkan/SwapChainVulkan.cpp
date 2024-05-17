#include "SwapChainVulkan.h"

#include <Configuration.h>
#include <Debug.h>

#include "FenceVulkan.h"
#include "ImageVulkan.h"
#include "SemaphoreVulkan.h"
#include "SwapChainSupportDetails.h"
#include "Vulkan.h"
#include "VulkanHelper.h"

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

Wolf::SwapChainVulkan::SwapChainVulkan(VkExtent2D extent)
{
	initialize(extent);
}

void Wolf::SwapChainVulkan::initialize(VkExtent2D extent)
{
	SwapChainSupportDetails swapChainSupport;
	querySwapChainSupport(swapChainSupport, g_vulkanInstance->getPhysicalDevice(), g_vulkanInstance->getSurface());

	if(swapChainSupport.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() && swapChainSupport.capabilities.currentExtent.width < extent.width)
	{
		Debug::sendError("Requested width is too high, reducing to max capability");
		extent.width = swapChainSupport.capabilities.currentExtent.width;
	}
	if (swapChainSupport.capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max() && swapChainSupport.capabilities.currentExtent.height < extent.height)
	{
		Debug::sendError("Requested height is too high, reducing to max capability");
		extent.height = swapChainSupport.capabilities.currentExtent.height;
	}

	const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapChainSupport.formats);
	const VkPresentModeKHR presentMode = choosePresentMode(swapChainSupport.presentModes);

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
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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
#ifdef __ANDROID__
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
#else
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
#endif
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(g_vulkanInstance->getDevice(), &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
		Debug::sendCriticalError("Swapchain creation failed");

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
		m_images[i].reset(new ImageVulkan(temporarySwapChainImages[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, extent));
	}

	m_imageAvailableSemaphores.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		m_imageAvailableSemaphores[i].reset(new SemaphoreVulkan(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT));
	}

	m_frameFences.resize(g_configuration->getMaxCachedFrames());
	for (uint32_t i = 0; i < g_configuration->getMaxCachedFrames(); ++i)
	{
		m_frameFences[i].reset(new FenceVulkan(0));
	}
}

Wolf::SwapChainVulkan::~SwapChainVulkan()
{
	vkDestroySwapchainKHR(g_vulkanInstance->getDevice(), m_swapChain, nullptr);
}

uint32_t Wolf::SwapChainVulkan::getCurrentImage(uint32_t currentFrame) const
{
	uint32_t imageIndex;
	const VkResult result = vkAcquireNextImageKHR(g_vulkanInstance->getDevice(), m_swapChain, std::numeric_limits<uint64_t>::max(), 
		static_cast<const SemaphoreVulkan*>(m_imageAvailableSemaphores[currentFrame % m_imageAvailableSemaphores.size()].get())->getSemaphore(), VK_NULL_HANDLE, &imageIndex);

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		Debug::sendCriticalError("Error : can't acquire image");
	}

	return imageIndex;
}

void Wolf::SwapChainVulkan::present(const Semaphore* waitSemaphore, uint32_t imageIndex) const
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	if (waitSemaphore)
	{
		presentInfo.waitSemaphoreCount = 1;
		const VkSemaphore semaphore = static_cast<const SemaphoreVulkan*>(waitSemaphore)->getSemaphore();
		presentInfo.pWaitSemaphores = &semaphore;
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


void Wolf::SwapChainVulkan::recreate(VkExtent2D extent)
{
	vkDestroySwapchainKHR(g_vulkanInstance->getDevice(), m_swapChain, nullptr);
	m_images.clear();
	m_frameFences.clear();
	
	initialize(extent);
}