#include "PushDataToGPU.h"

#ifndef CUSTOM_PUSH_DATA_TO_GPU
void Wolf::pushDataToGPUBuffer(const void* data, uint32_t size, const ResourceNonOwner<Buffer>& outputBuffer, uint32_t outputOffset)
{
	outputBuffer->transferCPUMemoryWithStagingBuffer(data, size, 0, outputOffset);
}

void Wolf::pushDataToGPUImage(const unsigned char* pixels, const ResourceNonOwner<Image>& outputImage, const Image::TransitionLayoutInfo& finalLayout, uint32_t mipLevel)
{
	outputImage->copyCPUBuffer(pixels, finalLayout, mipLevel);
}
#endif
