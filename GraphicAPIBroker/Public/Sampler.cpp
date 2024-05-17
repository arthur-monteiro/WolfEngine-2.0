#include "Sampler.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/SamplerVulkan.h"
#endif

Wolf::Sampler* Wolf::Sampler::createSampler(VkSamplerAddressMode addressMode, float mipLevels, VkFilter filter,
	float maxAnisotropy, float minLod, float mipLodBias)
{
#ifdef WOLF_VULKAN
	return new SamplerVulkan(addressMode, mipLevels, filter, maxAnisotropy, minLod, mipLodBias);
#else
	return nullptr;
#endif
}
