#pragma once

#include <cstdint>

namespace Wolf
{
	union ClearColorValue
	{
		float       float32[4];
		int32_t     int32[4];
		uint32_t    uint32[4];
	};
	struct ClearDepthStencilValue {
		float       depth;
		uint32_t    stencil;
	};
	union ClearValue
	{
		ClearColorValue           color;
		ClearDepthStencilValue    depthStencil;
	};
}
