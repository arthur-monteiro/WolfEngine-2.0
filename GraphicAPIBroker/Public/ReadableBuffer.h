#pragma once

#include <cstdint>
#include <vector>

#include <ResourceUniqueOwner.h>

#include "Buffer.h"

namespace Wolf
{
    class ReadableBuffer
    {
    public:
        ReadableBuffer(uint64_t size, uint32_t additionalUsageFlags);

        [[nodiscard]] const Buffer& getBuffer(uint32_t idx) const { return *m_buffers[idx]; }

    private:
        std::vector<ResourceUniqueOwner<Buffer>> m_buffers;
    };

}
