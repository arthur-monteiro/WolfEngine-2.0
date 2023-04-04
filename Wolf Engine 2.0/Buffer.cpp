#include "Buffer.h"

#include "CommandBuffer.h"
#include "Configuration.h"
#include "Debug.h"
#include "Fence.h"
#include "Vulkan.h"
#include "VulkanHelper.h"

Wolf::Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, UpdateRate updateRate)
{
	uint32_t bufferCount;
	switch (updateRate)
	{
	case Wolf::UpdateRate::EACH_FRAME:
		bufferCount = g_configuration->getMaxCachedFrames();
		break;
	case Wolf::UpdateRate::NEVER:
		bufferCount = 1;
		break;
	default:
		break;
	}

	m_buffers.resize(bufferCount);
	for (uint32_t idx = 0; idx < m_buffers.size(); ++idx)
		createBuffer(idx, size, usage, properties);

	m_bufferSize = size;
}

Wolf::Buffer::~Buffer()
{
	for (BufferElements& bufferElements : m_buffers)
	{
		vkDestroyBuffer(g_vulkanInstance->getDevice(), bufferElements.buffer, nullptr);
		vkFreeMemory(g_vulkanInstance->getDevice(), bufferElements.bufferMemory, nullptr);
	}
}

void Wolf::Buffer::transferCPUMemory(void* data, VkDeviceSize srcSize, VkDeviceSize srcOffset, uint32_t idx)
{
	void* pData;
	map(&pData, srcSize, idx);
	memcpy(pData, data, srcSize);
	unmap(idx);
}

void Wolf::Buffer::transferCPUMemoryWithStagingBuffer(void* data, VkDeviceSize srcSize, VkDeviceSize srcOffset, uint32_t idx)
{
	Buffer stagingBuffer(srcSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, UpdateRate::NEVER);

	void* mappedData;
	vkMapMemory(g_vulkanInstance->getDevice(), stagingBuffer.getBufferMemory(), 0, srcSize, 0, &mappedData);
	std::memcpy(mappedData, (char*)data + srcOffset, static_cast<size_t>(srcSize));
	vkUnmapMemory(g_vulkanInstance->getDevice(), stagingBuffer.getBufferMemory());

	VkBufferCopy bufferCopy;
	bufferCopy.size = srcSize;
	bufferCopy.srcOffset = 0;
	bufferCopy.dstOffset = 0;
	transferGPUMemory(stagingBuffer, bufferCopy, idx);
}

void Wolf::Buffer::transferGPUMemory(const Buffer& bufferSrc, const VkBufferCopy& copyRegion, uint32_t idx)
{
	CommandBuffer commandBuffer(QueueType::TRANSFER, true);
	commandBuffer.beginCommandBuffer(0);

	vkCmdCopyBuffer(commandBuffer.getCommandBuffer(0), bufferSrc.getBuffer(), m_buffers[idx].buffer, 1, &copyRegion);

	commandBuffer.endCommandBuffer(0);

	std::vector<const Semaphore*> waitSemaphores;
	std::vector<VkSemaphore> signalSemaphores;
	Fence fence(0);
	commandBuffer.submit(0, waitSemaphores, signalSemaphores, fence.getFence());
	fence.waitForFence();
}

void Wolf::Buffer::map(void** pData, VkDeviceSize size, uint32_t idx)
{
	if (size == 0)
		size = m_bufferSize;
	vkMapMemory(g_vulkanInstance->getDevice(), m_buffers[idx].bufferMemory, 0, size, 0, pData);
}

void Wolf::Buffer::unmap(uint32_t idx)
{
	vkUnmapMemory(g_vulkanInstance->getDevice(), m_buffers[idx].bufferMemory);
}

void Wolf::Buffer::getContent(void* outputData, uint32_t idx)
{
	void* pData;
	vkMapMemory(g_vulkanInstance->getDevice(), m_buffers[idx].bufferMemory, 0, m_bufferSize, 0, &pData);
	memcpy(outputData, pData, m_bufferSize);
	vkUnmapMemory(g_vulkanInstance->getDevice(), m_buffers[idx].bufferMemory);
}

VkDeviceAddress Wolf::Buffer::getBufferDeviceAddress(uint32_t idx) const
{
	VkBufferDeviceAddressInfo info = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	info.buffer = m_buffers[idx].buffer;
	return vkGetBufferDeviceAddress(g_vulkanInstance->getDevice(), &info);
}

void Wolf::Buffer::createBuffer(uint32_t idx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(g_vulkanInstance->getDevice(), &bufferInfo, nullptr, &m_buffers[idx].buffer) != VK_SUCCESS)
		Debug::sendError("Error : buffer creation");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(g_vulkanInstance->getDevice(), m_buffers[idx].buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(g_vulkanInstance->getPhysicalDevice(), memRequirements.memoryTypeBits, properties);

	VkMemoryAllocateFlagsInfoKHR allocFlagsInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR };
	allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	allocInfo.pNext = &allocFlagsInfo;

	if (vkAllocateMemory(g_vulkanInstance->getDevice(), &allocInfo, nullptr, &m_buffers[idx].bufferMemory) != VK_SUCCESS)
		Debug::sendError("Error : memory allocation");

	vkBindBufferMemory(g_vulkanInstance->getDevice(), m_buffers[idx].buffer, m_buffers[idx].bufferMemory, 0);
}
