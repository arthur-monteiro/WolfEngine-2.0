#include "Buffer.h"

#include <Debug.h>

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/BufferVulkan.h"
#endif

Wolf::Buffer* Wolf::Buffer::createBuffer(uint64_t size, BufferUsageFlags usageFlags, uint32_t propertyFlags)
{
	if (size == 0)
	{
		Debug::sendCriticalError("Size must be more than 0 for buffer allocation");
	}

#ifdef WOLF_VULKAN
	return new BufferVulkan(size, usageFlags, propertyFlags);
#else
	return nullptr;
#endif
}
