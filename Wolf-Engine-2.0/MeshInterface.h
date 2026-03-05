#pragma once

#include <Buffer.h>
#include <CommandBuffer.h>
#include <ResourceNonOwner.h>

namespace Wolf
{
    class BoundingSphere;

    class MeshInterface
    {
    public:
        virtual ~MeshInterface() = default;

        virtual void draw(const CommandBuffer& commandBuffer, uint32_t cameraIdx, uint32_t instanceCount = 1, uint32_t firstInstance = 0,
            const NullableResourceNonOwner<Buffer>& overrideIndexBuffer = NullableResourceNonOwner<Buffer>()) const = 0;

        virtual bool hasVertexBuffer() const = 0;
        virtual NullableResourceNonOwner<Buffer> getVertexBuffer() const = 0;
        virtual uint32_t getVertexBufferOffset() const = 0;
        virtual uint32_t getVertexSize() const = 0;
        virtual NullableResourceNonOwner<Buffer> getIndexBuffer() const = 0;
        virtual uint32_t getIndexBufferOffset() const = 0;
        virtual uint32_t getIndexSize() const = 0;
        virtual uint32_t getIndexCount() const = 0;

        virtual const BoundingSphere& getBoundingSphere() const = 0;
    };
}
