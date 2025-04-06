#pragma once

#include <cstdint>

namespace Wolf
{
	struct Extent2D
	{
		uint32_t width;
		uint32_t height;
	};

	struct Extent3D
	{
		uint32_t width;
		uint32_t height;
		uint32_t depth;
	};

	struct Offset3D
	{
		int32_t x;
		int32_t y;
		int32_t z;
	};
}
