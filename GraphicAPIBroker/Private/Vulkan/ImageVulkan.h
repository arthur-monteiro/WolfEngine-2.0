#pragma once

#include <array>
#include <unordered_map>
#include <string>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "../../Public/Image.h"

namespace Wolf
{
	class ImageVulkan : public Image
	{
	public:
		ImageVulkan(const CreateImageInfo& createImageInfo);
		ImageVulkan(VkImage image, Format format, ImageAspectFlags aspect, VkExtent2D extent);
		ImageVulkan(const ImageVulkan&) = delete;

		~ImageVulkan() override;

		void copyCPUBuffer(const unsigned char* pixels, const TransitionLayoutInfo& finalLayout, uint32_t mipLevel = 0, uint32_t baseArrayLayer = 0) override;
		void copyGPUBuffer(const Buffer& bufferSrc, const BufferImageCopy& copyRegion, const TransitionLayoutInfo& finalLayout) override;
		void recordCopyGPUBuffer(const CommandBuffer& commandBuffer, const Buffer& bufferSrc, const BufferImageCopy& copyRegion, const TransitionLayoutInfo& finalLayout) override;
		void copyGPUImage(const Image& imageSrc, const VkImageCopy& imageCopy) override;
		void recordCopyGPUImage(const Image& imageSrc, const VkImageCopy& imageCopy, const CommandBuffer& commandBuffer) override;

		[[nodiscard]] void* map() const override;
		void unmap() const override;
		void getResourceLayout(VkSubresourceLayout& output) const override;
		void exportToBuffer(std::vector<uint8_t>& outBuffer) const override;

		void setImageLayout(const TransitionLayoutInfo& transitionLayoutInfo) override;
		void transitionImageLayout(const CommandBuffer& commandBuffer, const TransitionLayoutInfo& transitionLayoutInfo) override;
		void setImageLayoutWithoutOperation(ImageLayout newImageLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = MAX_MIP_COUNT) override;

		[[nodiscard]] VkImage getImage() const { return m_image; }
		[[nodiscard]] VkDeviceMemory getImageMemory() const { return m_imageMemory; }
		[[nodiscard]] Format getFormat() const override { return m_imageFormat; }
		[[nodiscard]] SampleCountFlagBits getSampleCount() const override { return m_sampleCount; }
		[[nodiscard]] Extent3D getExtent() const override { return { m_extent.width, m_extent.height, m_extent.depth }; }
		[[nodiscard]] uint32_t getMipLevelCount() const override { return m_mipLevelCount; }
		ImageView getImageView(Format format) override;
		ImageView getDefaultImageView() override;

		[[nodiscard]] VkImageLayout getImageLayout(uint32_t mipLevel = 0, uint32_t layer = 0) const { return m_imageLayouts[layer][mipLevel]; }

	private:
		void createImageView(VkFormat format);
		float computeBPP() const;
		void setBPP();
		void resetAllLayouts();
		static VkImageUsageFlagBits wolfImageUsageFlagBitsToVkImageUsageFlagBits(ImageUsageFlagBits imageUsageFlagBits);
		static VkImageUsageFlags wolfImageUsageFlagsToVkImageUsageFlags(ImageUsageFlags imageUsageFlags);
		static VkBufferImageCopy wolfBufferImageCopyToVkBufferImageCopy(const BufferImageCopy& bufferImageCopy);
		static VkImageAspectFlagBits wolfImageAspectFlagBitsToVkImageAspectFlagBits(ImageAspectFlagBits imageAspectFlagBits);
		static VkImageAspectFlags wolfImageAspectFlagsToVkImageAspectFlags(ImageAspectFlags imageAspectFlags);

	private:
		VkImage m_image;
		VkDeviceMemory  m_imageMemory = VK_NULL_HANDLE;
		std::unordered_map<uint32_t, VkImageView> m_imageViews;

		std::vector<std::vector<VkImageLayout>> m_imageLayouts; // layer of mips
		VkAccessFlags m_accessFlags = 0;
		VkPipelineStageFlags m_pipelineStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		Format m_imageFormat;
		VkFormat m_vkImageFormat;

		uint32_t m_mipLevelCount;
		VkExtent3D m_extent;
		SampleCountFlagBits m_sampleCount;
		uint32_t m_arrayLayerCount;
		ImageAspectFlags m_aspectFlags;
		VkMemoryPropertyFlags m_memoryProperties = 0x0;

		VkDeviceSize m_allocationSize = 0;
	};
}
