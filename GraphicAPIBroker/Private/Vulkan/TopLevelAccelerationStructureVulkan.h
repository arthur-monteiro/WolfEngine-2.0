#pragma once

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30

#include <ResourceUniqueOwner.h>

#include "AccelerationStructureVulkan.h"
#include "../../Public/TopLevelAccelerationStructure.h"

namespace Wolf
{
	class TopLevelAccelerationStructureVulkan : public AccelerationStructureVulkan, public TopLevelAccelerationStructure
	{
	public:
		TopLevelAccelerationStructureVulkan(uint32_t instanceCount);
		TopLevelAccelerationStructureVulkan(const TopLevelAccelerationStructureVulkan&) = delete;

		void build(const CommandBuffer* commandBuffer, std::span<BLASInstance> blasInstances);
		void recordBuildBarriers(const CommandBuffer* commandBuffer);
		uint32_t getInstanceCount() const override { return m_instanceCount; }

		~TopLevelAccelerationStructureVulkan() override;

	private:
		uint32_t m_instanceCount;
		ResourceUniqueOwner<BufferVulkan> m_instanceBuffer;
		ResourceUniqueOwner<BufferVulkan> m_instanceBufferCPUWriteable;

		VkAccelerationStructureGeometryInstancesDataKHR m_instancesVk;
		VkAccelerationStructureGeometryKHR m_topASGeometry;
	};
}

#endif