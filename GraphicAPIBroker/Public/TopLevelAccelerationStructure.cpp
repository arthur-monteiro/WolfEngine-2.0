#include "TopLevelAccelerationStructure.h"

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/TopLevelAccelerationStructureVulkan.h"
#endif

Wolf::TopLevelAccelerationStructure* Wolf::TopLevelAccelerationStructure::createTopLevelAccelerationStructure(
	std::span<BLASInstance> blasInstances)
{
#ifdef WOLF_VULKAN
	return new TopLevelAccelerationStructureVulkan(blasInstances);
#else
	return nullptr;
#endif
}

#endif