#include "MipMapGenerator.h"

#include <cmath>

Wolf::MipMapGenerator::MipMapGenerator(const unsigned char* firstMipPixels, VkExtent2D extent, VkFormat format, int mipCount)
{
	if(mipCount < 0)
		mipCount = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
	m_mipLevels.resize(mipCount - 1);

	const uint32_t pixelCount = extent.width * extent.height;

	for (uint32_t mipLevel = 1; mipLevel < static_cast<uint32_t>(mipCount); ++mipLevel)
	{
		if (format == VK_FORMAT_R8G8B8A8_UNORM)
		{
			RGBA8* previousMip = (RGBA8*)firstMipPixels;
			if (mipLevel > 1)
				previousMip = (RGBA8*)m_mipLevels[mipLevel - 2].data();

			m_mipLevels[mipLevel - 1].resize((pixelCount >> mipLevel) * sizeof(RGBA8));

			createMipLevel(previousMip, (RGBA8*)m_mipLevels[mipLevel - 1].data(), extent.width >> (mipLevel - 1), extent.height >> (mipLevel - 1));
		}
		else
		{
			Debug::sendError("Unsupported format while generating mipmaps");
			break;
		}
	}
}

uint8_t Wolf::MipMapGenerator::mergeColor(uint8_t c00, uint8_t c01, uint8_t c10, uint8_t c11)
{
	const float f00 = c00 / 255.0f;
	const float f01 = c01 / 255.0f;
	const float f10 = c10 / 255.0f;
	const float f11 = c11 / 255.0f;

	const float merged0 = (f00 + f01) / 2.0f;
	const float merged1 = (f10 + f11) / 2.0f;

	const float merged = (merged0 + merged1) / 2.0f;

	return static_cast<uint8_t>(merged * 255);
}

void Wolf::MipMapGenerator::createMipLevel(RGBA8* previousMip, RGBA8* currentMip, uint32_t width, uint32_t height)
{
	for (uint32_t x = 0; x < width; x += 2)
	{
		for (uint32_t y = 0; y < height; y += 2)
		{
			RGBA8& block00 = previousMip[x       + y * height];
			RGBA8& block01 = previousMip[x       + (y + 1) * height];
			RGBA8& block10 = previousMip[(x + 1) + y * height];
			RGBA8& block11 = previousMip[(x + 1) + (y + 1) * height];

			const RGBA8 mergedPixel = RGBA8::mergeBlock(block00, block01, block10, block11);
			currentMip[x / 2 + (y / 2) * (height / 2)] = mergedPixel;
		}
	}
}

Wolf::MipMapGenerator::RGBA8 Wolf::MipMapGenerator::RGBA8::mergeBlock(RGBA8& block00, RGBA8& block01, RGBA8& block10, RGBA8& block11)
{
	const uint8_t r = mergeColor(block00.r, block01.r, block10.r, block11.r);
	const uint8_t g = mergeColor(block00.g, block01.g, block10.g, block11.g);
	const uint8_t b = mergeColor(block00.b, block01.b, block10.b, block11.b);
	const uint8_t a = mergeColor(block00.a, block01.a, block10.a, block11.a);

	return RGBA8(r, g, b, a);
}
