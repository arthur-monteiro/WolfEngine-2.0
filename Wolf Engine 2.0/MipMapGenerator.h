#pragma once

#include <vector>
#include <vulkan/vulkan.h>

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
		static uint8_t mergeColor(uint8_t c00, uint8_t c01, uint8_t c10, uint8_t c11);

		struct RGBA8
		{
			uint8_t r, g, b, a;

			RGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {}
			static RGBA8 mergeBlock(const RGBA8& block00, const RGBA8& block01, const RGBA8& block10, const RGBA8& block11);
		};

		static void createMipLevel(const RGBA8* previousMip, RGBA8* currentMip, uint32_t width, uint32_t height);

		std::vector<std::vector<unsigned char>> m_mipLevels;
	};
}

