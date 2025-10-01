R"(
uint computeVirtualTextureIndirectionId(uint sliceX, uint sliceY, uint sliceCountX, uint sliceCountY, uint mipLevel)
{
    if (mipLevel == 0)
        return sliceX + sliceY * sliceCountX;

    uint id = 0;

    uint maxMip = uint(log2(sliceCountX));

    if (mipLevel > maxMip)
    {
        id += mipLevel - maxMip;
        mipLevel = maxMip;
    }
    return id + (4 * sliceCountX * sliceCountY - (sliceCountX >> (mipLevel - 1)) * (sliceCountY >> (mipLevel - 1))) / 3 + sliceX + sliceY * (sliceCountX >> mipLevel);
}

uint computeMipCount(uint width, uint height)
{
	return uint(floor(log2(max(width, height)))) - 1; // remove 2 mip levels as min size must be 4x4
}
)"