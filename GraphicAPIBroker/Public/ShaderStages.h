#pragma once

#include <cstdint>

namespace Wolf
{
	enum ShaderStageFlagBits : uint32_t
	{
		VERTEX = 1 << 0,
		TESSELLATION_CONTROL = 1 << 1,
		TESSELLATION_EVALUATION = 1 << 2,
		GEOMETRY = 1 << 3,
		FRAGMENT = 1 << 4,
		COMPUTE = 1 << 5,
		ALL_GRAPHICS = 1 << 6,
		ALL = 1 << 7,
		RAYGEN = 1 << 8,
		ANY_HIT = 1 << 9,
		CLOSEST_HIT = 1 << 10,
		MISS = 1 << 11,
		TASK = 1 << 12,
		MESH = 1 << 13,
		SHADER_STAGE_MAX = 1 << 14
	};
	using ShaderStageFlags = uint32_t;
}
