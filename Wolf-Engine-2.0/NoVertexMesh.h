#pragma once

#include <cstdint>

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

        bool hasVertexBuffer() const override;

    private:
        uint32_t m_primitiveCount;
    };
}
