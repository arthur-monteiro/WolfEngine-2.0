#pragma once

#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <unordered_map>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Buffer.h"
#include "Vulkan.h"

namespace Wolf
{
	struct CreateImageInfo
	{
		VkExtent3D extent = { 0, 0, 0 };
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		VkFormat format = VK_FORMAT_UNDEFINED;
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
		Image(const CreateImageInfo& createImageInfo);
		Image(VkImage image, VkFormat format, VkImageAspectFlags aspect, VkExtent2D extent);
		Image(const Image&) = delete;

		~Image();

		void copyCPUBuffer(const unsigned char* pixels, uint32_t mipLevel = 0);
		void copyGPUBuffer(const Buffer& bufferSrc, const VkBufferImageCopy& copyRegion);
		void copyImagesToCubemap(std::array<Image*, 6> images, std::vector<std::pair<uint8_t, uint8_t>> mipsToCopy, bool generateMipsLevels);

		[[nodiscard]] void* map() const;
		void unmap() const;
		void getResourceLayout(VkSubresourceLayout& output) const;

		void setImageLayout(VkImageLayout dstLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstPipelineStageFlags);
		void transitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout dstLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstPipelineStageFlags, uint32_t baseMipLevel = 0, uint32_t levelCount = MAX_MIP_COUNT);
		void setImageLayoutWithoutOperation(VkImageLayout newImageLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = MAX_MIP_COUNT);

		[[nodiscard]] VkImage getImage() const { return m_image; }
		[[nodiscard]] VkDeviceMemory getImageMemory() const { return m_imageMemory; }
		[[nodiscard]] VkFormat getFormat() const { return m_imageFormat; }
		[[nodiscard]] VkSampleCountFlagBits getSampleCount() const { return m_sampleCount; }
		[[nodiscard]] VkExtent3D getExtent() const { return m_extent; }
		[[nodiscard]] VkImageLayout getImageLayout(uint32_t mipLevel = 0) const { return m_imageLayouts[mipLevel]; }
		[[nodiscard]] uint32_t getMipLevelCount() const { return m_mipLevelCount; }
		VkImageView getImageView(VkFormat format);
		VkImageView getDefaultImageView();
		uint32_t getBBP() const { return m_bbp; }

	private:
		void createImageView(VkFormat format);
		void setBBP();

	private:
		VkImage m_image;
		VkDeviceMemory  m_imageMemory = VK_NULL_HANDLE;
		std::unordered_map<uint32_t, VkImageView> m_imageViews;

		std::vector<VkImageLayout> m_imageLayouts;
		VkAccessFlags m_accessFlags = 0;
		VkPipelineStageFlags m_pipelineStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkFormat m_imageFormat;

		uint32_t m_mipLevelCount;
		VkExtent3D m_extent;
		VkSampleCountFlagBits m_sampleCount;
		uint32_t m_arrayLayerCount;
		uint32_t m_bbp;
		VkImageAspectFlags m_aspectFlags;
	};
}
