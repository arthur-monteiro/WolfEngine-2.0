#include "GPUDataTransfersManager.h"

#include <Debug.h>

Wolf::DefaultGPUDataTransfersManager::DefaultGPUDataTransfersManager() : GPUDataTransfersManagerInterface()
{
}

void Wolf::DefaultGPUDataTransfersManager::pushDataToGPUBuffer(const void* data, uint32_t size, const ResourceNonOwner<Buffer>& outputBuffer, uint32_t outputOffset)
{
	outputBuffer->transferCPUMemoryWithStagingBuffer(data, size, 0, outputOffset);
}

void Wolf::DefaultGPUDataTransfersManager::fillGPUBuffer(uint32_t fillValue, uint32_t size, const ResourceNonOwner<Buffer>& outputBuffer, uint32_t outputOffset)
{
	Wolf::Debug::sendCriticalError("Not implemented");
}

void Wolf::DefaultGPUDataTransfersManager::pushDataToGPUImage(const PushDataToGPUImageInfo& pushDataToGPUImageInfo)
{
	pushDataToGPUImageInfo.m_outputImage->copyCPUBuffer(pushDataToGPUImageInfo.m_pixels, pushDataToGPUImageInfo.m_finalLayout, pushDataToGPUImageInfo.m_mipLevel);
}

void Wolf::DefaultGPUDataTransfersManager::requestGPUBufferReadbackRecord(const ResourceNonOwner<Buffer>& srcBuffer,uint32_t srcOffset, const ResourceNonOwner<ReadableBuffer>& readableBuffer, uint32_t size)
{
	Debug::sendCriticalError("Not implemented");
}
