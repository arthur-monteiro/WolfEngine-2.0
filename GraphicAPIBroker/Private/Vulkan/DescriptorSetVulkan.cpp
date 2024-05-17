#include "DescriptorSetVulkan.h"

#include <Debug.h>

#include "BufferVulkan.h"
#include "DescriptorSetLayoutVulkan.h"
#include "SamplerVulkan.h"
#include "TopLevelAccelerationStructureVulkan.h"
#include "Vulkan.h"

Wolf::DescriptorSetVulkan::DescriptorSetVulkan(const DescriptorSetLayout& descriptorSetLayout)
{
	initDescriptorSet(descriptorSetLayout);
}

void Wolf::DescriptorSetVulkan::update(const DescriptorSetUpdateInfo& descriptorSetCreateInfo) const
{
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	std::vector<std::vector<VkDescriptorBufferInfo>> descriptorBufferInfos(descriptorSetCreateInfo.descriptorBuffers.size());
	for (uint32_t i = 0; i < descriptorBufferInfos.size(); ++i)
	{
		const DescriptorSetUpdateInfo::DescriptorBuffer& descriptorBuffer = descriptorSetCreateInfo.descriptorBuffers[i];
		descriptorBufferInfos[i].resize(descriptorBuffer.buffers.size());
		for (uint32_t j = 0; j < descriptorBufferInfos[i].size(); ++j)
		{
			descriptorBufferInfos[i][j].buffer = static_cast<const BufferVulkan*>(descriptorBuffer.buffers[j])->getBuffer();
			descriptorBufferInfos[i][j].offset = 0;
			descriptorBufferInfos[i][j].range = static_cast<const BufferVulkan*>(descriptorBuffer.buffers[j])->getBufferSize();
		}

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSet;
		descriptorWrite.dstBinding = descriptorBuffer.descriptorLayout.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorBuffer.descriptorLayout.descriptorType;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(descriptorBufferInfos[i].size());
		descriptorWrite.pBufferInfo = descriptorBufferInfos[i].data();
		descriptorWrite.pNext = nullptr;

		descriptorWrites.push_back(descriptorWrite);
	}

	std::vector<std::vector<VkDescriptorImageInfo>> descriptorImageInfos(descriptorSetCreateInfo.descriptorImages.size());
	for (uint32_t i = 0; i < descriptorImageInfos.size(); ++i)
	{
		const DescriptorSetUpdateInfo::DescriptorImage& descriptorImage = descriptorSetCreateInfo.descriptorImages[i];
		descriptorImageInfos[i].resize(descriptorImage.images.size());
		for (uint32_t j = 0; j < descriptorImageInfos[i].size(); ++j)
		{
			if (descriptorImage.images[j].imageView)
			{
				descriptorImageInfos[i][j].imageLayout = descriptorImage.images[j].imageLayout;
				descriptorImageInfos[i][j].imageView = static_cast<VkImageView>(descriptorImage.images[j].imageView);
			}
			if (descriptorImage.images[j].sampler)
				descriptorImageInfos[i][j].sampler = static_cast<const SamplerVulkan*>(descriptorImage.images[j].sampler)->getSampler();
		}

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSet;
		descriptorWrite.dstBinding = descriptorImage.descriptorLayout.binding;
		descriptorWrite.dstArrayElement = descriptorImage.descriptorLayout.arrayIndex;
		descriptorWrite.descriptorType = descriptorImage.descriptorLayout.descriptorType;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(descriptorImageInfos[i].size());
		descriptorWrite.pImageInfo = descriptorImageInfos[i].data();
		descriptorWrite.pNext = nullptr;

		descriptorWrites.push_back(descriptorWrite);
	}

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30
	for (uint32_t i = 0; i < descriptorSetCreateInfo.descriptorAccelerationStructures.size(); ++i)
	{
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSet;
		descriptorWrite.dstBinding = descriptorSetCreateInfo.descriptorAccelerationStructures[i].descriptorLayout.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorSetCreateInfo.descriptorAccelerationStructures[i].descriptorLayout.descriptorType;
		descriptorWrite.descriptorCount = 1; // static_cast<uint32_t>(descriptorSetCreateInfo.descriptorAccelerationStructures.size());
		descriptorWrite.pImageInfo = nullptr;

		VkWriteDescriptorSetAccelerationStructureKHR descriptorSetInfo{ };
		descriptorSetInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptorSetInfo.accelerationStructureCount = 1;
		descriptorSetInfo.pAccelerationStructures = static_cast<const TopLevelAccelerationStructureVulkan*>(descriptorSetCreateInfo.descriptorAccelerationStructures[i].accelerationStructure)->getStructure();

		descriptorWrite.pNext = &descriptorSetInfo;

		descriptorWrites.push_back(descriptorWrite);
	}
#endif

	vkUpdateDescriptorSets(g_vulkanInstance->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Wolf::DescriptorSetVulkan::initDescriptorSet(const DescriptorSetLayout& descriptorSetLayout)
{
	const VkDescriptorSetLayout layouts[] = { static_cast<const DescriptorSetLayoutVulkan*>(&descriptorSetLayout)->getDescriptorSetLayout()};
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = g_vulkanInstance->getDescriptorPool();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(g_vulkanInstance->getDevice(), &allocInfo, &m_descriptorSet) != VK_SUCCESS)
		Debug::sendCriticalError("Allocate descriptor set");
}