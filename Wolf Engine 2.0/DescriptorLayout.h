#pragma once

#include <vulkan/vulkan_core.h>

namespace Wolf
{
	struct DescriptorLayout
	{
		VkDescriptorType descriptorType;
		VkShaderStageFlags accessibility{};
		uint32_t binding{};
		uint32_t count = 1;
	};
}