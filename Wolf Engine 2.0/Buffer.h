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

		void transferCPUMemory(const void* data, VkDeviceSize srcSize, VkDeviceSize srcOffset = 0, uint32_t idx = 0) const;
		void transferCPUMemoryWithStagingBuffer(const void* data, VkDeviceSize srcSize, VkDeviceSize srcOffset = 0, uint32_t idx = 0) const;
		void transferGPUMemory(const Buffer& bufferSrc, const VkBufferCopy& copyRegion, uint32_t idx = 0) const;

		void map(void** pData, VkDeviceSize size = 0, uint32_t idx = 0) const;
		void unmap(uint32_t idx = 0) const;

		void getContent(void* outputData, uint32_t idx = 0) const;

		[[nodiscard]] VkBuffer getBuffer(uint32_t idx = 0) const { return m_buffers[idx].buffer; }
		[[nodiscard]] VkDeviceMemory getBufferMemory(uint32_t idx = 0) const { return m_buffers[idx].bufferMemory; }
		[[nodiscard]] VkDeviceSize getBufferSize() const { return m_bufferSize; }
#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30
		[[nodiscard]] VkDeviceAddress getBufferDeviceAddress(uint32_t idx = 0) const;
#endif

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

