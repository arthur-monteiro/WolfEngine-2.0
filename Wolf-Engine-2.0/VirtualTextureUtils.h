#pragma once

#include <cmath>
#include <cstdint>

[[nodiscard]] inline uint32_t computeVirtualTextureIndirectionId(uint32_t sliceX, uint32_t sliceY, uint32_t sliceCountX, uint32_t sliceCountY, uint32_t mipLevel)
{
    if (mipLevel == 0)
        return sliceX + sliceY * sliceCountX;

    uint32_t id = 0;

    uint32_t maxMip = static_cast<uint32_t>(std::log2(sliceCountX));

    if (mipLevel > maxMip)
    {
        id += mipLevel - maxMip;
        mipLevel = maxMip;
    }
    return id + (4 * sliceCountX * sliceCountY - (sliceCountX >> (mipLevel - 1)) * (sliceCountY >> (mipLevel - 1))) / 3 + sliceX + sliceY * (sliceCountX >> mipLevel);
}