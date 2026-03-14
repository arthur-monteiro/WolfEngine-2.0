#pragma once

#ifdef WOLF_VULKAN

#include <vulkan/vulkan_core.h>

#include <GPUMemoryAllocatorInterface.h>

#include "../../Public/Buffer.h"

namespace Wolf
{
	class BufferVulkan final : public Buffer, public GPUMemoryAllocatorInterface
	{
	public:
		BufferVulkan(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		BufferVulkan(const BufferVulkan&) = delete;
		~BufferVulkan() override;

		[[nodiscard]] uint32_t getMemoryAllocatedSize() const override { return m_allocationSize; }
		[[nodiscard]] uint32_t getMemoryRequestedSize() const override { return m_bufferSize; }
		[[nodiscard]] Type getType() const override { return Type::BUFFER; }
		void setName(const std::string& name) override;
		[[nodiscard]] std::string getName() const override { return m_name; }
		[[nodiscard]] bool isPoolOrAtlas() const override { return false; }
		void registerUsageCallback(const std::function<float(void)>& callback) override { m_getUsageCallback = callback; }
		[[nodiscard]] float getUsagePercentage() const override { return m_getUsageCallback ? m_getUsageCallback() * 100.0f : 100.0f; };

		void transferCPUMemory(const void* data, uint64_t srcSize, uint64_t srcOffset = 0) const override;
		void transferCPUMemoryWithStagingBuffer(const void* data, uint64_t srcSize, uint64_t srcOffset = 0, uint64_t dstOffset = 0) const override;
		void transferGPUMemoryImmediate(const Buffer& bufferSrc, const BufferCopy& copyRegion) const override;
		void recordTransferGPUMemory(const CommandBuffer* commandBuffer, const Buffer& bufferSrc, const BufferCopy& copyRegion) const override;
		void recordFillBuffer(const CommandBuffer* commandBuffer, const BufferFill& bufferFill) const override;
		void recordBarrier(const CommandBuffer* commandBuffer, const BufferAccess& accessBefore, const BufferAccess& accessAfter, uint32_t offset, uint32_t size) const override;

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

		VkDeviceSize m_bufferSize = 0;
		VkDeviceSize m_allocationSize = 0;

		std::string m_name = "Undefined";
		std::function<float(void)> m_getUsageCallback;
		bool m_registeredToVRAMProfiler = false;
	};
}

#endif