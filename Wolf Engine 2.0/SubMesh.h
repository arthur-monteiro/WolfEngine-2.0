#pragma once

#include "AABB.h"
#include "CommandBuffer.h"

namespace Wolf
{
	class Mesh;

	class SubMesh
	{
	public:
		SubMesh(uint32_t indicesOffset, uint32_t indexCount, AABB aabb = {});
		SubMesh(const SubMesh&) = delete;
		SubMesh(const SubMesh&&) = delete;

		uint32_t getIndicesOffset() const { return m_indicesOffset; }
		uint32_t getIndexCount() const { return m_indexCount; }

	private:
		uint32_t m_indicesOffset;
		uint32_t m_indexCount;

		AABB m_AABB;
	};
}
