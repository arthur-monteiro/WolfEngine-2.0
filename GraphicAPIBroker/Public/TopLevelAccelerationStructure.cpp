#include "TopLevelAccelerationStructure.h"

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/TopLevelAccelerationStructureVulkan.h"
#endif

Wolf::TopLevelAccelerationStructure* Wolf::TopLevelAccelerationStructure::createTopLevelAccelerationStructure(uint32_t maxInstanceCount)
{
#ifdef WOLF_VULKAN
	return new TopLevelAccelerationStructureVulkan(maxInstanceCount);
#else
	return nullptr;
#endif
}

#endif