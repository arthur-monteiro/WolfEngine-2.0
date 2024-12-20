#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "ImageCompression.h"

namespace Wolf
{
	class MipMapGenerator
	{
	public:
		MipMapGenerator(const unsigned char* firstMipPixels, VkExtent2D extent, VkFormat format, int mipCount = -1);
		MipMapGenerator(const MipMapGenerator&) = delete;

		uint32_t getMipLevelCount() const { return static_cast<uint32_t>(m_mipLevels.size()) + 1; }
		const std::vector<unsigned char>& getMipLevel(uint32_t mipLevel) const { return m_mipLevels[mipLevel - 1]; }

	private:
		static void createMipLevel(const ImageCompression::RGBA8* previousMip, ImageCompression::RGBA8* currentMip, uint32_t width, uint32_t height);
		static void createMipLevel(const ImageCompression::RG32F* previousMip, ImageCompression::RG32F* currentMip, uint32_t width, uint32_t height);

		std::vector<std::vector<unsigned char>> m_mipLevels;
	};
}

