#pragma once

#ifdef WOLF_VULKAN

#include <vulkan/vulkan_core.h>

#include "../../Public/Buffer.h"

namespace Wolf
{
	class BufferVulkan final : public Buffer
	{
	public:
		BufferVulkan(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		BufferVulkan(const BufferVulkan&) = delete;
		~BufferVulkan() override;

		void transferCPUMemory(const void* data, uint64_t srcSize, uint64_t srcOffset = 0) const override;
		void transferCPUMemoryWithStagingBuffer(const void* data, uint64_t srcSize, uint64_t srcOffset = 0, uint64_t dstOffset = 0) const override;
		void transferGPUMemoryImmediate(const Buffer& bufferSrc, const BufferCopy& copyRegion) const override;
		void recordTransferGPUMemory(const CommandBuffer* commandBuffer, const Buffer& bufferSrc, const BufferCopy& copyRegion) const override;
		void recordFillBuffer(const CommandBuffer* commandBuffer, const BufferFill& bufferFill) const override;
		void recordBarrier(const CommandBuffer* commandBuffer, const BufferAccess& accessBefore, const BufferAccess& accessAfter) const override;

		void* map(VkDeviceSize size = 0) const override;
		void unmap() const override;

		[[nodiscard]] uint32_t getSize() const override;

		[[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }
		[[nodiscard]] VkDeviceMemory getBufferMemory() const { return m_bufferMemory; }
		[[nodiscard]] VkDeviceSize getBufferSize() const { return m_bufferSize; }
#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30
		[[nodiscard]] VkDeviceAddress getBufferDeviceAddress() const;
#endif

	private:
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
		VkBuffer m_buffer = nullptr;
		VkDeviceMemory m_bufferMemory = nullptr;
#else
		VkBuffer m_buffer = 0;
		VkDeviceMemory m_bufferMemory = 0;
#endif

		VkDeviceSize m_bufferSize;
		VkDeviceSize m_allocationSize = 0;
	};
}

#endif