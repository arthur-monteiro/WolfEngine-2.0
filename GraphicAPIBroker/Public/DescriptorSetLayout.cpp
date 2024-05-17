#include "DescriptorSetLayout.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/DescriptorSetLayoutVulkan.h"
#endif

Wolf::DescriptorSetLayout* Wolf::DescriptorSetLayout::createDescriptorSetLayout(
	const std::span<const DescriptorLayout> descriptorLayouts, uint32_t flags)
{
#ifdef WOLF_VULKAN
	return new DescriptorSetLayoutVulkan(descriptorLayouts, flags);
#else
	return nullptr;
#endif
}