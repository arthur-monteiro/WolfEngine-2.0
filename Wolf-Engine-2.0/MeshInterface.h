#pragma once

namespace Wolf
{
    class MeshInterface
    {
    public:
        virtual ~MeshInterface() = default;

        virtual void draw(const CommandBuffer& commandBuffer, uint32_t cameraIdx, uint32_t instanceCount = 1, uint32_t firstInstance = 0,
            const NullableResourceNonOwner<Buffer>& overrideIndexBuffer = NullableResourceNonOwner<Buffer>()) const = 0;

        virtual bool hasVertexBuffer() const = 0;
    };
}
