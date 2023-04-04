#include "DescriptorSet.h"

#include "Configuration.h"

Wolf::DescriptorSet::DescriptorSet(VkDescriptorSetLayout descriptorSetLayout, UpdateRate updateRate)
{
	uint32_t bufferCount;
	switch (updateRate)
	{
	case Wolf::UpdateRate::EACH_FRAME:
		bufferCount = g_configuration->getMaxCachedFrames();
		break;
	case Wolf::UpdateRate::NEVER:
		bufferCount = 1;
		break;
	default:
		Debug::sendError("Not handled update rate");
		bufferCount = 1;
		break;
	}

	m_descriptorSets.resize(bufferCount);

	for (uint32_t idx = 0; idx < bufferCount; ++idx)
		createDescriptorSet(idx, descriptorSetLayout);
}

void Wolf::DescriptorSet::update(const DescriptorSetCreateInfo& descriptorSetCreateInfo)
{
	for (uint32_t i = 0; i < m_descriptorSets.size(); ++i)
		update(descriptorSetCreateInfo, i);
}

void Wolf::DescriptorSet::update(const DescriptorSetCreateInfo& descriptorSetCreateInfo, uint32_t idx)
{
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	std::vector<std::vector<VkDescriptorBufferInfo>> descriptorBufferInfos(descriptorSetCreateInfo.descriptorBuffers.size());
	for (int i(0); i < descriptorBufferInfos.size(); ++i)
	{
		const DescriptorSetCreateInfo::DescriptorBuffer& descriptorBuffer = descriptorSetCreateInfo.descriptorBuffers[i];
		descriptorBufferInfos[i].resize(descriptorBuffer.buffers.size());
		for (int j(0); j < descriptorBufferInfos[i].size(); ++j)
		{
			descriptorBufferInfos[i][j].buffer = descriptorBuffer.buffers[j]->getBuffer(idx);
			descriptorBufferInfos[i][j].offset = 0;
			descriptorBufferInfos[i][j].range = descriptorBuffer.buffers[j]->getBufferSize();
		}

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSets[idx];
		descriptorWrite.dstBinding = descriptorBuffer.descriptorLayout.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorBuffer.descriptorLayout.descriptorType;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(descriptorBufferInfos[i].size());
		descriptorWrite.pBufferInfo = descriptorBufferInfos[i].data();
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	std::vector<std::vector<VkDescriptorImageInfo>> descriptorImageInfos(descriptorSetCreateInfo.descriptorImages.size());
	for (int i(0); i < descriptorImageInfos.size(); ++i)
	{
		const DescriptorSetCreateInfo::DescriptorImage& descriptorImage = descriptorSetCreateInfo.descriptorImages[i];
		descriptorImageInfos[i].resize(descriptorImage.images.size());
		for (int j(0); j < descriptorImageInfos[i].size(); ++j)
		{
			if (descriptorImage.images[j].imageView)
			{
				descriptorImageInfos[i][j].imageLayout = descriptorImage.images[j].imageLayout;
				descriptorImageInfos[i][j].imageView = descriptorImage.images[j].imageView;
			}
			if (descriptorImage.images[j].sampler)
				descriptorImageInfos[i][j].sampler = descriptorImage.images[j].sampler;
		}

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSets[idx];
		descriptorWrite.dstBinding = descriptorImage.descriptorLayout.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorImage.descriptorLayout.descriptorType;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(descriptorImageInfos[i].size());
		descriptorWrite.pImageInfo = descriptorImageInfos[i].data();
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	for (int i(0); i < descriptorSetCreateInfo.descriptorAccelerationStructures.size(); ++i)
	{
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSets[idx];
		descriptorWrite.dstBinding = descriptorSetCreateInfo.descriptorAccelerationStructures[i].descriptorLayout.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorSetCreateInfo.descriptorAccelerationStructures[i].descriptorLayout.descriptorType;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(descriptorSetCreateInfo.descriptorAccelerationStructures.size());
		descriptorWrite.pImageInfo = NULL;
		descriptorWrite.pNext = &descriptorSetCreateInfo.descriptorAccelerationStructures[i].accelerationStructure;

		descriptorWrites.push_back(descriptorWrite);
	}

	vkUpdateDescriptorSets(g_vulkanInstance->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Wolf::DescriptorSet::createDescriptorSet(uint32_t idx, VkDescriptorSetLayout descriptorSetLayout)
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = g_vulkanInstance->getDescriptorPool();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(g_vulkanInstance->getDevice(), &allocInfo, &m_descriptorSets[idx]) != VK_SUCCESS)
		Debug::sendError("Error : allocate descriptor set");
}