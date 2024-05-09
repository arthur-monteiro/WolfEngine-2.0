#pragma once

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Wolf
{
	class BufferVulkan;
	class CommandBufferVulkan;

	class AccelerationStructureVulkan
	{
	public:
		void build(const CommandBufferVulkan& commandBuffer);

		[[nodiscard]] const BufferVulkan& getStructureBuffer() const { return *m_structureBuffer; }
		[[nodiscard]] const VkAccelerationStructureKHR* getStructure() const { return &m_accelerationStructure; }

	protected:
		VkAccelerationStructureKHR m_accelerationStructure = VK_NULL_HANDLE;
		std::unique_ptr<BufferVulkan> m_structureBuffer;
		std::unique_ptr<BufferVulkan> m_scratchBuffer;

		VkAccelerationStructureBuildRangeInfoKHR m_rangeInfo = {};
		VkAccelerationStructureBuildGeometryInfoKHR m_buildInfo = {};
		VkAccelerationStructureBuildSizesInfoKHR m_buildSizeInfo = {};
	};
}

#endif