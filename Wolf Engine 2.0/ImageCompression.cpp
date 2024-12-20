#include "ImageCompression.h"

#include <glm/glm.hpp>

#include "Debug.h"

uint64_t Wolf::ImageCompression::BC5::BC5Channel::toUInt64() const
{
    return bitmap;
}

template<> void Wolf::ImageCompression::compress<Wolf::ImageCompression::BC1>(const VkExtent3D& extent, const std::vector<RGBA8>& pixels, std::vector<Wolf::ImageCompression::BC1>& outBlocks)
{
    compressBC1(extent, pixels, outBlocks);
}

template<> void Wolf::ImageCompression::compress<Wolf::ImageCompression::BC1>(const VkExtent3D& extent, const std::vector<RG32F>& pixels, std::vector<Wolf::ImageCompression::BC1>& outBlocks)
{
    Debug::sendCriticalError("Can't compress to BC1 with RG32F pixels");
}

template<> void Wolf::ImageCompression::compress<Wolf::ImageCompression::BC3>(const VkExtent3D& extent, const std::vector<RGBA8>& pixels, std::vector<Wolf::ImageCompression::BC3>& outBlocks)
{
    compressBC3(extent, pixels, outBlocks);
}

template<> void Wolf::ImageCompression::compress<Wolf::ImageCompression::BC3>(const VkExtent3D& extent, const std::vector<RG32F>& pixels, std::vector<Wolf::ImageCompression::BC3>& outBlocks)
{
    Debug::sendCriticalError("Can't compress to BC3 with RG32F pixels");
}

template<> void Wolf::ImageCompression::compress<Wolf::ImageCompression::BC5>(const VkExtent3D& extent, const std::vector<RG32F>& pixels, std::vector<Wolf::ImageCompression::BC5>& outBlocks)
{
    compressBC5(extent, pixels, outBlocks);
}

template<> void Wolf::ImageCompression::compress<Wolf::ImageCompression::BC5>(const VkExtent3D& extent, const std::vector<RGBA8>& pixels, std::vector<Wolf::ImageCompression::BC5>& outBlocks)
{
	Debug::sendCriticalError("Can't compress to BC5 with RGBA8 pixels");
}

uint8_t Wolf::ImageCompression::RGBA8::mergeColor(uint8_t c00, uint8_t c01, uint8_t c10, uint8_t c11)
{
    const float f00 = c00 / 255.0f;
    const float f01 = c01 / 255.0f;
    const float f10 = c10 / 255.0f;
    const float f11 = c11 / 255.0f;

    const float merged0 = (f00 + f01) / 2.0f;
    const float merged1 = (f10 + f11) / 2.0f;

    const float merged = (merged0 + merged1) / 2.0f;

    return static_cast<uint8_t>(merged * 255);
}

Wolf::ImageCompression::RGBA8 Wolf::ImageCompression::RGBA8::mergeBlock(const RGBA8& block00, const RGBA8& block01,
                                                                        const RGBA8& block10, const RGBA8& block11)
{
    const uint8_t r = mergeColor(block00.r, block01.r, block10.r, block11.r);
    const uint8_t g = mergeColor(block00.g, block01.g, block10.g, block11.g);
    const uint8_t b = mergeColor(block00.b, block01.b, block10.b, block11.b);
    const uint8_t a = mergeColor(block00.a, block01.a, block10.a, block11.a);

    return { r, g, b, a };
}

float Wolf::ImageCompression::RGBA32F::mergeFloats(float c00, float c01, float c10, float c11)
{
    const float f00 = c00;
    const float f01 = c01;
    const float f10 = c10;
    const float f11 = c11;

    const float merged0 = (f00 + f01) / 2.0f;
    const float merged1 = (f10 + f11) / 2.0f;

    const float merged = (merged0 + merged1) / 2.0f;

    return merged;
}

float Wolf::ImageCompression::RG32F::operator[](uint32_t idx) const
{
    if (idx > 1)
        Debug::sendCriticalError("Invalid idx");
    return idx == 0 ? r : g;
}

Wolf::ImageCompression::RG32F Wolf::ImageCompression::RG32F::mergeBlock(const RG32F& block00, const RG32F& block01,
                                                                        const RG32F& block10, const RG32F& block11)
{
    const float r = RGBA32F::mergeFloats(block00.r, block01.r, block10.r, block11.r);
    const float g = RGBA32F::mergeFloats(block00.g, block01.g, block10.g, block11.g);

    return { r, g };
}

void Wolf::ImageCompression::compressBC1(const VkExtent3D& extent, const std::vector<RGBA8>& pixels, std::vector<BC1>& outBlocks)
{
	const uint32_t blockCountX = extent.width / 4;
	const uint32_t blockCountY = extent.height / 4;

    outBlocks.resize(static_cast<size_t>(blockCountX) * blockCountY);

    for (uint32_t blockX = 0; blockX < blockCountX; ++blockX)
    {
		for (uint32_t blockY = 0; blockY < blockCountY; ++blockY)
		{
            RGBA8 min(255, 255, 255, 255), max(0, 0, 0, 255);
            for (uint32_t pixelX = blockX * 4; pixelX < (blockX + 1) * 4; ++pixelX)
            {
                for (uint32_t pixelY = blockY * 4; pixelY < (blockY + 1) * 4; ++pixelY)
                {
                    const RGBA8& pixel = pixels[pixelX + pixelY * extent.width];
                    for (uint32_t channel = 0; channel < 3; ++channel)
                    {
                        if (pixel[channel] < min[channel])
                            min[channel] = pixel[channel];

                        if (pixel[channel] > max[channel])
                            max[channel] = pixel[channel];
                    }
                }
            }

            BC1& block = outBlocks[blockX + blockY * blockCountX];
            block = BC1{};

            block.rgb[0] |= (static_cast<uint16_t>(static_cast<float>(min.r) * (31.0f / 255.0f)) & 0x1f) << 11;
            block.rgb[0] |= (static_cast<uint16_t>(static_cast<float>(min.g) * (63.0f / 255.0f)) & 0x3f) << 5;
            block.rgb[0] |= (static_cast<uint16_t>(static_cast<float>(min.b) * (31.0f / 255.0f)) & 0x1f) << 0;

            block.rgb[1] |= (static_cast<uint16_t>(static_cast<float>(max.r) * (31.0f / 255.0f)) & 0x1f) << 11;
            block.rgb[1] |= (static_cast<uint16_t>(static_cast<float>(max.g) * (63.0f / 255.0f)) & 0x3f) << 5;
            block.rgb[1] |= (static_cast<uint16_t>(static_cast<float>(max.b) * (31.0f / 255.0f)) & 0x1f) << 0;

            RGBA8 refs[4];
            refs[0] = min;
            refs[1] = max;
            refs[2] = RGBA8::mixColors(min, max, 1.0f / 2.0f /* 1.0f / 3.0f */);
            //refs[3] = RGBA8::mixColors(min, max, 2.0f / 3.0f);
            refs[3] = RGBA8(0, 0, 0, 255);

            for (uint32_t pixelX = blockX * 4; pixelX < (blockX + 1) * 4; ++pixelX)
            {
                for (uint32_t pixelY = blockY * 4; pixelY < (blockY + 1) * 4; ++pixelY)
                {
                    const RGBA8& pixel = pixels[pixelX + pixelY * extent.width];
                    float minDistance = 1'000;
                    uint8_t minRefIdx = 0;
                    for (uint32_t refIdx = 0; refIdx < 4; ++refIdx)
                    {
	                    const float distance = pixel.computeDistanceWithRef(refs[refIdx]);
                        if (distance < minDistance)
                        {
                            minDistance = distance;
                            minRefIdx = static_cast<uint8_t>(refIdx);
                        }
                    }
                    
                    block.bitmap |= (minRefIdx & 3) << (((pixelX - blockX * 4) + (pixelY - blockY * 4) * 4) * 2);
                }
            }
		}
    }
}

void Wolf::ImageCompression::compressBC3(const VkExtent3D& extent, const std::vector<RGBA8>& pixels, std::vector<BC3>& outBlocks)
{
    std::vector<BC1> bc1Blocks;
    compressBC1(extent, pixels, bc1Blocks);

    outBlocks.resize(bc1Blocks.size());

    const uint32_t blockCountX = extent.width / 4;
    const uint32_t blockCountY = extent.height / 4;

    for (uint32_t blockIdx = 0; blockIdx < outBlocks.size(); ++blockIdx)
    {
        uint32_t blockX = blockIdx % blockCountX;
        uint32_t blockY = blockIdx / blockCountX;

        uint32_t firstPixelX = blockX * 4;
        uint32_t firstPixelY = blockY * 4;

        uint8_t minAlpha = 255, maxAlpha = 0;
        for (uint32_t alphaIdx = 0; alphaIdx < 16; ++alphaIdx)
        {
            uint32_t pixelX = firstPixelX + alphaIdx % 4;
            uint32_t pixelY = firstPixelY + alphaIdx / 4;

            uint8_t alpha = pixels[pixelX + pixelY * extent.width].a;

            if (alpha < minAlpha)
                minAlpha = alpha;
            if (alpha > maxAlpha)
                maxAlpha = alpha;
        }

        uint8_t refs[8];
        refs[0] = maxAlpha;
        refs[1] = minAlpha;
        for (uint32_t refIdx = 2; refIdx < 8; ++refIdx)
        {
            refs[refIdx] = glm::mix(maxAlpha, minAlpha, static_cast<float>(refIdx - 1) / 7);
        }

        uint64_t alphaBitmap = 0;
        for (uint32_t alphaIdx = 0; alphaIdx < 16; ++alphaIdx)
        {
            uint32_t pixelX = firstPixelX + alphaIdx % 4;
            uint32_t pixelY = firstPixelY + alphaIdx / 4;

            uint8_t alpha = pixels[pixelX + pixelY * extent.width].a;

            float minDistance = 1'000;
            uint64_t minRefIdx = 0;
            for (uint32_t refIdx = 0; refIdx < 8; ++refIdx)
            {
                const float distance = glm::abs(static_cast<float>(refs[refIdx] - alpha));
                if (distance < minDistance)
                {
                    minDistance = distance;
                    minRefIdx = static_cast<uint64_t>(refIdx);
                }
            }

            alphaBitmap |= (minRefIdx & 0b111) << alphaIdx * 3;
        }

        outBlocks[blockIdx].alpha[0] = maxAlpha;
        outBlocks[blockIdx].alpha[1] = minAlpha;
        for (uint32_t i = 0; i < 6; ++i)
        {
        	memcpy(&outBlocks[blockIdx].bitmap[i], reinterpret_cast<uint8_t*>(&alphaBitmap) + i , 1);
        }

        outBlocks[blockIdx].bc1 = bc1Blocks[blockIdx];
    }
}

void Wolf::ImageCompression::compressBC5(const VkExtent3D& extent, const std::vector<RG32F>& pixels, std::vector<BC5>& outBlocks)
{
    const uint32_t blockCountX = extent.width / 4;
    const uint32_t blockCountY = extent.height / 4;

    outBlocks.resize(static_cast<size_t>(blockCountX) * blockCountY);

    for (uint32_t blockX = 0; blockX < blockCountX; ++blockX)
    {
        for (uint32_t blockY = 0; blockY < blockCountY; ++blockY)
        {
            float minR(1.0f), maxR(0.0f), minG(1.0f), maxG(0.0f);
            for (uint32_t pixelX = blockX * 4; pixelX < (blockX + 1) * 4; ++pixelX)
            {
                for (uint32_t pixelY = blockY * 4; pixelY < (blockY + 1) * 4; ++pixelY)
                {
                    const RG32F& pixel = pixels[pixelX + pixelY * extent.width];

                    if (pixel.r > maxR)
                        maxR = pixel.r;
                    if (pixel.r < minR)
                        minR = pixel.r;

                    if (pixel.g > maxG)
                        maxG = pixel.g;
                    if (pixel.g < minG)
                        minG = pixel.g;
                }
            }

            BC5& block = outBlocks[blockX + blockY * blockCountX];
            block = BC5{};

            auto computeChannel = [&](uint8_t channelIdx)
                {
                    float min = channelIdx == 0 ? minR : minG;
                    float max = channelIdx == 0 ? maxR : maxG;

                    BC5::BC5Channel& channel = channelIdx == 0 ? block.red : block.green;

                    channel.data.refs[0] = static_cast<uint8_t>((max * 0.5f + 0.5f) * 255.0f);
                    channel.data.refs[1] = static_cast<uint8_t>((min * 0.5f + 0.5f) * 255.0f);

					// min and max must not be equal (https://learn.microsoft.com/fr-fr/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression#bc5)
					if (channel.data.refs[0] == channel.data.refs[1])
					{
                        if (channel.data.refs[0] != 255)
                            channel.data.refs[0]++;
                        else
                            channel.data.refs[1]--;
					}

                    float refs[8];
                    refs[0] = max;
                    refs[1] = min;
                    for (uint32_t i = 2; i < 8; ++i)
                    {
                        refs[i] = glm::mix(max, min, static_cast<float>(i - 1) / 7.0f);
                    }

                    for (uint32_t pixelX = blockX * 4; pixelX < (blockX + 1) * 4; ++pixelX)
                    {
                        for (uint32_t pixelY = blockY * 4; pixelY < (blockY + 1) * 4; ++pixelY)
                        {
                            const RG32F& pixel = pixels[pixelX + pixelY * extent.width];
                            float minDistance = 1'000;
                            uint8_t minRefIdx = 0;
                            for (uint32_t refIdx = 0; refIdx < 8; ++refIdx)
                            {
                                const float distance = glm::distance(pixel[channelIdx], refs[refIdx]);
                                if (distance < minDistance)
                                {
                                    minDistance = distance;
                                    minRefIdx = static_cast<uint8_t>(refIdx);
                                }
                            }

                            uint32_t bitOffset = ((pixelX - blockX * 4) + (pixelY - blockY * 4) * 4) * 3 + 16;
                            uint64_t valueToPush = (minRefIdx & 0b111); // this must be 64 bits
                            channel.bitmap |= valueToPush << bitOffset;
                        }
                    }
                };

            computeChannel(0);
            computeChannel(1);
        }
    }
}

void Wolf::ImageCompression::uncompressImage(Compression compression, const unsigned char* data, VkExtent2D extent, std::vector<RGBA8>& outPixels)
{
    if (compression != Compression::BC1 && compression != Compression::BC3)
    {
        Debug::sendError("Compression type not supported for image uncompressing");
        return;
    }

	outPixels.resize(extent.width * extent.height);

    uint32_t uncompressedPixelOffsetX = 0;
    uint32_t uncompressedPixelOffsetY = 0;

    uint32_t blockSize;
    uint32_t maxIndex;
    if (compression == Compression::BC1)
    {
        blockSize = 8;
        maxIndex = extent.width * extent.height / 2;
    }
    else
    {
        blockSize = 16;
        maxIndex = extent.width * extent.height;
    }

    for (uint32_t i = 0; i < maxIndex; i += blockSize)
    {
        void* block;
        if (compression == Compression::BC1)
        {
            block = new BC1();
            memcpy(block, &data[i], sizeof(BC1));
        }
        else if (compression == Compression::BC2)
        {
            block = new BC2();
            memcpy(block, &data[i], sizeof(BC2));
        }
        else
        {
            block = new BC3();
            memcpy(block, &data[i], sizeof(BC3));
        }

        RGBA8 uncompressedColors[16];

        // Color
        RGBA8 referenceColors[4];
        auto compressedToColor = [](uint16_t compressedColor)
            {
                float b = static_cast<float>(compressedColor & 0x1f);
                b *= 255.0f / 31.0f;

                compressedColor >>= 5;
                float g = static_cast<float>(compressedColor & 0x3f);
                g *= 255.0f / 63.0f;

                compressedColor >>= 6;
                float r = static_cast<float>(compressedColor & 0x1f);
                r *= 255.0f / 31.0f;

                return RGBA8(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), 255);
            };

        if (compression == Compression::BC1)
        {
            referenceColors[0] = compressedToColor(static_cast<BC1*>(block)->rgb[0]);
            referenceColors[1] = compressedToColor(static_cast<BC1*>(block)->rgb[1]);
        }
        else
        {
            referenceColors[0] = compressedToColor(static_cast<BC3*>(block)->bc1.rgb[0]);
            referenceColors[1] = compressedToColor(static_cast<BC3*>(block)->bc1.rgb[1]);
        }

        referenceColors[2] = RGBA8::mixColors(referenceColors[0], referenceColors[1], 1.0f / 3.0f);
        referenceColors[3] = RGBA8::mixColors(referenceColors[0], referenceColors[1], 2.0f / 3.0f);

        uint32_t dw;
        if (compression == Compression::BC1)
        {
            dw = static_cast<BC1*>(block)->bitmap;
        }
        else
        {
            dw = static_cast<BC3*>(block)->bc1.bitmap;
        }

        for (size_t j = 0; j < 16; ++j, dw >>= 2)
        {
            switch (dw & 3)
            {
	            case 0: uncompressedColors[j] = referenceColors[0]; break;
	            case 1: uncompressedColors[j] = referenceColors[1]; break;
	            case 2: uncompressedColors[j] = referenceColors[2]; break;
	            case 3: uncompressedColors[j] = referenceColors[3]; break;
            }
        }

        // Alpha
        if (compression == Compression::BC3)
        {
	        const BC3* bc3Block = static_cast<BC3*>(block);

	        const uint8_t alpha0 = bc3Block->alpha[0];
	        const uint8_t alpha1 = bc3Block->alpha[1];

            uint8_t alphaReferences[8];
            alphaReferences[0] = alpha0;
            alphaReferences[1] = alpha1;

            if (alpha0 > alpha1)
            {
                // 6 interpolated alpha values.
                alphaReferences[2] = static_cast<uint8_t>(glm::mix(static_cast<float>(alpha0) / 255.0f, static_cast<float>(alpha1) / 255.0f, 1.0f / 7.0f) * 255.0f);
                alphaReferences[3] = static_cast<uint8_t>(glm::mix(static_cast<float>(alpha0) / 255.0f, static_cast<float>(alpha1) / 255.0f, 2.0f / 7.0f) * 255.0f);
                alphaReferences[4] = static_cast<uint8_t>(glm::mix(static_cast<float>(alpha0) / 255.0f, static_cast<float>(alpha1) / 255.0f, 3.0f / 7.0f) * 255.0f);
                alphaReferences[5] = static_cast<uint8_t>(glm::mix(static_cast<float>(alpha0) / 255.0f, static_cast<float>(alpha1) / 255.0f, 4.0f / 7.0f) * 255.0f);
                alphaReferences[6] = static_cast<uint8_t>(glm::mix(static_cast<float>(alpha0) / 255.0f, static_cast<float>(alpha1) / 255.0f, 5.0f / 7.0f) * 255.0f);
                alphaReferences[7] = static_cast<uint8_t>(glm::mix(static_cast<float>(alpha0) / 255.0f, static_cast<float>(alpha1) / 255.0f, 6.0f / 7.0f) * 255.0f);
            }
            else
            {
                // 4 interpolated alpha values.
                alphaReferences[2] = static_cast<uint8_t>(glm::mix(static_cast<float>(alpha0) / 255.0f, static_cast<float>(alpha1) / 255.0f, 1.0f / 5.0f) * 255.0f);
                alphaReferences[3] = static_cast<uint8_t>(glm::mix(static_cast<float>(alpha0) / 255.0f, static_cast<float>(alpha1) / 255.0f, 2.0f / 5.0f) * 255.0f);
                alphaReferences[4] = static_cast<uint8_t>(glm::mix(static_cast<float>(alpha0) / 255.0f, static_cast<float>(alpha1) / 255.0f, 3.0f / 5.0f) * 255.0f);
                alphaReferences[5] = static_cast<uint8_t>(glm::mix(static_cast<float>(alpha0) / 255.0f, static_cast<float>(alpha1) / 255.0f, 4.0f / 5.0f) * 255.0f);
                alphaReferences[6] = 0;
                alphaReferences[7] = 255;
            }

            dw = static_cast<uint32_t>(bc3Block->bitmap[0]) | static_cast<uint32_t>(bc3Block->bitmap[1] << 8) | static_cast<uint32_t>(bc3Block->bitmap[2] << 16);

            for (size_t j = 0; j < 8; ++j, dw >>= 3)
                uncompressedColors[j].a = alphaReferences[dw & 0x7];

            dw = static_cast<uint32_t>(bc3Block->bitmap[3]) | static_cast<uint32_t>(bc3Block->bitmap[4] << 8) | static_cast<uint32_t>(bc3Block->bitmap[5] << 16);

            for (size_t j = 8; j < 16; ++j, dw >>= 3)
                uncompressedColors[j].a = alphaReferences[dw & 0x7];
        }
        else if (compression == Compression::BC2)
        {
	        const BC2* bc2Block = static_cast<BC2*>(block);

            for (size_t j = 0; j < 16; ++j)
            {
                if (j % 4 == 0)
                    uncompressedColors[j].a = bc2Block->bitmap[j / 4] & 0x000f;
                else if (j % 4 == 1)
                    uncompressedColors[j].a = (bc2Block->bitmap[j / 4] >> 4) & 0x000f;
                else if (j % 4 == 2)
                    uncompressedColors[j].a = (bc2Block->bitmap[j / 4] >> 8) & 0x000f;
                else if (j % 4 == 3)
                    uncompressedColors[j].a = (bc2Block->bitmap[j / 4] >> 12) & 0x000f;

                uncompressedColors[j].a *= 16;
            }
        }
        else
        {
            for (size_t j = 0; j < 16; ++j)
                uncompressedColors[j].a = 255;
        }

        // Write to uncompressed buffer
        for (int j = 0; j < 16; ++j)
        {
	        const uint32_t blockOffset = uncompressedPixelOffsetX * 4 + uncompressedPixelOffsetY * extent.width * 4;
	        const uint32_t pixelOffset = j % 4 + (j / 4) * extent.width;
            outPixels[blockOffset + pixelOffset] = uncompressedColors[j];
        }

        uncompressedPixelOffsetX++;
        if (uncompressedPixelOffsetX >= extent.width / 4)
        {
            uncompressedPixelOffsetX = 0;
            uncompressedPixelOffsetY++;
        }
    }
}

void Wolf::ImageCompression::uncompressImage(Compression compression, const unsigned char* data, VkExtent2D extent, std::vector<RG8>& outPixels)
{
    if (compression != Compression::BC5)
    {
        Debug::sendError("Compression type not supported for image uncompressing");
        return;
    }

    outPixels.resize(extent.width * extent.height);

    uint32_t uncompressedPixelOffsetX = 0;
    uint32_t uncompressedPixelOffsetY = 0;

    uint32_t blockSize;
    uint32_t maxIndex;
    if (compression == Compression::BC5)
    {
        blockSize = 16;
        maxIndex = extent.width * extent.height;
    }
    else
    {
        Debug::sendError("Compression type not supported");
        return;
    }

    for (uint32_t i = 0; i < maxIndex; i += blockSize)
    {
        void* block;
        if (compression == Compression::BC5)
        {
            block = new BC5();
            memcpy(block, &data[i], sizeof(BC5));
        }
        else
        {
            Debug::sendError("Compression type not supported");
            return;
        }

        RG8 uncompressedColors[16];

        float referencesRed[2];
        float referencesGreen[2];
        auto refToFloat = [](uint8_t ref)
            {
	            const float channel = static_cast<float>(ref) / 255.0f;
                return channel;
            };

        if (compression == Compression::BC5)
        {
            referencesRed[0] = refToFloat(static_cast<BC5*>(block)->red.data.refs[0]);
            referencesRed[1] = refToFloat(static_cast<BC5*>(block)->red.data.refs[1]);

            referencesGreen[0] = refToFloat(static_cast<BC5*>(block)->green.data.refs[0]);
            referencesGreen[1] = refToFloat(static_cast<BC5*>(block)->green.data.refs[1]);
        }
        else
        {
            Debug::sendError("Compression type not supported");
            return;
        }

        uint64_t redDW;
        uint64_t greenDW;
        if (compression == Compression::BC5)
        {
            redDW = static_cast<BC5*>(block)->red.toUInt64();
            greenDW = static_cast<BC5*>(block)->green.toUInt64();
        }
        else
        {
            Debug::sendError("Compression type not supported");
            return;
        }

        for (size_t j = 0; j < 16; ++j, redDW >>= 3)
        {
	        const uint8_t redValue = redDW & 7;
            uncompressedColors[j].r = static_cast<uint8_t>(glm::mix(referencesRed[0], referencesRed[1], static_cast<float>(redValue) / 7.0f) * 255.0f);
        }

        for (size_t j = 0; j < 16; ++j, greenDW >>= 3)
        {
	        const uint8_t greenValue = greenDW & 7;
            uncompressedColors[j].g = static_cast<uint8_t>(glm::mix(referencesGreen[0], referencesGreen[1], static_cast<float>(greenValue) / 7.0f) * 255.0f);
        }

        // Write to uncompressed buffer
        for (int j = 0; j < 16; ++j)
        {
            const uint32_t blockOffset = uncompressedPixelOffsetX * 4 + uncompressedPixelOffsetY * extent.width * 4;
            const uint32_t pixelOffset = j % 4 + (j / 4) * extent.width;
            if (const uint32_t fullPixelOffset = blockOffset + pixelOffset; fullPixelOffset < outPixels.size())
				outPixels[fullPixelOffset] = uncompressedColors[j];
        }

        uncompressedPixelOffsetX++;
        if (uncompressedPixelOffsetX >= extent.width / 4)
        {
            uncompressedPixelOffsetX = 0;
            uncompressedPixelOffsetY++;
        }
    }
}
