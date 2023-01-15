#pragma once

#include <vulkan/vulkan_core.h>

namespace Wolf
{
	class Sampler
	{
	public:
		Sampler(VkSamplerAddressMode addressMode, float mipLevels, VkFilter filter, float maxAnisotropy = 16.0f, float minLod = 0.0f, float mipLodBias = 0.0f);
		Sampler(const Sampler&) = delete;
		~Sampler();

		VkSampler getSampler() const { return m_sampler; }

	private:
		VkSampler m_sampler;
	};
}
