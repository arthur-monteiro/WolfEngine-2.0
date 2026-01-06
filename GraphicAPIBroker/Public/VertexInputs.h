#pragma once

#include <cstdint>

#include "Formats.h"

namespace Wolf
{
    enum class VertexInputRate { VERTEX, INSTANCE };

    struct VertexInputBindingDescription
    {
        uint32_t binding;
        uint32_t stride;
        VertexInputRate inputRate;
    };

    struct VertexInputAttributeDescription
    {
        uint32_t location;
        uint32_t binding;
        Format format;
        uint32_t offset;
    };
}
