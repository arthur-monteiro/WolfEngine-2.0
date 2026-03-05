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
#include "BufferPoolInterface.h"
#include "MeshInterface.h"
#include "SubMesh.h"

namespace Wolf
{
	class CameraInterface;

	class Mesh : public MeshInterface
	{
	public:
		template <typename T>
		Mesh(const std::vector<T>& vertices, const std::vector<uint32_t>& indices, const ResourceNonOwner<BufferPoolInterface>& bufferPoolInterface, AABB aabb = {}, BoundingSphere boundingSphere = {},
			VkBufferUsageFlags additionalVertexBufferUsages = 0, VkBufferUsageFlags additionalIndexBufferUsages = 0);
		Mesh(const Mesh&) = delete;

		void addSubMesh(uint32_t indicesOffset, uint32_t indexCount, AABB aabb = {});

		[[nodiscard]] uint32_t getVertexCount() const { return m_vertexCount; }
		[[nodiscard]] uint32_t getVertexSize() const override { return m_vertexSize; }
		[[nodiscard]] uint32_t getIndexSize() const override { return sizeof(uint32_t); };

		[[nodiscard]] uint32_t getIndexCount() const override { return m_indexCount; }
		[[nodiscard]] NullableResourceNonOwner<Buffer> getVertexBuffer() const override;
		[[nodiscard]] uint32_t getVertexBufferOffset() const override;
		[[nodiscard]] NullableResourceNonOwner<Buffer> getIndexBuffer() const override;
		[[nodiscard]] uint32_t getIndexBufferOffset() const override;
		[[nodiscard]] const AABB& getAABB() const { return m_AABB; }
		[[nodiscard]] const BoundingSphere& getBoundingSphere() const override { return m_boundingSphere; }

		void cullForCamera(uint32_t cameraIdx, const CameraInterface* camera, const glm::mat4& transform, bool isInstanced);
		void draw(const CommandBuffer& commandBuffer, uint32_t cameraIdx, uint32_t instanceCount = 1, uint32_t firstInstance = 0,
			const NullableResourceNonOwner<Buffer>& overrideIndexBuffer = NullableResourceNonOwner<Buffer>()) const override;

		bool hasVertexBuffer() const override;

	private:
		ResourceNonOwner<BufferPoolInterface> m_bufferPoolInterface;
		BufferPoolInterface::BufferPoolInstance m_vertexBufferPoolInstance;
		BufferPoolInterface::BufferPoolInstance m_indexBufferPoolInstance;

		uint32_t m_vertexCount;
		uint32_t m_vertexSize;
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
	Mesh::Mesh(const std::vector<T>& vertices, const std::vector<uint32_t>& indices, const ResourceNonOwner<BufferPoolInterface>& bufferPoolInterface, AABB aabb, BoundingSphere boundingSphere,
		VkBufferUsageFlags additionalVertexBufferUsages, VkBufferUsageFlags additionalIndexBufferUsages) : m_bufferPoolInterface(bufferPoolInterface)
	{
		m_vertexBufferPoolInstance = m_bufferPoolInterface->allocate(sizeof(T) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | additionalVertexBufferUsages, sizeof(T));
		getVertexBuffer()->transferCPUMemoryWithStagingBuffer(static_cast<const void*>(vertices.data()), sizeof(T) * vertices.size(), 0, getVertexBufferOffset());

		m_indexBufferPoolInstance = m_bufferPoolInterface->allocate(sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | additionalIndexBufferUsages, sizeof(uint32_t));
		getIndexBuffer()->transferCPUMemoryWithStagingBuffer(indices.data(), sizeof(uint32_t) * indices.size(), 0, getIndexBufferOffset());

		m_vertexCount = static_cast<uint32_t>(vertices.size());
		m_vertexSize = sizeof(T);
		m_indexCount = static_cast<uint32_t>(indices.size());

		m_AABB = aabb;
		m_boundingSphere = boundingSphere;
	}
}

