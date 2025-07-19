#pragma once

#include <vulkan/vulkan_core.h>

#include "ShaderStages.h"

namespace Wolf
{
	enum class DescriptorType
	{
		SAMPLER,
		UNIFORM_BUFFER,
		STORAGE_BUFFER,
		STORAGE_IMAGE,
		SAMPLED_IMAGE,
		COMBINED_IMAGE_SAMPLER,
		ACCELERATION_STRUCTURE
	};

	struct DescriptorLayout
	{
		DescriptorType descriptorType;
		ShaderStageFlags accessibility = 0;
		uint32_t binding{};
		uint32_t count = 1;
		uint32_t arrayIndex = 0;

		VkDescriptorBindingFlags bindingFlags = 0;
	};
}