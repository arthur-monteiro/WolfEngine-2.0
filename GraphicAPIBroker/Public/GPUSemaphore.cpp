#include "GPUSemaphore.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/SemaphoreVulkan.h"
#endif

Wolf::Semaphore* Wolf::Semaphore::createSemaphore(uint32_t pipelineStage, Type type)
{
#ifdef WOLF_VULKAN
	return new SemaphoreVulkan(pipelineStage, type);
#else
	return nullptr;
#endif
}
