#include "PushDataToGPU.h"

#include <Debug.h>

#ifndef CUSTOM_PUSH_DATA_TO_GPU
void Wolf::pushDataToGPUBuffer(const void* data, uint32_t size, const ResourceNonOwner<Buffer>& outputBuffer, uint32_t outputOffset)
{
	outputBuffer->transferCPUMemoryWithStagingBuffer(data, size, 0, outputOffset);
}

void Wolf::fillGPUBuffer(uint32_t fillValue, uint32_t size, const ResourceNonOwner<Buffer>& outputBuffer, uint32_t outputOffset)
{
	Wolf::Debug::sendCriticalError("Not implemented");
}

void Wolf::pushDataToGPUImage(const unsigned char* pixels, const ResourceNonOwner<Image>& outputImage, const Image::TransitionLayoutInfo& finalLayout, uint32_t mipLevel,
	const glm::ivec2& copySize, const glm::ivec2& imageOffset)
{
	outputImage->copyCPUBuffer(pixels, finalLayout, mipLevel);
}
#endif
