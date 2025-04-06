#pragma once
#include <cstdint>

#include <ResourceNonOwner.h>

#include <Buffer.h>
#include <Image.h>

namespace Wolf
{
	void pushDataToGPUBuffer(const void* data, uint32_t size, const ResourceNonOwner<Buffer>& outputBuffer, uint32_t outputOffset);
	void pushDataToGPUImage(const unsigned char* pixels, const ResourceNonOwner<Image>& outputImage, const Image::TransitionLayoutInfo& finalLayout, uint32_t mipLevel = 0);
}
