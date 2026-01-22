#pragma once

#include <glm/glm.hpp>

#include <ResourceNonOwner.h>

#include <Buffer.h>
#include <Image.h>

#include "ReadableBuffer.h"

namespace Wolf
{
	class GPUDataTransfersManagerInterface
	{
	public:
		virtual ~GPUDataTransfersManagerInterface() = default;

		virtual void pushDataToGPUBuffer(const void* data, uint32_t size, const ResourceNonOwner<Buffer>& outputBuffer, uint32_t outputOffset) = 0;
		virtual void fillGPUBuffer(uint32_t fillValue, uint32_t size, const ResourceNonOwner<Buffer>& outputBuffer, uint32_t outputOffset) = 0;

		struct PushDataToGPUImageInfo
		{
			const unsigned char* m_pixels;
			const ResourceNonOwner<Image> m_outputImage;
			const Image::TransitionLayoutInfo m_finalLayout;
			uint32_t m_mipLevel;
			glm::ivec2 m_copySize;
			glm::ivec2 m_imageOffset;

			PushDataToGPUImageInfo(const unsigned char* pixels, const ResourceNonOwner<Image>& outputImage, const Image::TransitionLayoutInfo& finalLayout, uint32_t mipLevel = 0,
			const glm::ivec2& copySize = glm::vec2(0, 0), const glm::ivec2& imageOffset = glm::vec2(0, 0)) :
				m_pixels{ pixels }, m_outputImage(outputImage), m_finalLayout(finalLayout), m_mipLevel(mipLevel), m_copySize(copySize), m_imageOffset(imageOffset)
			{}
		};
		virtual void pushDataToGPUImage(const PushDataToGPUImageInfo& pushDataToGPUImageInfo) = 0;

		virtual void requestGPUBufferReadbackRecord(const ResourceNonOwner<Buffer>& srcBuffer, uint32_t srcOffset, const ResourceNonOwner<ReadableBuffer>& readableBuffer, uint32_t size) = 0;

	protected:
		GPUDataTransfersManagerInterface() = default;
	};

	class DefaultGPUDataTransfersManager : public GPUDataTransfersManagerInterface
	{
	public:
		DefaultGPUDataTransfersManager();
		~DefaultGPUDataTransfersManager() override = default;

		void pushDataToGPUBuffer(const void* data, uint32_t size, const ResourceNonOwner<Buffer>& outputBuffer, uint32_t outputOffset) override;
		void fillGPUBuffer(uint32_t fillValue, uint32_t size, const ResourceNonOwner<Buffer>& outputBuffer, uint32_t outputOffset) override;
		void pushDataToGPUImage(const PushDataToGPUImageInfo& pushDataToGPUImageInfo) override;

		void requestGPUBufferReadbackRecord(const ResourceNonOwner<Buffer>& srcBuffer, uint32_t srcOffset, const ResourceNonOwner<ReadableBuffer>& readableBuffer, uint32_t size) override;
	};
}
