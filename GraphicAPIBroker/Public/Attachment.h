#pragma once

#include <vulkan/vulkan_core.h>

#include "Extents.h"
#include "Image.h"
#include "Formats.h"
#include "ImageView.h"

namespace Wolf
{
	struct Attachment
	{
		Extent2D extent = { 0, 0 };
		Format format;
		VkSampleCountFlagBits sampleCount;
		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout finalLayout;
		VkAttachmentLoadOp loadOperation = VK_ATTACHMENT_LOAD_OP_CLEAR;
		VkAttachmentStoreOp storeOperation;
		ImageView imageView;

		ImageUsageFlags usageType;

		Attachment(Extent2D extent, Format format, VkSampleCountFlagBits sampleCount, VkImageLayout finalLayout,
			VkAttachmentStoreOp storeOperation, ImageUsageFlags usageType, ImageView imageView)
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
