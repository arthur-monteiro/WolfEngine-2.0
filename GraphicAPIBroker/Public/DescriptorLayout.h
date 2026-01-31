#pragma once

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

	enum DescriptorBindingFlagBits : uint32_t
	{
		DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT = 1 << 0,
		DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT = 1 << 1,
		DESCRIPTOR_BINDING_FLAG_MAX_BIT = 1 << 1,
	};
	using DescriptorBindingFlags = uint32_t;

	struct DescriptorLayout
	{
		DescriptorType descriptorType;
		ShaderStageFlags accessibility = 0;
		uint32_t binding{};
		uint32_t count = 1;
		uint32_t arrayIndex = 0;

		DescriptorBindingFlags bindingFlags = 0;
	};
}