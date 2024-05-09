#include "TopLevelAccelerationStructure.h"

#ifdef WOLF_USE_VULKAN
#include "../Private/Vulkan/TopLevelAccelerationStructureVulkan.h"
#endif

Wolf::TopLevelAccelerationStructure* Wolf::TopLevelAccelerationStructure::createTopLevelAccelerationStructure(
	std::span<BLASInstance> blasInstances)
{
#ifdef WOLF_USE_VULKAN
	return new TopLevelAccelerationStructureVulkan(blasInstances);
#else
	return nullptr;
#endif
}
