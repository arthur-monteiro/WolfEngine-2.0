#pragma once

#include <cstdint>

namespace Wolf
{
    enum class RayTracingShaderGroupType { GENERAL, TRIANGLES_HIT_GROUP };
    static constexpr uint32_t SHADER_UNUSED = ~0U;

    struct RayTracingShaderGroupCreateInfo
    {
        RayTracingShaderGroupType type;
        uint32_t generalShader = SHADER_UNUSED;
        uint32_t closestHitShader = SHADER_UNUSED;
        uint32_t anyHitShader = SHADER_UNUSED;
        uint32_t intersectionShader = SHADER_UNUSED;
    };
}
