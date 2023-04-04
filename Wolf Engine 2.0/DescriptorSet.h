#pragma once

#include "Buffer.h"
#include "DescriptorLayout.h"
#include "UpdateRate.h"
#include "Vulkan.h"

namespace Wolf
{
	struct DescriptorSetCreateInfo
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
			VkImageView imageView = VK_NULL_HANDLE;
			VkSampler sampler = VK_NULL_HANDLE;
		};
		struct DescriptorImage
		{
			std::vector<ImageData> images;
			DescriptorLayout descriptorLayout;
		};
		std::vector<DescriptorImage> descriptorImages;

		struct DescriptorAccelerationStructure
		{
			VkWriteDescriptorSetAccelerationStructureKHR accelerationStructure;
			DescriptorLayout descriptorLayout;
		};
		std::vector<DescriptorAccelerationStructure> descriptorAccelerationStructures;
	};

	class DescriptorSet
	{
	public:
		DescriptorSet(VkDescriptorSetLayout descriptorSetLayout, UpdateRate updateRate);
		DescriptorSet(const DescriptorSet&) = delete;

		void update(const DescriptorSetCreateInfo& descriptorSetCreateInfo);
		void update(const DescriptorSetCreateInfo& descriptorSetCreateInfo, uint32_t idx);

		const VkDescriptorSet* getDescriptorSet(uint32_t idx = 0) const { return &m_descriptorSets[idx]; }

	private:
		void createDescriptorSet(uint32_t idx, VkDescriptorSetLayout descriptorSetLayout);

		std::vector<VkDescriptorSet> m_descriptorSets;
	};
}

