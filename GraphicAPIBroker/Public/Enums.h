#pragma once

#include <cstdint>

namespace Wolf
{
	enum class BufferUsageFlag : uint32_t {  };

	enum class MemoryPropertyFlag : uint32_t {  };

	enum class PipelineStage {  };
	enum class IndexType { U16, U32 };

	enum class FragmentShadingRateCombinerOp { KEEP, REPLACE, MIN, MAX, MUL	};
}
