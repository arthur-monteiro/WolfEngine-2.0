#pragma once

#include <glm/glm.hpp>

#include <ResourceNonOwner.h>

#include <Buffer.h>
#include <Image.h>

namespace Wolf
{
	void pushDataToGPUBuffer(const void* data, uint32_t size, const ResourceNonOwner<Buffer>& outputBuffer, uint32_t outputOffset);
	void fillGPUBuffer(uint32_t fillValue, uint32_t size, const ResourceNonOwner<Buffer>& outputBuffer, uint32_t outputOffset);
	void pushDataToGPUImage(const unsigned char* pixels, const ResourceNonOwner<Image>& outputImage, const Image::TransitionLayoutInfo& finalLayout, uint32_t mipLevel = 0,
		const glm::ivec2& copySize = glm::vec2(0, 0), const glm::ivec2& imageOffset = glm::vec2(0, 0));
}
