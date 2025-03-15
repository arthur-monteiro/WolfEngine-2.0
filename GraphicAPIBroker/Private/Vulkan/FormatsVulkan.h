#pragma once

#include <vulkan/vulkan_core.h>

#include "../../Public/Formats.h"

namespace Wolf
{
	inline VkFormat wolfFormatToVkFormat(Format imageFormat)
	{
		switch (imageFormat) 
		{
			case Format::R8_UINT:
				return VK_FORMAT_R8_UINT;
			case Format::R8G8B8A8_UNORM:
				return VK_FORMAT_R8G8B8A8_UNORM;
			case Format::R8G8B8A8_SRGB:
				return VK_FORMAT_R8G8B8A8_SRGB;
			case Format::B8G8R8A8_UNORM:
				return VK_FORMAT_B8G8R8A8_UNORM;
			case Format::R32G32_SFLOAT:
				return VK_FORMAT_R32G32_SFLOAT;
			case Format::R32G32B32_SFLOAT:
				return VK_FORMAT_R32G32B32_SFLOAT;
			case Format::BC1_RGB_SRGB_BLOCK:
				return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
			case Format::BC1_RGBA_UNORM_BLOCK:
				return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			case Format::BC3_SRGB_BLOCK:
				return VK_FORMAT_BC3_SRGB_BLOCK;
			case Format::BC3_UNORM_BLOCK:
				return VK_FORMAT_BC3_UNORM_BLOCK;
			case Format::BC5_UNORM_BLOCK:
				return VK_FORMAT_BC5_UNORM_BLOCK;
			case Format::D32_SFLOAT:
				return VK_FORMAT_D32_SFLOAT;
			case Format::D32_SFLOAT_S8_UINT:
				return VK_FORMAT_D32_SFLOAT_S8_UINT;
			case Format::D24_UNORM_S8_UINT:
				return VK_FORMAT_D24_UNORM_S8_UINT;
			case Format::UNDEFINED:
			default:
				return VK_FORMAT_UNDEFINED;
		}
	}
}
