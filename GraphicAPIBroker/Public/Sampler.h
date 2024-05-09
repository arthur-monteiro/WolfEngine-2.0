#pragma once

// TEMP
#include <vulkan/vulkan_core.h>

namespace Wolf
{
	class Sampler
	{
	public:
		static Sampler* createSampler(VkSamplerAddressMode addressMode, float mipLevels, VkFilter filter, float maxAnisotropy = 16.0f, float minLod = 0.0f, float mipLodBias = 0.0f);

		virtual ~Sampler() = default;
	};
}
