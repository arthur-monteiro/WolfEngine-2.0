#pragma once

#include "Vulkan.h"

namespace Wolf
{
	class MipMapGenerator
	{
	public:
		MipMapGenerator(const unsigned char* firstMipPixels, VkExtent2D extent, VkFormat format, int mipCount = -1);
		MipMapGenerator(const MipMapGenerator&) = delete;

		uint32_t getMipLevelCount() const { return static_cast<uint32_t>(m_mipLevels.size()) + 1; }
		const unsigned char* getMipLevel(uint32_t mipLevel) const { return m_mipLevels[mipLevel - 1].data(); }

	private:
		static uint8_t mergeColor(uint8_t c00, uint8_t c01, uint8_t c10, uint8_t c11);

		struct RGBA8
		{
			uint8_t r, g, b, a;

			RGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {}
			static RGBA8 mergeBlock(RGBA8& block00, RGBA8& block01, RGBA8& block10, RGBA8& block11);
		};
		void createMipLevel(RGBA8* previousMip, RGBA8* currentMip, uint32_t width, uint32_t height);

		std::vector<std::vector<unsigned char>> m_mipLevels;
	};
}

