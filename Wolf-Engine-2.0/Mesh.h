#pragma once

#include <map>
#include <memory>

#include <Debug.h>
#include <ResourceUniqueOwner.h>

#include <Buffer.h>
#include <CommandBuffer.h>
#include <Formats.h>

#include "AABB.h"
#include "BoundingSphere.h"
#include "SubMesh.h"

namespace Wolf
{
	class CameraInterface;

	class Mesh
	{
	public:
		template <typename T>
		Mesh(const std::vector<T>& vertices, const std::vector<uint32_t>& indices, AABB aabb = {}, BoundingSphere boundingSphere = {}, VkBufferUsageFlags additionalVertexBufferUsages = 0, VkBufferUsageFlags additionalIndexBufferUsages = 0, Format vertexFormat = Format::UNDEFINED);
		Mesh(const Mesh&) = delete;

		void addSubMesh(uint32_t indicesOffset, uint32_t indexCount, AABB aabb = {});

		[[nodiscard]] uint32_t getVertexCount() const { return m_vertexCount; }
		[[nodiscard]] uint32_t getVertexSize() const { return m_vertexSize; }

		[[nodiscard]] Format getVertexFormat() const
		{ 
			if (m_vertexFormat == Format::UNDEFINED)
			{
				Debug::sendError("Format is undefined");
			}
			return m_vertexFormat; 
		}

		[[nodiscard]] uint32_t getIndexCount() const { return m_indexCount; }
		[[nodiscard]] const Buffer& getVertexBuffer() const { return *m_vertexBuffer; }
		[[nodiscard]] ResourceNonOwner<Buffer> getVertexBuffer() { return m_vertexBuffer.createNonOwnerResource(); }
		[[nodiscard]] const Buffer& getIndexBuffer() const { return *m_indexBuffer; }
		[[nodiscard]] ResourceNonOwner<Buffer> getIndexBuffer() { return m_indexBuffer.createNonOwnerResource(); }
		[[nodiscard]] const AABB& getAABB() const { return m_AABB; }
		[[nodiscard]] const BoundingSphere& getBoundingSphere() const { return m_boundingSphere; }

		void cullForCamera(uint32_t cameraIdx, const CameraInterface* camera, const glm::mat4& transform, bool isInstanced);
		void draw(const CommandBuffer& commandBuffer, uint32_t cameraIdx, uint32_t instanceCount = 1, uint32_t firstInstance = 0) const;

	private:
		ResourceUniqueOwner<Buffer> m_vertexBuffer;
		ResourceUniqueOwner<Buffer> m_indexBuffer;

		uint32_t m_vertexCount;
		uint32_t m_vertexSize;
		Format m_vertexFormat;
		uint32_t m_indexCount;

		AABB m_AABB;
		BoundingSphere m_boundingSphere;

		std::vector<std::unique_ptr<SubMesh>> m_subMeshes;

		struct SubMeshToDrawInfo
		{
			uint32_t indicesOffset;
			uint32_t indexCount;
		};
		std::map<uint32_t, std::vector<std::unique_ptr<SubMeshToDrawInfo>>> m_subMeshesToDrawByCamera;
	};

	template<typename T>
	Mesh::Mesh(const std::vector<T>& vertices, const std::vector<uint32_t>& indices, AABB aabb, BoundingSphere boundingSphere, VkBufferUsageFlags additionalVertexBufferUsages, VkBufferUsageFlags additionalIndexBufferUsages, Format vertexFormat)
	{
		m_vertexBuffer.reset(Buffer::createBuffer(sizeof(T) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | additionalVertexBufferUsages, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		m_vertexBuffer->transferCPUMemoryWithStagingBuffer(static_cast<const void*>(vertices.data()), sizeof(T) * vertices.size());

		m_indexBuffer.reset(Buffer::createBuffer(sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | additionalIndexBufferUsages, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		m_indexBuffer->transferCPUMemoryWithStagingBuffer(indices.data(), sizeof(uint32_t) * indices.size());

		m_vertexCount = static_cast<uint32_t>(vertices.size());
		m_vertexSize = sizeof(T);
		m_vertexFormat = vertexFormat;
		m_indexCount = static_cast<uint32_t>(indices.size());

		m_AABB = aabb;
		m_boundingSphere = boundingSphere;
	}
}

