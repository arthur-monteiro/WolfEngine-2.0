#pragma once
#include "Debug.h"

namespace Wolf
{
	enum class Format
	{
		UNDEFINED,

		R8_UINT,
		R32_UINT,
		R8G8B8A8_UNORM,
		R8G8B8A8_SRGB,
		B8G8R8A8_UNORM,

		R16_SFLOAT,
		R16G16_SFLOAT,
		R16G16B16_SFLOAT,
		R16G16B16A16_SFLOAT,
		R32_SFLOAT,
		R32G32_SFLOAT,
		R32G32B32_SFLOAT,
		R32G32B32A32_SFLOAT,
		R32G32B32A32_SINT,

		BC1_RGB_SRGB_BLOCK,
		BC1_RGBA_UNORM_BLOCK,
		BC3_SRGB_BLOCK,
		BC3_UNORM_BLOCK,
		BC5_UNORM_BLOCK,

		D32_SFLOAT,
		D32_SFLOAT_S8_UINT,
		D24_UNORM_S8_UINT
	};

	inline bool isSRGBFormat(Format format)
	{
		switch (format)
		{
			case Format::R8G8B8A8_SRGB:
			case Format::BC1_RGB_SRGB_BLOCK:
			case Format::BC3_SRGB_BLOCK:
				return true;

			case Format::UNDEFINED:
			case Format::R8_UINT:
			case Format::R32_UINT:
			case Format::R8G8B8A8_UNORM:
			case Format::B8G8R8A8_UNORM:
			case Format::R16_SFLOAT:
			case Format::R16G16B16_SFLOAT:
			case Format::R16G16B16A16_SFLOAT:
			case Format::R32_SFLOAT:
			case Format::R32G32_SFLOAT:
			case Format::R32G32B32_SFLOAT:
			case Format::R32G32B32A32_SFLOAT:
			case Format::BC1_RGBA_UNORM_BLOCK:
			case Format::BC3_UNORM_BLOCK:
			case Format::BC5_UNORM_BLOCK:
			case Format::D32_SFLOAT:
			case Format::D32_SFLOAT_S8_UINT:
			case Format::D24_UNORM_S8_UINT:
				return false;
			default:
				Wolf::Debug::sendError("Unhandled format");
				return false;
		}
	}

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
