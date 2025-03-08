#include "MipMapGenerator.h"

#include <cmath>

#include "Debug.h"
#include "ImageCompression.h"

Wolf::MipMapGenerator::MipMapGenerator(const unsigned char* firstMipPixels, Extent2D extent, VkFormat format, int mipCount)
{
	if(mipCount < 0)
		mipCount = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) - 1; // remove 2 mip levels as min size must be 4x4

	if (mipCount < 0)
	{
		Debug::sendWarning("Can't compute mip levels");
		return;
	}

	m_mipLevels.resize(mipCount - 1);

	const uint32_t pixelCount = extent.width * extent.height;

	for (uint32_t mipLevel = 1; mipLevel < static_cast<uint32_t>(mipCount); ++mipLevel)
	{
		if (format == VK_FORMAT_R8G8B8A8_UNORM || format == VK_FORMAT_R8G8B8A8_SRGB)
		{
			const ImageCompression::RGBA8* previousMip = reinterpret_cast<const ImageCompression::RGBA8*>(firstMipPixels);
			if (mipLevel > 1)
				previousMip = reinterpret_cast<ImageCompression::RGBA8*>(m_mipLevels[mipLevel - 2].data());

			m_mipLevels[mipLevel - 1].resize((pixelCount >> mipLevel) * sizeof(ImageCompression::RGBA8));

			createMipLevel(previousMip, reinterpret_cast<ImageCompression::RGBA8*>(m_mipLevels[mipLevel - 1].data()), extent.width >> (mipLevel - 1), extent.height >> (mipLevel - 1));
		}
		else if (format == VK_FORMAT_R32G32_SFLOAT)
		{
			const ImageCompression::RG32F * previousMip = reinterpret_cast<const ImageCompression::RG32F*>(firstMipPixels);
			if (mipLevel > 1)
				previousMip = reinterpret_cast<ImageCompression::RG32F*>(m_mipLevels[mipLevel - 2].data());

			m_mipLevels[mipLevel - 1].resize((pixelCount >> mipLevel) * sizeof(ImageCompression::RG32F));

			createMipLevel(previousMip, reinterpret_cast<ImageCompression::RG32F*>(m_mipLevels[mipLevel - 1].data()), extent.width >> (mipLevel - 1), extent.height >> (mipLevel - 1));
		}
		else
		{
			Debug::sendError("Unsupported format while generating mipmaps");
			break;
		}
	}
}

void Wolf::MipMapGenerator::createMipLevel(const ImageCompression::RGBA8* previousMip, ImageCompression::RGBA8* currentMip, uint32_t width, uint32_t height)
{
	for (uint32_t x = 0; x < width; x += 2)
	{
		for (uint32_t y = 0; y < height; y += 2)
		{
			const ImageCompression::RGBA8& block00 = previousMip[x       + y * width];
			const ImageCompression::RGBA8& block01 = previousMip[x       + (y + 1) * width];
			const ImageCompression::RGBA8& block10 = previousMip[(x + 1) + y * width];
			const ImageCompression::RGBA8& block11 = previousMip[(x + 1) + (y + 1) * width];

			const ImageCompression::RGBA8 mergedPixel = ImageCompression::RGBA8::mergeBlock(block00, block01, block10, block11);
			currentMip[x / 2 + (y / 2) * (width / 2)] = mergedPixel;
		}
	}
}

void Wolf::MipMapGenerator::createMipLevel(const ImageCompression::RG32F* previousMip, ImageCompression::RG32F* currentMip, uint32_t width, uint32_t height)
{
	for (uint32_t x = 0; x < width; x += 2)
	{
		for (uint32_t y = 0; y < height; y += 2)
		{
			const ImageCompression::RG32F& block00 = previousMip[x + y * width];
			const ImageCompression::RG32F& block01 = previousMip[x + (y + 1) * width];
			const ImageCompression::RG32F& block10 = previousMip[(x + 1) + y * width];
			const ImageCompression::RG32F& block11 = previousMip[(x + 1) + (y + 1) * width];

			const ImageCompression::RG32F mergedPixel = ImageCompression::RG32F::mergeBlock(block00, block01, block10, block11);
			glm::vec3 mergedPixelAsVec = glm::vec3(mergedPixel.r, mergedPixel.g, glm::sqrt(1.0f - mergedPixel.r * mergedPixel.r - mergedPixel.g * mergedPixel.g));
			mergedPixelAsVec = glm::normalize(mergedPixelAsVec);

			currentMip[x / 2 + (y / 2) * (width / 2)] = ImageCompression::RG32F(mergedPixelAsVec.x, mergedPixelAsVec.y);
		}
	}
}
