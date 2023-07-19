#include "TopLevelAccelerationStructure.h"

#include "CommandBuffer.h"
#include "Fence.h"
#include "Vulkan.h"
#include "VulkanHelper.h"

Wolf::TopLevelAccelerationStructure::TopLevelAccelerationStructure(std::span<BLASInstance> blasInstances)
{
    std::vector<VkAccelerationStructureInstanceKHR> instances;
    instances.reserve(blasInstances.size());
    for (const BLASInstance& blasInstance : blasInstances)
    {
        VkAccelerationStructureInstanceKHR rayInst{};
        rayInst.transform = toTransformMatrixKHR(&blasInstance.transform);
        rayInst.instanceCustomIndex = blasInstance.instanceID; // gl_InstanceCustomIndexEXT
        rayInst.accelerationStructureReference = blasInstance.bottomLevelAS->getStructureBuffer().getBufferDeviceAddress();
        rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        rayInst.mask = 0xFF; //  Only be hit if rayMask & instance.mask != 0
        rayInst.instanceShaderBindingTableRecordOffset = 0; // Do be replace with blasInstance.hitGroupIndex

        instances.emplace_back(rayInst);
    }

	m_instanceBuffer.reset(new Buffer(instances.size() * sizeof(VkAccelerationStructureInstanceKHR), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, UpdateRate::NEVER));
	m_instanceBuffer->transferCPUMemoryWithStagingBuffer(instances.data(), instances.size() * sizeof(VkAccelerationStructureInstanceKHR));

	VkAccelerationStructureGeometryInstancesDataKHR instancesVk{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
	instancesVk.data.deviceAddress = m_instanceBuffer->getBufferDeviceAddress();

	VkAccelerationStructureGeometryKHR topASGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	topASGeometry.geometry.instances = instancesVk;

	m_buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	m_buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	m_buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR; // VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR for update
	m_buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; // Useful ?
	m_buildInfo.geometryCount = 1;
	m_buildInfo.pGeometries = &topASGeometry;

	uint32_t instanceCount = static_cast<uint32_t>(instances.size());
	m_buildSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(g_vulkanInstance->getDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &m_buildInfo, &instanceCount, &m_buildSizeInfo);

	m_structureBuffer.reset(new Buffer(m_buildSizeInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, UpdateRate::NEVER));

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationStructureCreateInfo.pNext = nullptr;
	accelerationStructureCreateInfo.size = m_buildSizeInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.buffer = m_structureBuffer->getBuffer();

	if (vkCreateAccelerationStructureKHR(g_vulkanInstance->getDevice(), &accelerationStructureCreateInfo, nullptr, &m_accelerationStructure))
		Debug::sendError("vkCreateAccelerationStructureNV failed");

	m_buildInfo.dstAccelerationStructure = m_accelerationStructure;

	m_rangeInfo.primitiveCount = instanceCount;

	CommandBuffer commandBuffer(QueueType::COMPUTE, true);
	commandBuffer.beginCommandBuffer(0);

	build(commandBuffer.getCommandBuffer(0));

	commandBuffer.endCommandBuffer(0);

	std::vector<const Semaphore*> waitSemaphores;
	std::vector<VkSemaphore> signalSemaphores;
	Fence fence(0);
	commandBuffer.submit(0, waitSemaphores, signalSemaphores, fence.getFence());
	fence.waitForFence();

	m_scratchBuffer.reset();
}
