#pragma once

#include <cstdint>
#include <vector>

namespace Wolf
{
	class CommandBuffer;

	class Buffer
	{
	public:
		static Buffer* createBuffer(uint64_t size, uint32_t usageFlags, uint32_t propertyFlags);

		virtual ~Buffer() = default;

		virtual void transferCPUMemory(const void* data, uint64_t srcSize, uint64_t srcOffset = 0) const = 0;
		virtual void transferCPUMemoryWithStagingBuffer(const void* data, uint64_t srcSize, uint64_t srcOffset = 0, uint64_t dstOffset = 0) const = 0;

		typedef struct BufferCopy {
			uint64_t    srcOffset;
			uint64_t    dstOffset;
			uint64_t    size;
		} BufferCopy;
		virtual void transferGPUMemoryImmediate(const Buffer& bufferSrc, const BufferCopy& copyRegion) const = 0;
		virtual void recordTransferGPUMemory(CommandBuffer* commandBuffer, const Buffer& bufferSrc, const BufferCopy& copyRegion) const = 0;

		virtual void map(void** pData, uint64_t size = 0) const = 0;
		virtual void unmap() const = 0;
	};
}

