#ifdef WOLF_VULKAN

#include <Debug.h>
#include <GPUMemoryDebug.h>

#include "BufferVulkan.h"
#include "CommandBufferVulkan.h"
#include "FenceVulkan.h"
#include "SemaphoreVulkan.h"
#include "Vulkan.h"
#include "VulkanHelper.h"

Wolf::BufferVulkan::BufferVulkan(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	createBuffer(size, usage, properties);

	m_bufferSize = size;
}

Wolf::BufferVulkan::~BufferVulkan()
{
	vkDestroyBuffer(g_vulkanInstance->getDevice(), m_buffer, nullptr);
	vkFreeMemory(g_vulkanInstance->getDevice(), m_bufferMemory, nullptr);

	GPUMemoryDebug::unregisterResource(GPUMemoryDebug::TYPE::BUFFER, 0, m_bufferSize, m_allocationSize);
}

void Wolf::BufferVulkan::transferCPUMemory(const void* data, uint64_t srcSize, uint64_t srcOffset) const
{
	void* pData;
	map(&pData, srcSize);
	memcpy(pData, data, srcSize);
	unmap();
}

void Wolf::BufferVulkan::transferCPUMemoryWithStagingBuffer(const void* data, uint64_t srcSize, uint64_t srcOffset, uint64_t dstOffset) const
{
	const BufferVulkan stagingBuffer(srcSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* mappedData;
	vkMapMemory(g_vulkanInstance->getDevice(), stagingBuffer.getBufferMemory(), 0, srcSize, 0, &mappedData);
	std::memcpy(mappedData, static_cast<const char*>(data) + srcOffset, srcSize);
	vkUnmapMemory(g_vulkanInstance->getDevice(), stagingBuffer.getBufferMemory());

	BufferCopy bufferCopy;
	bufferCopy.size = srcSize;
	bufferCopy.srcOffset = 0;
	bufferCopy.dstOffset = dstOffset;
	transferGPUMemoryImmediate(stagingBuffer, bufferCopy);
}

void Wolf::BufferVulkan::transferGPUMemoryImmediate(const Buffer& bufferSrc, const BufferCopy& copyRegion) const
{
	VkBufferCopy vkCopyRegion;
	vkCopyRegion.size = copyRegion.size;
	vkCopyRegion.srcOffset = copyRegion.srcOffset;
	vkCopyRegion.dstOffset = copyRegion.dstOffset;

	const BufferVulkan* srcAsBufferVulkan = static_cast<const BufferVulkan*>(&bufferSrc);

	const CommandBufferVulkan commandBuffer(QueueType::TRANSFER, true);
	commandBuffer.beginCommandBuffer();

	vkCmdCopyBuffer(commandBuffer.getCommandBuffer(), srcAsBufferVulkan->getBuffer(), m_buffer, 1, &vkCopyRegion);

	commandBuffer.endCommandBuffer();

	const std::vector<const Semaphore*> waitSemaphores;
	const std::vector<const Semaphore*> signalSemaphores;
	const FenceVulkan fence(0);
	commandBuffer.submit(waitSemaphores, signalSemaphores, &fence);
	fence.waitForFence();
}

void Wolf::BufferVulkan::recordTransferGPUMemory(const CommandBuffer* commandBuffer, const Buffer& bufferSrc, const BufferCopy& copyRegion) const
{
	const CommandBufferVulkan* commandBufferVulkan = static_cast<const CommandBufferVulkan*>(commandBuffer);
	const BufferVulkan* srcAsBufferVulkan = static_cast<const BufferVulkan*>(&bufferSrc);

	VkBufferCopy vkCopyRegion;
	vkCopyRegion.size = copyRegion.size;
	vkCopyRegion.srcOffset = copyRegion.srcOffset;
	vkCopyRegion.dstOffset = copyRegion.dstOffset;

	vkCmdCopyBuffer(commandBufferVulkan->getCommandBuffer(), srcAsBufferVulkan->getBuffer(), m_buffer, 1, &vkCopyRegion);
}

void Wolf::BufferVulkan::map(void** pData, VkDeviceSize size) const
{
	if (size == 0)
		size = m_bufferSize;
	vkMapMemory(g_vulkanInstance->getDevice(), m_bufferMemory, 0, size, 0, pData);
}

void Wolf::BufferVulkan::unmap() const
{
	vkUnmapMemory(g_vulkanInstance->getDevice(), m_bufferMemory);
}

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30
VkDeviceAddress Wolf::BufferVulkan::getBufferDeviceAddress() const
{
	VkBufferDeviceAddressInfo info = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	info.buffer = m_buffer;
	return vkGetBufferDeviceAddress(g_vulkanInstance->getDevice(), &info);
}
#endif

void Wolf::BufferVulkan::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(g_vulkanInstance->getDevice(), &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS)
		Debug::sendError("Error : buffer creation");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(g_vulkanInstance->getDevice(), m_buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	m_allocationSize = allocInfo.allocationSize;
	allocInfo.memoryTypeIndex = findMemoryType(g_vulkanInstance->getPhysicalDevice(), memRequirements.memoryTypeBits, properties);

#ifdef WOLF_VULKAN_1_2
	VkMemoryAllocateFlagsInfoKHR allocFlagsInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR };
	allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	allocInfo.pNext = &allocFlagsInfo;
#endif

	if (vkAllocateMemory(g_vulkanInstance->getDevice(), &allocInfo, nullptr, &m_bufferMemory) != VK_SUCCESS)
		Debug::sendError("Error : memory allocation");

	vkBindBufferMemory(g_vulkanInstance->getDevice(), m_buffer, m_bufferMemory, 0);

	GPUMemoryDebug::registerNewResource(GPUMemoryDebug::TYPE::BUFFER, 0, size, memRequirements.size);
}

#endif