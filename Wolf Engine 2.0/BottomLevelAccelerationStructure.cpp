#include "BottomLevelAccelerationStructure.h"

#include "Fence.h"
#include "Mesh.h"
#include "Vulkan.h"

Wolf::BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(const BottomLevelAccelerationStructureCreateInfo& createInfo, bool keepVertexBuffersInCach)
{
	m_vertexBuffers.reserve(createInfo.geometryInfos.size());

	m_rangeInfo.firstVertex = 0;
	m_rangeInfo.primitiveCount = 0;
	m_rangeInfo.primitiveOffset = 0;
	m_rangeInfo.transformOffset = 0;

	for (const GeometryInfo& geometryInfo : createInfo.geometryInfos)
	{
		VkAccelerationStructureGeometryKHR geometry;
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.pNext = nullptr;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		geometry.geometry.triangles.pNext = nullptr;
		geometry.geometry.triangles.vertexData.deviceAddress = geometryInfo.mesh->getVertexBuffer().getBufferDeviceAddress();
		geometry.geometry.triangles.maxVertex = geometryInfo.mesh->getVertexCount();
		geometry.geometry.triangles.vertexStride = geometryInfo.mesh->getVertexSize();
		// Limitation to 3xfloat32 for vertices
		geometry.geometry.triangles.vertexFormat = geometryInfo.mesh->getVertexFormat();
		geometry.geometry.triangles.indexData.deviceAddress = geometryInfo.mesh->getIndexBuffer().getBufferDeviceAddress();
		// Limitation to 32-bit indices
		geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		geometry.geometry.triangles.transformData.deviceAddress = geometryInfo.transformBuffer ? geometryInfo.transformBuffer->getBufferDeviceAddress() : 0;
		geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

		m_vertexBuffers.emplace_back(geometry);
		
		m_rangeInfo.primitiveCount += geometryInfo.mesh->getIndexCount() / 3;
	}

	m_buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	m_buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	m_buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	m_buildInfo.flags = createInfo.buildFlags;
	m_buildInfo.geometryCount = static_cast<uint32_t>(m_vertexBuffers.size());
	m_buildInfo.pGeometries = m_vertexBuffers.data();

	uint32_t maxPrimCount = m_rangeInfo.primitiveCount;
	m_buildSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(g_vulkanInstance->getDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &m_buildInfo, &maxPrimCount, &m_buildSizeInfo);

	m_structureBuffer.reset(new Buffer(m_buildSizeInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, UpdateRate::NEVER));

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationStructureCreateInfo.pNext = nullptr;
	accelerationStructureCreateInfo.size = m_buildSizeInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.buffer = m_structureBuffer->getBuffer();

	if (vkCreateAccelerationStructureKHR(g_vulkanInstance->getDevice(), &accelerationStructureCreateInfo, nullptr, &m_accelerationStructure))
		Debug::sendError("vkCreateAccelerationStructureNV failed");

	m_buildInfo.dstAccelerationStructure = m_accelerationStructure;

	CommandBuffer commandBuffer(QueueType::COMPUTE, true);
	commandBuffer.beginCommandBuffer(0);

	build(commandBuffer.getCommandBuffer(0));

	commandBuffer.endCommandBuffer(0);

	std::vector<const Semaphore*> waitSemaphores;
	std::vector<VkSemaphore> signalSemaphores;
	Fence fence(0);
	commandBuffer.submit(0, waitSemaphores, signalSemaphores, fence.getFence());
	fence.waitForFence();

	if (!keepVertexBuffersInCach)
	{
		m_scratchBuffer.reset();
	}
}