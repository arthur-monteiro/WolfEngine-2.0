#pragma once

#include <cstdint>

namespace Wolf
{
    enum AccessFlagBits : uint32_t
    {
        SHADER_READ = 1 << 0,
        SHADER_WRITE = 1 << 1,
        ACCESS_MAX = 1 << 2
    };
    using AccessFlags = uint32_t;
}