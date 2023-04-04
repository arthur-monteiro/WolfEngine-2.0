#pragma once

#include <span>

#include "AccelerationStructure.h"

namespace Wolf
{
	class Mesh;

	struct GeometryInfo
	{
		const Mesh* mesh;

		const Buffer* transformBuffer = nullptr;
		VkDeviceSize transformOffsetInBytes = 0;
	};

	struct BottomLevelAccelerationStructureCreateInfo
	{
		std::span<GeometryInfo> geometryInfos;
		VkBuildAccelerationStructureFlagsKHR buildFlags;
	};

	class BottomLevelAccelerationStructure : public AccelerationStructure
	{
	public:
		BottomLevelAccelerationStructure(const BottomLevelAccelerationStructureCreateInfo& createInfo, bool keepVertexBuffersInCache = false);
		BottomLevelAccelerationStructure(const BottomLevelAccelerationStructure&) = delete;

	private:
		// Cached data
		std::vector<VkAccelerationStructureGeometryKHR> m_vertexBuffers;
	};
}
