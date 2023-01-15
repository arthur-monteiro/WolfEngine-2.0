#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

namespace Wolf
{
	class DescriptorPool
	{
	public:
		DescriptorPool(VkDevice device);
		DescriptorPool(const DescriptorPool&) = delete;
		~DescriptorPool();

		VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }

	private:
		static void addDescriptorPoolSize(VkDescriptorType descriptorType, uint32_t descriptorCount, std::vector<VkDescriptorPoolSize>& poolSizes);

	private:
		unsigned int m_uniformBufferCount = 2;
		unsigned int m_combinedImageSamplerCount = 2;
		unsigned int m_storageImageCount = 3;
		unsigned int m_samplerCount = 0;
		unsigned int m_sampledImageCount = 0;
		unsigned int m_storageBufferCount = 0;
		unsigned int m_accelerationStructureCount = 0;

		VkDescriptorPool m_descriptorPool;
	};
}