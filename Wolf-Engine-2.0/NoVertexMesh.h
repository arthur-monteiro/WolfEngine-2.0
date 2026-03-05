#pragma once

#include <cstdint>

#include "BoundingSphere.h"
#include "CommandBuffer.h"
#include "MeshInterface.h"

namespace Wolf
{
    class NoVertexMesh : public MeshInterface
    {
    public:
        NoVertexMesh(uint32_t primitiveCount);

        void draw(const CommandBuffer& commandBuffer, uint32_t cameraIdx, uint32_t instanceCount = 1, uint32_t firstInstance = 0,
            const NullableResourceNonOwner<Buffer>& overrideIndexBuffer = NullableResourceNonOwner<Buffer>()) const override;

        [[nodiscard]] bool hasVertexBuffer() const override;
        [[nodiscard]] NullableResourceNonOwner<Buffer> getVertexBuffer() const override;
        [[nodiscard]] uint32_t getVertexBufferOffset() const override { return 0; }
        [[nodiscard]] uint32_t getVertexSize() const override { return 0; }
        [[nodiscard]] NullableResourceNonOwner<Buffer> getIndexBuffer() const override;
        [[nodiscard]] uint32_t getIndexBufferOffset() const override { return 0; }
        [[nodiscard]] uint32_t getIndexSize() const override { return 0; }
        [[nodiscard]] uint32_t getIndexCount() const override { return m_primitiveCount; }

        [[nodiscard]] const BoundingSphere& getBoundingSphere() const override { return m_boundingSphere; }

        void setBoundingSphere(const BoundingSphere& boundingSphere) { m_boundingSphere = boundingSphere; }

    private:
        uint32_t m_primitiveCount;

        BoundingSphere m_boundingSphere = BoundingSphere(glm::vec3(0.0f), 0.0f);
    };
}
