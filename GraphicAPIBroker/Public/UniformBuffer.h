#pragma once

#include <cstdint>
#include <vector>

#include <ResourceUniqueOwner.h>

#include "Buffer.h"

namespace Wolf
{
	class UniformBuffer
	{
	public:
		explicit UniformBuffer(uint64_t size);

		void transferCPUMemory(const void* data, uint64_t srcSize, uint64_t srcOffset = 0) const;
		[[nodiscard]] const Buffer& getBuffer(uint32_t idx) const { return *m_buffers[idx]; }

	private:
		std::vector<ResourceUniqueOwner<Buffer>> m_buffers;
	};
}

