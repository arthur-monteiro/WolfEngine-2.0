#include "TopLevelAccelerationStructureVulkan.h"

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30

#include <Debug.h>

#include "BottomLevelAccelerationStructureVulkan.h"
#include "BufferVulkan.h"
#include "CommandBufferVulkan.h"
#include "FenceVulkan.h"
#include "Vulkan.h"
#include "VulkanHelper.h"

Wolf::TopLevelAccelerationStructureVulkan::TopLevelAccelerationStructureVulkan(uint32_t instanceCount) : m_instanceCount(instanceCount)
{
	m_instanceBuffer.reset(new BufferVulkan(m_instanceCount * sizeof(VkAccelerationStructureInstanceKHR), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	m_instanceBufferCPUWriteable.reset(new BufferVulkan(m_instanceBuffer->getSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

	m_instancesVk = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
	m_instancesVk.data.deviceAddress = m_instanceBuffer->getBufferDeviceAddress();

	m_topASGeometry = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	m_topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	m_topASGeometry.geometry.instances = m_instancesVk;

	m_buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	m_buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	m_buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR; // VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR for update
	m_buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; // Useful ?
	m_buildInfo.geometryCount = 1;
	m_buildInfo.pGeometries = &m_topASGeometry;

	m_buildSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(g_vulkanInstance->getDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &m_buildInfo, &instanceCount, &m_buildSizeInfo);

	m_structureBuffer.reset(new BufferVulkan(m_buildSizeInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationStructureCreateInfo.pNext = nullptr;
	accelerationStructureCreateInfo.size = m_buildSizeInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.buffer = m_structureBuffer->getBuffer();

	if (vkCreateAccelerationStructureKHR(g_vulkanInstance->getDevice(), &accelerationStructureCreateInfo, nullptr, &m_accelerationStructure))
		Debug::sendError("vkCreateAccelerationStructureNV failed");

	m_buildInfo.dstAccelerationStructure = m_accelerationStructure;

	m_rangeInfo.primitiveCount = instanceCount;
}

void Wolf::TopLevelAccelerationStructureVulkan::build(CommandBuffer* commandBuffer, std::span<BLASInstance> blasInstances)
{
	if (blasInstances.size() != m_instanceCount)
	{
		Debug::sendCriticalError("BLAS instance count doesn't match the number of instances used at creation");
	}

	std::vector<VkAccelerationStructureInstanceKHR> instances;
	instances.reserve(blasInstances.size());
	for (const BLASInstance& blasInstance : blasInstances)
	{
	    VkAccelerationStructureInstanceKHR rayInst{};
	    rayInst.transform = toTransformMatrixKHR(&blasInstance.transform);
	    rayInst.instanceCustomIndex = blasInstance.instanceID; // gl_InstanceCustomIndexEXT
	    rayInst.accelerationStructureReference = static_cast<const BottomLevelAccelerationStructureVulkan*>(blasInstance.bottomLevelAS)->getStructureBuffer().getBufferDeviceAddress();
	    rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	    rayInst.mask = 0xFF; //  Only be hit if rayMask & instance.mask != 0
	    rayInst.instanceShaderBindingTableRecordOffset = 0; // Do be replace with blasInstance.hitGroupIndex

	    instances.emplace_back(rayInst);
	}

	m_instanceBufferCPUWriteable->transferCPUMemory(instances.data(), instances.size() * sizeof(VkAccelerationStructureInstanceKHR));

	Buffer::BufferCopy bufferCopy{};
	bufferCopy.srcOffset = 0;
	bufferCopy.dstOffset = 0;
	bufferCopy.size = m_instanceBufferCPUWriteable->getSize();
	m_instanceBuffer->recordTransferGPUMemory(commandBuffer, *m_instanceBufferCPUWriteable, bufferCopy);

	AccelerationStructureVulkan::build(*static_cast<CommandBufferVulkan*>(commandBuffer));
}

#endif
