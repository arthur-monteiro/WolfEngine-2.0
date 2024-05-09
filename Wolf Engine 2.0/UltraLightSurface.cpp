#include "UltraLightSurface.h"

void* Wolf::UltraLightSurface::LockPixels()
{
	return m_image->map();
}

void Wolf::UltraLightSurface::UnlockPixels()
{
	m_image->unmap();
}

void Wolf::UltraLightSurface::Resize(uint32_t width, uint32_t height)
{
	CreateImageInfo createImageInfo;
	createImageInfo.extent = { width, height, 1 };
	createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	createImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	createImageInfo.mipLevelCount = 1;
	createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	createImageInfo.imageTiling = VK_IMAGE_TILING_LINEAR;
	createImageInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	m_image.reset(Image::createImage(createImageInfo));
	m_image->setImageLayout({ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT });
	m_image->getResourceLayout(m_imageResourceLayout);

	m_width = width;
	m_height = height;
	m_rowBytes = static_cast<uint32_t>(m_imageResourceLayout.rowPitch);
	m_size = static_cast<uint32_t>(m_imageResourceLayout.size);
}
