#pragma once

#include <Debug.h>

#include <Buffer.h>
#include <CommandBuffer.h>

#include "AABB.h"
#include "BoundingSphere.h"
#include "BufferPoolInterface.h"
#include "GPUDataTransfersManager.h"
#include "MeshInterface.h"

namespace Wolf
{
	class CameraInterface;

	class Mesh : public MeshInterface
	{
	public:
		template <typename T>
		Mesh(const std::vector<T>& vertices, const std::vector<uint32_t>& indices, const ResourceNonOwner<BufferPoolInterface>& bufferPoolInterface,
			const ResourceNonOwner<GPUDataTransfersManagerInterface>& gpuDataTransfersManager, AABB aabb = {}, BoundingSphere boundingSphere = {},
			VkBufferUsageFlags additionalVertexBufferUsages = 0, VkBufferUsageFlags additionalIndexBufferUsages = 0);
		Mesh(const ResourceNonOwner<BufferPoolInterface>& bufferPoolInterface, const BufferPoolInterface::BufferPoolInstance& vertexBufferPoolInstance, uint32_t vertexCount, uint32_t vertexSize,
			const std::vector<uint32_t>& indices, AABB aabb = {}, BoundingSphere boundingSphere = {}, VkBufferUsageFlags additionalIndexBufferUsages = 0);
		Mesh(const Mesh&) = delete;

		~Mesh() override;

		[[nodiscard]] uint32_t getVertexCount() const override { return m_vertexCount; }
		[[nodiscard]] uint32_t getVertexSize() const override { return m_vertexSize; }
		[[nodiscard]] uint32_t getIndexSize() const override { return sizeof(uint32_t); };

		[[nodiscard]] uint32_t getIndexCount() const override { return m_indexCount; }
		[[nodiscard]] NullableResourceNonOwner<Buffer> getVertexBuffer() const override;
		[[nodiscard]] uint32_t getVertexBufferOffset() const override;
		[[nodiscard]] NullableResourceNonOwner<Buffer> getIndexBuffer() const override;
		[[nodiscard]] uint32_t getIndexBufferOffset() const override;
		[[nodiscard]] const AABB& getAABB() const { return m_AABB; }
		[[nodiscard]] const BoundingSphere& getBoundingSphere() const override { return m_boundingSphere; }

		void draw(const CommandBuffer& commandBuffer, uint32_t cameraIdx, uint32_t instanceCount = 1, uint32_t firstInstance = 0) const override;

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
	};

	template<typename T>
	Mesh::Mesh(const std::vector<T>& vertices, const std::vector<uint32_t>& indices, const ResourceNonOwner<BufferPoolInterface>& bufferPoolInterface,
		const ResourceNonOwner<GPUDataTransfersManagerInterface>& gpuDataTransfersManager, AABB aabb, BoundingSphere boundingSphere,
		VkBufferUsageFlags additionalVertexBufferUsages, VkBufferUsageFlags additionalIndexBufferUsages) : m_bufferPoolInterface(bufferPoolInterface)
	{
		m_vertexBufferPoolInstance = m_bufferPoolInterface->allocate(sizeof(T) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | additionalVertexBufferUsages, sizeof(T));
		gpuDataTransfersManager->pushDataToGPUBuffer(static_cast<const void*>(vertices.data()), sizeof(T) * vertices.size(), getVertexBuffer(), getVertexBufferOffset());

		m_indexBufferPoolInstance = m_bufferPoolInterface->allocate(sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | additionalIndexBufferUsages, sizeof(uint32_t));
		gpuDataTransfersManager->pushDataToGPUBuffer(indices.data(), sizeof(uint32_t) * indices.size(), getIndexBuffer(), getIndexBufferOffset());

		m_vertexCount = static_cast<uint32_t>(vertices.size());
		m_vertexSize = sizeof(T);
		m_indexCount = static_cast<uint32_t>(indices.size());

		m_AABB = aabb;
		m_boundingSphere = boundingSphere;
	}
}

