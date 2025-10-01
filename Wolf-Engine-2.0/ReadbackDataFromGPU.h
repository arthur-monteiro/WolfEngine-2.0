#pragma once

#include <Buffer.h>
#include <ReadableBuffer.h>

namespace Wolf
{
    void requestGPUBufferReadbackRecord(const ResourceNonOwner<Buffer>& srcBuffer, uint32_t srcOffset, const ResourceNonOwner<ReadableBuffer>& readableBuffer, uint32_t size);
}
