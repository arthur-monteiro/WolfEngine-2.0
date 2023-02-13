#pragma once

#include <vulkan/vulkan_core.h>

namespace Wolf
{
	struct Attachment
	{
		VkExtent2D extent = { 0, 0 };
		VkFormat format;
		VkSampleCountFlagBits sampleCount;
		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout finalLayout;
		VkAttachmentLoadOp loadOperation = VK_ATTACHMENT_LOAD_OP_CLEAR;
		VkAttachmentStoreOp storeOperation;
		VkImageView imageView;

		VkImageUsageFlags usageType{};

		Attachment(VkExtent2D extent, VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout finalLayout,
			VkAttachmentStoreOp storeOperation, VkImageUsageFlags usageType, VkImageView imageView)
		{
			this->extent = extent;
			this->format = format;
			this->sampleCount = sampleCount;
			this->finalLayout = finalLayout;
			this->storeOperation = storeOperation;
			this->usageType = usageType;
			this->imageView = imageView;
		}
		Attachment() = delete;
	};
}