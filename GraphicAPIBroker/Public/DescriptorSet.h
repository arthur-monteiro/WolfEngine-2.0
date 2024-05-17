#pragma once

#include <vector>

// TEMP
#include <vulkan/vulkan_core.h>

#include "DescriptorLayout.h"
#include "ImageView.h"

namespace Wolf
{
	class TopLevelAccelerationStructure;
	class DescriptorSetLayout;
	class Sampler;
	class Buffer;

	struct DescriptorSetUpdateInfo
	{
		struct DescriptorBuffer
		{
			std::vector<const Buffer*> buffers;
			DescriptorLayout descriptorLayout;
		};
		std::vector<DescriptorBuffer> descriptorBuffers;

		struct ImageData
		{
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			ImageView imageView = IMAGE_VIEW_NULL;
			const Sampler* sampler = nullptr;
		};
		struct DescriptorImage
		{
			std::vector<ImageData> images;
			DescriptorLayout descriptorLayout;
		};
		std::vector<DescriptorImage> descriptorImages;

		struct DescriptorAccelerationStructure
		{
			const TopLevelAccelerationStructure* accelerationStructure;
			DescriptorLayout descriptorLayout;
		};
		std::vector<DescriptorAccelerationStructure> descriptorAccelerationStructures;
	};

	class DescriptorSet
	{
	public:
		static DescriptorSet* createDescriptorSet(const DescriptorSetLayout& descriptorSetLayout);

		virtual ~DescriptorSet() = default;

		virtual void update(const DescriptorSetUpdateInfo& descriptorSetCreateInfo) const = 0;
	};
}

