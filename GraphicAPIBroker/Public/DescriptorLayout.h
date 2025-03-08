#pragma once

#include <vulkan/vulkan_core.h>

#include "ShaderStages.h"

namespace Wolf
{
	struct DescriptorLayout
	{
		VkDescriptorType descriptorType;
		ShaderStageFlags accessibility = 0;
		uint32_t binding{};
		uint32_t count = 1;
		uint32_t arrayIndex = 0;

		VkDescriptorBindingFlags bindingFlags = 0;
	};
}