#include "ReadbackDataFromGPU.h"

#include <Debug.h>

#ifndef CUSTOM_READBACK_DATA_FROM_GPU
void Wolf::requestGPUBufferReadbackRecord(const ResourceNonOwner<Buffer>& srcBuffer, uint32_t srcOffset, const ResourceNonOwner<ReadableBuffer>& readableBuffer, uint32_t size)
{
    Debug::sendCriticalError("Not implemented");
}
#endif