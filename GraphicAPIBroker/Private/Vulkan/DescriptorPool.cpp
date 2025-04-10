#include "DescriptorPool.h"

#include <Debug.h>

#include "Vulkan.h"

Wolf::DescriptorPool::DescriptorPool(VkDevice device)
{
	std::vector<VkDescriptorPoolSize> poolSizes{};
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_uniformBufferCount, poolSizes);
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_combinedImageSamplerCount, poolSizes);
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_storageImageCount, poolSizes);
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, m_samplerCount, poolSizes);
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_sampledImageCount, poolSizes);
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_storageBufferCount, poolSizes);
#ifndef __ANDROID__
	addDescriptorPoolSize(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, m_accelerationStructureCount, poolSizes);
#endif

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 512;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		Debug::sendError("Error : create descriptor pool");
}

Wolf::DescriptorPool::~DescriptorPool()
{
	vkDestroyDescriptorPool(g_vulkanInstance->getDevice(), m_descriptorPool, nullptr);
}

void Wolf::DescriptorPool::addDescriptorPoolSize(VkDescriptorType descriptorType, uint32_t descriptorCount, std::vector<VkDescriptorPoolSize>& poolSizes)
{
	if (descriptorCount > 0)
	{
		VkDescriptorPoolSize poolSize;
		poolSize.type = descriptorType;
		poolSize.descriptorCount = descriptorCount;

		poolSizes.emplace_back(poolSize);
	}
}