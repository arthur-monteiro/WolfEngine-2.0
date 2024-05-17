#include "BottomLevelAccelerationStructure.h"

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/BottomLevelAccelerationStructureVulkan.h"
#endif

Wolf::BottomLevelAccelerationStructure* Wolf::BottomLevelAccelerationStructure::createBottomLevelAccelerationStructure(const BottomLevelAccelerationStructureCreateInfo& createInfo)
{
#ifdef WOLF_VULKAN
	return new BottomLevelAccelerationStructureVulkan(createInfo);
#else
	return nullptr;
#endif
}

#endif