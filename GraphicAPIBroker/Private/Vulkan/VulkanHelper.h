#pragma once

#include <glm/glm.hpp>

#include "SwapChainSupportDetails.h"

namespace Wolf
{
	void querySwapChainSupport(SwapChainSupportDetails& details, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice);
	bool hasDepthComponent(VkFormat format);
	bool hasStencilComponent(VkFormat format);
	VkTransformMatrixKHR toTransformMatrixKHR(const glm::mat4* matrix);
}