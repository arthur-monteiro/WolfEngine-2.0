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
		unsigned int m_uniformBufferCount = 8192;
		unsigned int m_combinedImageSamplerCount = 128;
		unsigned int m_storageImageCount = 512;
		unsigned int m_samplerCount = 64;
		unsigned int m_sampledImageCount = 16384;
		unsigned int m_storageBufferCount = 65536;
		unsigned int m_accelerationStructureCount = 8;

		VkDescriptorPool m_descriptorPool;
	};
}