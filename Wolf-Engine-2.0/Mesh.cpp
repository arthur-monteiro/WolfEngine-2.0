#include "Mesh.h"

#include "CameraInterface.h"
#include "DefaultMeshRenderer.h"

Wolf::Mesh::Mesh(const ResourceNonOwner<BufferPoolInterface>& bufferPoolInterface, const BufferPoolInterface::BufferPoolInstance& vertexBufferPoolInstance, uint32_t vertexCount, uint32_t vertexSize,
	const std::vector<uint32_t>& indices, AABB aabb, BoundingSphere boundingSphere, VkBufferUsageFlags additionalIndexBufferUsages) : m_bufferPoolInterface(bufferPoolInterface)
{
	m_vertexBufferPoolInstance = vertexBufferPoolInstance;

	m_indexBufferPoolInstance = m_bufferPoolInterface->allocate(sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		additionalIndexBufferUsages, sizeof(uint32_t));
	m_bufferPoolInterface->getBuffer(m_indexBufferPoolInstance)->transferCPUMemoryWithStagingBuffer(indices.data(), sizeof(uint32_t) * indices.size(), 0,
		m_indexBufferPoolInstance.m_bufferOffset);

	m_vertexCount = vertexCount;
	m_vertexSize = vertexSize;
	m_indexCount = static_cast<uint32_t>(indices.size());

	m_AABB = aabb;
	m_boundingSphere = boundingSphere;
}

Wolf::NullableResourceNonOwner<Wolf::Buffer> Wolf::Mesh::getVertexBuffer() const
{
	return m_bufferPoolInterface->getBuffer(m_vertexBufferPoolInstance);
}

uint32_t Wolf::Mesh::getVertexBufferOffset() const
{
	return m_vertexBufferPoolInstance.m_bufferOffset;
}

Wolf::NullableResourceNonOwner<Wolf::Buffer> Wolf::Mesh::getIndexBuffer() const
{
	return m_bufferPoolInterface->getBuffer(m_indexBufferPoolInstance);
}

uint32_t Wolf::Mesh::getIndexBufferOffset() const
{
	return m_indexBufferPoolInstance.m_bufferOffset;
}

void Wolf::Mesh::draw(const CommandBuffer& commandBuffer, uint32_t cameraIdx, uint32_t instanceCount, uint32_t firstInstance) const
{
	commandBuffer.bindVertexBuffer(*getVertexBuffer(), getVertexBufferOffset());
	commandBuffer.bindIndexBuffer(*getIndexBuffer(), getIndexBufferOffset(), IndexType::U32);

	uint32_t indexCount = m_indexCount;
	commandBuffer.drawIndexed(indexCount, instanceCount, 0, 0, firstInstance);
}

bool Wolf::Mesh::hasVertexBuffer() const
{
	return true;
}
