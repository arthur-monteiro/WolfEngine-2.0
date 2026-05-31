#pragma once

#include <cstdint>

namespace Wolf
{
    enum AccessFlagBits : uint32_t
    {
        SHADER_READ = 1 << 0,
        SHADER_WRITE = 1 << 1,
        TRANSFER_READ = 1 << 2,
        TRANSFER_WRITE = 1 << 3,
        ACCESS_MAX = 1 << 4
    };
    using AccessFlags = uint32_t;
}