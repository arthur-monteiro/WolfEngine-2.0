#pragma once

#include <memory>

#include "Buffer.h"

namespace Wolf
{
	class AccelerationStructure
	{
	public:
		void build(VkCommandBuffer commandBuffer);

		const Buffer& getStructureBuffer() const { return *m_structureBuffer.get(); }
		const VkAccelerationStructureKHR* getStructure() const { return &m_accelerationStructure; }

	protected:
		VkAccelerationStructureKHR m_accelerationStructure = VK_NULL_HANDLE;
		std::unique_ptr<Buffer> m_structureBuffer;
		std::unique_ptr<Buffer> m_scratchBuffer;

		VkAccelerationStructureBuildRangeInfoKHR m_rangeInfo = {};
		VkAccelerationStructureBuildGeometryInfoKHR m_buildInfo = {};
		VkAccelerationStructureBuildSizesInfoKHR m_buildSizeInfo = {};
	};
}
