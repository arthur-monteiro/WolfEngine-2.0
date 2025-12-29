#include "NoVertexMesh.h"

Wolf::NoVertexMesh::NoVertexMesh(uint32_t primitiveCount) : m_primitiveCount(primitiveCount)
{
}

void Wolf::NoVertexMesh::draw(const CommandBuffer& commandBuffer, uint32_t cameraIdx, uint32_t instanceCount, uint32_t firstInstance, const NullableResourceNonOwner<Buffer>& overrideIndexBuffer) const
{
    commandBuffer.draw(m_primitiveCount, instanceCount, 0, firstInstance);
}

bool Wolf::NoVertexMesh::hasVertexBuffer() const
{
    return false;
}
