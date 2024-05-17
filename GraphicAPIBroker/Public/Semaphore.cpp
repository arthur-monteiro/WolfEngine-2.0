#include "Semaphore.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/SemaphoreVulkan.h"
#endif

Wolf::Semaphore* Wolf::Semaphore::createSemaphore(uint32_t pipelineStage)
{
#ifdef WOLF_VULKAN
	return new SemaphoreVulkan(pipelineStage);
#else
	return nullptr;
#endif
}
