#pragma once

#include <array>
#include <string>
#include <unordered_map>

// TEMP
#include <vulkan/vulkan_core.h>

#include "Extents.h"
#include "Formats.h"
#include "ImageView.h"

namespace Wolf
{
	class CommandBuffer;
	class Buffer;

	enum ImageUsageFlagBits : uint32_t
	{
		SAMPLED = 1 << 0,
		TRANSFER_SRC = 1 << 1,
		TRANSFER_DST = 1 << 2,
		DEPTH_STENCIL_ATTACHMENT = 1 << 3,
		COLOR_ATTACHMENT = 1 << 4,
		TRANSIENT_ATTACHMENT = 1 << 5,
		FRAGMENT_SHADING_RATE_ATTACHMENT = 1 << 6,
		STORAGE = 1 << 7,
		IMAGE_USAGE_MAX = 1 << 8
	};
	using ImageUsageFlags = uint32_t;

	struct CreateImageInfo
	{
		Extent3D extent = { 0, 0, 0 };
		ImageUsageFlags usage = ImageUsageFlagBits::SAMPLED;
		Format format = Format::UNDEFINED;
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
#define MAX_MIP_COUNT UINT32_MAX
		uint32_t mipLevelCount = MAX_MIP_COUNT;
		uint32_t arrayLayerCount = 1;
		VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL;
	};

	class Image
	{
	public:
		static Image* createImage(const CreateImageInfo& createImageInfo);

		virtual ~Image() = default;

		struct TransitionLayoutInfo
		{
			VkImageLayout dstLayout;
			VkAccessFlags dstAccessMask;
			VkPipelineStageFlags dstPipelineStageFlags;
			uint32_t baseMipLevel = 0;
			uint32_t levelCount = MAX_MIP_COUNT;
			VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		};

		virtual void copyCPUBuffer(const unsigned char* pixels, const TransitionLayoutInfo& finalLayout, uint32_t mipLevel = 0) = 0;
		virtual void copyGPUBuffer(const Buffer& bufferSrc, const VkBufferImageCopy& copyRegion, const TransitionLayoutInfo& finalLayout) = 0;
		virtual void copyGPUImage(const Image& imageSrc, const VkImageCopy& imageCopy) = 0;
		virtual void recordCopyGPUImage(const Image& imageSrc, const VkImageCopy& imageCopy, const CommandBuffer& commandBuffer) = 0;

		[[nodiscard]] virtual void* map() const = 0;
		virtual void unmap() const = 0;
		virtual void getResourceLayout(VkSubresourceLayout& output) const = 0;
		virtual void exportToBuffer(std::vector<uint8_t>& outBuffer) const = 0;

		static constexpr TransitionLayoutInfo SampledInFragmentShader(uint32_t mipLevel = 0, VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED)
		{
			return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, mipLevel, 1, oldLayout };
		}
		virtual void setImageLayout(const TransitionLayoutInfo& transitionLayoutInfo) = 0;
		virtual void transitionImageLayout(const CommandBuffer& commandBuffer, const TransitionLayoutInfo& transitionLayoutInfo) = 0;
		virtual void setImageLayoutWithoutOperation(VkImageLayout newImageLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = MAX_MIP_COUNT) = 0;

		float getBPP() const { return m_bpp; }

		virtual ImageView getImageView(Format format) = 0;
		virtual ImageView getDefaultImageView() = 0;

		[[nodiscard]] virtual Format getFormat() const = 0;
		[[nodiscard]] virtual VkSampleCountFlagBits getSampleCount() const = 0;
		[[nodiscard]] virtual Extent3D getExtent() const = 0;
		[[nodiscard]] virtual VkImageLayout getImageLayout(uint32_t mipLevel = 0) const = 0;
		[[nodiscard]] virtual uint32_t getMipLevelCount() const = 0;

	protected:
		float m_bpp = 0.0f;
	};
}
