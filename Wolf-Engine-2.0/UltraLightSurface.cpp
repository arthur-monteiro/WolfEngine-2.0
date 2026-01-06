#include "UltraLightSurface.h"

#include "ProfilerCommon.h"

void Wolf::UltraLightSurface::beforeRender() const
{
	PROFILE_FUNCTION

	// This is too costly
	/*const void* srcImage = m_images[getReadyImageIdx()]->map();
	void* dstImage = m_images[m_currentImageIdx]->map();

	memcpy(dstImage, srcImage, m_imageResourceLayout.size);

	m_images[getReadyImageIdx()]->unmap();
	m_images[m_currentImageIdx]->unmap();*/
}

void Wolf::UltraLightSurface::moveToNextFrame()
{
	m_currentImageIdx = (m_currentImageIdx + 1) % IMAGE_COUNT;
}

void* Wolf::UltraLightSurface::LockPixels()
{
	return m_images[m_currentImageIdx]->map();
}

void Wolf::UltraLightSurface::UnlockPixels()
{
	m_images[m_currentImageIdx]->unmap();
}

void Wolf::UltraLightSurface::Resize(uint32_t width, uint32_t height)
{
	CreateImageInfo createImageInfo;
	createImageInfo.extent = { width, height, 1 };
	createImageInfo.aspectFlags = ImageAspectFlagBits::COLOR;
	createImageInfo.format = Format::R8G8B8A8_UNORM;
	createImageInfo.mipLevelCount = 1;
	createImageInfo.usage = ImageUsageFlagBits::TRANSFER_SRC;
	createImageInfo.imageTiling = VK_IMAGE_TILING_LINEAR;
	createImageInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	for (ResourceUniqueOwner<Image>& image : m_images)
	{
		image.reset(Image::createImage(createImageInfo));
		image->setImageLayout({ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT });
		image->getResourceLayout(m_imageResourceLayout);
	}

	m_width = width;
	m_height = height;
	m_rowBytes = static_cast<uint32_t>(m_imageResourceLayout.rowPitch);
	m_size = static_cast<uint32_t>(m_imageResourceLayout.size);
}
