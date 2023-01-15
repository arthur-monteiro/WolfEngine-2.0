#pragma once

#include <cstdint>

struct HardwareCapabilities
{
	bool rayTracingAvailable = false;
	bool meshShaderAvailable = false;
	uint64_t VRAMSize = 0;
};