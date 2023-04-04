#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
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
	VkTransformMatrixKHR toTransformMatrixKHR(glm::mat4 matrix);
}