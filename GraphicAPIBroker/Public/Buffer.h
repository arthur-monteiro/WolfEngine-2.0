#pragma once

#include "AccessFlags.h"
#include "Enums.h"

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

		struct BufferCopy
		{
			uint64_t srcOffset;
			uint64_t dstOffset;
			uint64_t size;
		};
		virtual void transferGPUMemoryImmediate(const Buffer& bufferSrc, const BufferCopy& copyRegion) const = 0;
		virtual void recordTransferGPUMemory(const CommandBuffer* commandBuffer, const Buffer& bufferSrc, const BufferCopy& copyRegion) const = 0;

		struct BufferFill
		{
			uint64_t dstOffset;
			uint64_t size;
			uint32_t data;
		};
		virtual void recordFillBuffer(const CommandBuffer* commandBuffer, const BufferFill& bufferFill) const = 0;

		struct BufferAccess
		{
			PipelineStage stage;
			AccessFlags accessFlags;
		};
		virtual void recordBarrier(const CommandBuffer* commandBuffer, const BufferAccess& accessBefore, const BufferAccess& accessAfter) const = 0;

		[[nodiscard]] virtual void* map(uint64_t size = 0) const = 0;
		[[nodiscard]] virtual void unmap() const = 0;

		virtual uint32_t getSize() const = 0;
	};
}

