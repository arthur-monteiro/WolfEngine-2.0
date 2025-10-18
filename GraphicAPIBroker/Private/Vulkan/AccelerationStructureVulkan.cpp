#include "AccelerationStructureVulkan.h"

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30

#include "BufferVulkan.h"
#include "CommandBufferVulkan.h"
#include "Vulkan.h"

Wolf::AccelerationStructureVulkan::~AccelerationStructureVulkan()
{
	vkDestroyAccelerationStructureKHR(g_vulkanInstance->getDevice(), m_accelerationStructure, nullptr);
}

void Wolf::AccelerationStructureVulkan::build(const CommandBufferVulkan& commandBuffer)
{
	if (!m_scratchBuffer)
	{
		m_scratchBuffer.reset(new BufferVulkan(m_buildSizeInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	}

	m_buildInfo.scratchData.deviceAddress = m_scratchBuffer->getBufferDeviceAddress();

	const VkAccelerationStructureBuildRangeInfoKHR* rangeInfoPtr = &m_rangeInfo;
	vkCmdBuildAccelerationStructuresKHR(commandBuffer.getCommandBuffer(), 1, &m_buildInfo, &rangeInfoPtr);
}

#endif