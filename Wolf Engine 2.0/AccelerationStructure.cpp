#include "AccelerationStructure.h"

void Wolf::AccelerationStructure::build(VkCommandBuffer commandBuffer)
{
	if (!m_scratchBuffer)
	{
		m_scratchBuffer.reset(new Buffer(m_buildSizeInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, UpdateRate::NEVER));
	}

	m_buildInfo.scratchData.deviceAddress = m_scratchBuffer->getBufferDeviceAddress();

	VkAccelerationStructureBuildRangeInfoKHR* rangeInfoPtr = &m_rangeInfo;
	vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &m_buildInfo, &rangeInfoPtr);
}