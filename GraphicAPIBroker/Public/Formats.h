#pragma once

namespace Wolf
{
	enum class Format
	{
		UNDEFINED,

		R8_UINT,
		R8G8B8A8_UNORM,
		R8G8B8A8_SRGB,
		B8G8R8A8_UNORM,

		R32G32_SFLOAT,
		R32G32B32_SFLOAT,

		BC1_RGB_SRGB_BLOCK,
		BC1_RGBA_UNORM_BLOCK,
		BC3_SRGB_BLOCK,
		BC3_UNORM_BLOCK,
		BC5_UNORM_BLOCK,

		D32_SFLOAT,
		D32_SFLOAT_S8_UINT,
		D24_UNORM_S8_UINT
	};


	enum SampleCountFlagBits : uint32_t
	{
		SAMPLE_COUNT_1 = 1 << 0,
		SAMPLE_COUNT_2 = 1 << 1,
		SAMPLE_COUNT_4 = 1 << 2,
		SAMPLE_COUNT_8 = 1 << 3,
		SAMPLE_COUNT_16 = 1 << 4,
		SAMPLE_COUNT_32 = 1 << 5,
		SAMPLE_COUNT_64 = 1 << 6,
		SAMPLE_COUNT_MAX = 1 << 7
	};
	using SampleCountFlags = uint32_t;
}
