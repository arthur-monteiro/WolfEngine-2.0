#pragma once

#include <vector>

#include <Extents.h>
#include <Formats.h>

#include "ImageCompression.h"

namespace Wolf
{
	class MipMapGenerator
	{
	public:
		static uint32_t computeMipCount(Extent2D extent);

		MipMapGenerator(const unsigned char* firstMipPixels, Extent2D extent, Format format, int mipCount = -1);
		MipMapGenerator(const MipMapGenerator&) = delete;

		uint32_t getMipLevelCount() const { return static_cast<uint32_t>(m_mipLevels.size()) + 1; }
		const std::vector<unsigned char>& getMipLevel(uint32_t mipLevel) const { return m_mipLevels[mipLevel - 1]; }

	private:
		static void createMipLevel(const ImageCompression::RGBA8* previousMip, ImageCompression::RGBA8* currentMip, uint32_t width, uint32_t height);
		static void createMipLevel(const ImageCompression::RG32F* previousMip, ImageCompression::RG32F* currentMip, uint32_t width, uint32_t height);

		std::vector<std::vector<unsigned char>> m_mipLevels;
	};
}

