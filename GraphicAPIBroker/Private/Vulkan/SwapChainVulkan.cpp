#include "SwapChainVulkan.h"

#include <Configuration.h>
#include <Debug.h>

#include "FenceVulkan.h"
#include "FormatsVulkan.h"
#include "ImageVulkan.h"
#include "SemaphoreVulkan.h"
#include "SwapChainSupportDetails.h"
#include "Vulkan.h"
#include "VulkanHelper.h"

VkSurfaceFormatKHR Wolf::SwapChainVulkan::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, Wolf::SwapChain::SwapChainCreateInfo::ColorSpace colorSpace, Format format)
{
	VkFormat fallbackFormat = VK_FORMAT_R8G8B8A8_UNORM;
	VkColorSpaceKHR fallbackColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	VkFormat preferredFormat;
	VkColorSpaceKHR preferredColorSpace;
	switch (colorSpace)
	{
		case SwapChainCreateInfo::ColorSpace::S_RGB:
			preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			break;
		case SwapChainCreateInfo::ColorSpace::SC_RGB:
		case SwapChainCreateInfo::ColorSpace::PQ:
			Debug::sendCriticalError("Color space is not currently supported by the wolf engine");
			break;
		case SwapChainCreateInfo::ColorSpace::LINEAR:
			preferredFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
			preferredColorSpace = VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
			break;
		default:
			Debug::sendCriticalError("Unknown color space");
	}

	preferredFormat = wolfFormatToVkFormat(format);

	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == preferredFormat && availableFormat.colorSpace == preferredColorSpace)
		{
			m_colorSpace = colorSpace;
			return availableFormat;
		}
	}

	Debug::sendError("Chosen swapchain format is not supported by the hardware, switching to fallback (SDR)");

	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == fallbackFormat && availableFormat.colorSpace == fallbackColorSpace)
		{
			m_colorSpace = SwapChainCreateInfo::ColorSpace::S_RGB;
			return availableFormat;
		}
	}

	Debug::sendCriticalError("Fallback format is not supported by the hardware, behaviour will be undefined");
	m_colorSpace = SwapChainCreateInfo::ColorSpace::S_RGB;
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

Wolf::SwapChainVulkan::SwapChainVulkan(const SwapChainCreateInfo& swapChainCreateInfo)
{
	m_format = swapChainCreateInfo.format;
	initialize(swapChainCreateInfo);
}

void Wolf::SwapChainVulkan::initialize(const SwapChainCreateInfo& swapChainCreateInfo)
{
	VkExtent2D extent = { swapChainCreateInfo.extent.width, swapChainCreateInfo.extent.height };

	SwapChainSupportDetails swapChainSupport;
	querySwapChainSupport(swapChainSupport, g_vulkanInstance->getPhysicalDevice(), g_vulkanInstance->getSurface());

	if(swapChainSupport.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() && swapChainSupport.capabilities.currentExtent.width < swapChainCreateInfo.extent.width)
	{
		Debug::sendError("Requested width is too high, reducing to max capability");
		extent.width = swapChainSupport.capabilities.currentExtent.width;
	}
	if (swapChainSupport.capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max() && swapChainSupport.capabilities.currentExtent.height < swapChainCreateInfo.extent.height)
	{
		Debug::sendError("Requested height is too high, reducing to max capability");
		extent.height = swapChainSupport.capabilities.currentExtent.height;
	}

	const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapChainSupport.formats, swapChainCreateInfo.colorSpace, swapChainCreateInfo.format);
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

	Format wolfFormat = Format::UNDEFINED;
	switch (surfaceFormat.format)
	{
		case VK_FORMAT_B8G8R8A8_UNORM:
			wolfFormat = Format::B8G8R8A8_UNORM;
			break;
		case VK_FORMAT_R8G8B8A8_UNORM:
			wolfFormat = Format::R8G8B8A8_UNORM;
			break;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			wolfFormat = Format::R16G16B16A16_SFLOAT;
			break;
		default:
			Debug::sendError("Unhandled format");
	}
	for (size_t i(0); i < imageCount; ++i)
	{
		m_images[i].reset(new ImageVulkan(temporarySwapChainImages[i], wolfFormat, VK_IMAGE_ASPECT_COLOR_BIT, extent));
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

uint32_t Wolf::SwapChainVulkan::acquireNextImage(uint32_t currentFrame) const
{
	uint32_t imageIndex;
	const VkResult result = vkAcquireNextImageKHR(g_vulkanInstance->getDevice(), m_swapChain, std::numeric_limits<uint64_t>::max(), 
		static_cast<const SemaphoreVulkan*>(m_imageAvailableSemaphores[currentFrame % m_imageAvailableSemaphores.size()].get())->getSemaphore(), VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return NO_IMAGE_IDX;
	}
	else if (result != VK_SUCCESS)
	{
		Debug::sendCriticalError("Error : can't acquire image");
	}

	return imageIndex;
}

void Wolf::SwapChainVulkan::present(const Semaphore* waitSemaphore, uint32_t imageIndex) const
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	VkSemaphore semaphore = VK_NULL_HANDLE;
	if (waitSemaphore)
	{
		semaphore = static_cast<const SemaphoreVulkan*>(waitSemaphore)->getSemaphore();
		presentInfo.waitSemaphoreCount = 1;
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

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// It's most likely that window is not visible
	}
	else if (result != VK_SUCCESS)
		Debug::sendCriticalError("Can't present image");
}

void Wolf::SwapChainVulkan::resetAllFences()
{
	for (std::unique_ptr<Fence>& frameFence : m_frameFences)
	{
		frameFence->resetFence();
	}
}

void Wolf::SwapChainVulkan::recreate(Extent2D extent)
{
	vkDestroySwapchainKHR(g_vulkanInstance->getDevice(), m_swapChain, nullptr);
	m_images.clear();
	m_frameFences.clear();

	SwapChainCreateInfo swapChainCreateInfo{};
	swapChainCreateInfo.extent = extent;
	swapChainCreateInfo.colorSpace = m_colorSpace;
	swapChainCreateInfo.format = m_format;
	initialize(swapChainCreateInfo);
}
