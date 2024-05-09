#include "DescriptorSet.h"

#ifdef WOLF_USE_VULKAN
#include "../Private/Vulkan/DescriptorSetVulkan.h"
#endif

Wolf::DescriptorSet* Wolf::DescriptorSet::createDescriptorSet(const DescriptorSetLayout& descriptorSetLayout)
{
#ifdef WOLF_USE_VULKAN
	return new DescriptorSetVulkan(descriptorSetLayout);
#else
	return nullptr;
#endif
}