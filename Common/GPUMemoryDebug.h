#pragma once

#include <cstdint>

namespace Wolf
{
	class GPUMemoryDebug
	{
	public:
		enum class TYPE { BUFFER, TEXTURE };
		enum class FLAG_VALUE : uint32_t { TRANSIENT = 1 };

		static void registerNewResource(TYPE type, uint32_t flags, uint64_t requestedSize, uint64_t memorySize)
		{
			ms_totalMemoryRequested += requestedSize;
			ms_totalMemoryUsed += memorySize;
		}
		static void unregisterResource(TYPE type, uint32_t flags, uint64_t requestedSize, uint64_t memorySize)
		{
			ms_totalMemoryRequested -= requestedSize;
			ms_totalMemoryUsed -= memorySize;
		}

		static uint64_t getTotalMemoryRequested() { return ms_totalMemoryRequested; }
		static uint64_t getTotalMemoryUsed() { return ms_totalMemoryUsed; }

	private:
		inline static uint64_t ms_totalMemoryRequested = 0;
		inline static uint64_t ms_totalMemoryUsed = 0;
	};
}
