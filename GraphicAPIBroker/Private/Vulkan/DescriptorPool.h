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

		[[nodiscard]] VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }

	private:
		static void addDescriptorPoolSize(VkDescriptorType descriptorType, uint32_t descriptorCount, std::vector<VkDescriptorPoolSize>& poolSizes);

	private:
		unsigned int m_uniformBufferCount = 20;
		unsigned int m_combinedImageSamplerCount = 10;
		unsigned int m_storageImageCount = 10;
		unsigned int m_samplerCount = 10;
		unsigned int m_sampledImageCount = 300;
		unsigned int m_storageBufferCount = 10;
		unsigned int m_accelerationStructureCount = 1;

		VkDescriptorPool m_descriptorPool;
	};
}