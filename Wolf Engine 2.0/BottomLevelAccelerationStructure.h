#pragma once

#include <span>

#include "AccelerationStructure.h"
#include "Mesh.h"
#include "ResourceNonOwner.h"

namespace Wolf
{
	struct GeometryInfo
	{
		ResourceNonOwner<const Mesh> mesh;

		const Buffer* transformBuffer = nullptr;
		VkDeviceSize transformOffsetInBytes = 0;

		GeometryInfo(const ResourceNonOwner<const Mesh>& mesh) : mesh(mesh) {}
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
