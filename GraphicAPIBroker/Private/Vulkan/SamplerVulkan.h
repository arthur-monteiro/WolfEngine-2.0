#pragma once

#include <vulkan/vulkan.h>

#include "../../Public/Sampler.h"

namespace Wolf
{
	class SamplerVulkan : public Sampler
	{
	public:
		SamplerVulkan(VkSamplerAddressMode addressMode, float mipLevels, VkFilter filter, float maxAnisotropy = 16.0f, float minLod = 0.0f, float mipLodBias = 0.0f);
		SamplerVulkan(const SamplerVulkan&) = delete;
		~SamplerVulkan() override;

		[[nodiscard]] VkSampler getSampler() const { return m_sampler; }

	private:
		VkSampler m_sampler;
	};
}
