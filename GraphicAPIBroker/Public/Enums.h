#pragma once

#include <cstdint>

namespace Wolf
{
	enum class BufferUsageFlag : uint32_t {  };

	enum class MemoryPropertyFlag : uint32_t {  };

	enum class PipelineStage {  };
	enum class IndexType { U16, U32 };

	enum class FragmentShadingRateCombinerOp { KEEP, REPLACE, MIN, MAX, MUL	};

	enum class CompareOp { NEVER, LESS, EQUAL, LESS_OR_EQUAL, GREATER, NOT_EQUAL, GREATER_OR_EQUAL, ALWAYS };
}
