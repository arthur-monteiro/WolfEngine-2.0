#include "Buffer.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/BufferVulkan.h"
#endif

Wolf::Buffer* Wolf::Buffer::createBuffer(uint64_t size, uint32_t usageFlags, uint32_t propertyFlags)
{
#ifdef WOLF_VULKAN
	return new BufferVulkan(size, usageFlags, propertyFlags);
#else
	return nullptr;
#endif
}
