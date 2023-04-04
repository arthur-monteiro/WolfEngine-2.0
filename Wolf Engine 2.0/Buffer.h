#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "UpdateRate.h"

namespace Wolf
{
	class Buffer
	{
	public:
		Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, UpdateRate updateRate);
		Buffer(const Buffer&) = delete;
		~Buffer();

		void transferCPUMemory(void* data, VkDeviceSize srcSize, VkDeviceSize srcOffset = 0, uint32_t idx = 0);
		void transferCPUMemoryWithStagingBuffer(void* data, VkDeviceSize srcSize, VkDeviceSize srcOffset = 0, uint32_t idx = 0);
		void transferGPUMemory(const Buffer& bufferSrc, const VkBufferCopy& copyRegion, uint32_t idx = 0);

		void map(void** pData, VkDeviceSize size = 0, uint32_t idx = 0);
		void unmap(uint32_t idx = 0);

		void getContent(void* outputData, uint32_t idx = 0);

		VkBuffer getBuffer(uint32_t idx = 0) const { return m_buffers[idx].buffer; }
		VkDeviceMemory getBufferMemory(uint32_t idx = 0) const { return m_buffers[idx].bufferMemory; }
		VkDeviceSize getBufferSize() const { return m_bufferSize; }
		VkDeviceAddress getBufferDeviceAddress(uint32_t idx = 0) const;

	private:
		void createBuffer(uint32_t idx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

		struct BufferElements
		{
			VkBuffer buffer;
			VkDeviceMemory bufferMemory;
		};
		std::vector<BufferElements> m_buffers;

		VkDeviceSize m_bufferSize;
	};
}

