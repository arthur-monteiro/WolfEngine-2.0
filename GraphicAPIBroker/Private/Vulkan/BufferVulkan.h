#pragma once

#ifdef WOLF_VULKAN

#include <vulkan/vulkan_core.h>

#include "../../Public/Buffer.h"

namespace Wolf
{
	class BufferVulkan : public Buffer
	{
	public:
		BufferVulkan(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		BufferVulkan(const BufferVulkan&) = delete;
		~BufferVulkan() override;

		void transferCPUMemory(const void* data, uint64_t srcSize, uint64_t srcOffset = 0) const override;
		void transferCPUMemoryWithStagingBuffer(const void* data, uint64_t srcSize, uint64_t srcOffset = 0, uint64_t dstOffset = 0) const override;
		void transferGPUMemory(const Buffer& bufferSrc, const BufferCopy& copyRegion) const override;

		void map(void** pData, VkDeviceSize size = 0) const override;
		void unmap() const override;

		[[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }
		[[nodiscard]] VkDeviceMemory getBufferMemory() const { return m_bufferMemory; }
		[[nodiscard]] VkDeviceSize getBufferSize() const { return m_bufferSize; }
#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30
		[[nodiscard]] VkDeviceAddress getBufferDeviceAddress() const;
#endif

	private:
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

		VkBuffer m_buffer;
		VkDeviceMemory m_bufferMemory;

		VkDeviceSize m_bufferSize;

	};
}

#endif