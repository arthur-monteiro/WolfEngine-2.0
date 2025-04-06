#pragma once

#include <vulkan/vulkan_core.h>

#include "Extents.h"
#include "Image.h"
#include "Formats.h"
#include "ImageView.h"

namespace Wolf
{
	enum class AttachmentLoadOp
	{
		LOAD = 0,
		CLEAR = 1,
		DONT_CARE = 2
	};

	enum class AttachmentStoreOp
	{
		STORE = 0,
		DONT_CARE = 1,
		NONE = 2,
	};

	struct Attachment
	{
		Extent2D extent = { 0, 0 };
		Format format;
		SampleCountFlagBits sampleCount;
		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout finalLayout;
		AttachmentLoadOp loadOperation = AttachmentLoadOp::CLEAR;
		AttachmentStoreOp storeOperation;
		ImageView imageView;

		ImageUsageFlags usageType;

		Attachment(Extent2D extent, Format format, SampleCountFlagBits sampleCount, VkImageLayout finalLayout,
			AttachmentStoreOp storeOperation, ImageUsageFlags usageType, ImageView imageView)
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
