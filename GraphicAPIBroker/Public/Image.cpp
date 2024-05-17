#include "Image.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/ImageVulkan.h"
#endif

Wolf::Image* Wolf::Image::createImage(const CreateImageInfo& createImageInfo)
{
#ifdef WOLF_VULKAN
	return new ImageVulkan(createImageInfo);
#else
	return nullptr;
#endif
}