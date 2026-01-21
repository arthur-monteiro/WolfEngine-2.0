#include "VulkanHelper.h"

#include <cstring>

#include <Debug.h>

#include "FormatsVulkan.h"

void Wolf::querySwapChainSupport(SwapChainSupportDetails& details, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
	}
}

uint32_t Wolf::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	uint32_t memoryType = -1;
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;

			if (memoryType != static_cast<uint32_t>(-1))
			{
				if (memProperties.memoryTypes[memProperties.memoryTypes[i].heapIndex].propertyFlags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) // prefer local over host
				{
					memoryType = i;
				}
			}
			else
			{
				memoryType = i;
			}
		}
	}

	return memoryType;
}

Wolf::Format Wolf::findDepthFormat(VkPhysicalDevice physicalDevice)
{
	return findSupportedFormat({ Format::D32_SFLOAT, Format::D32_SFLOAT_S8_UINT, Format::D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
		physicalDevice);
}

Wolf::Format Wolf::findSupportedFormat(const std::vector<Format>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice)
{
	for (const Format format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, wolfFormatToVkFormat(format), &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			return format;
	}

	Debug::sendError("Error : no format found");
	return Format::UNDEFINED;
}

bool Wolf::hasDepthComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool Wolf::hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkTransformMatrixKHR Wolf::toTransformMatrixKHR(const glm::mat4* matrix)
{
	glm::mat4 transposedMatrix = glm::transpose(glm::mat4(*matrix));

	VkTransformMatrixKHR outMatrix;
	memcpy(&outMatrix, &transposedMatrix, sizeof(VkTransformMatrixKHR));
	return outMatrix;
}
