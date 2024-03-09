#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Wolf
{
    class ImageCompression
    {
    public:
        struct BC1
        {
            uint16_t    rgb[2]; // 565 colors
            uint32_t    bitmap; // 2bpp rgb bitmap
        };

        static_assert(sizeof(BC1) == 8, "Mismatch block size");

        struct BC2
        {
            uint16_t    bitmap[4];  // 4bpp alpha bitmap
            BC1         bc1;        // BC1 rgb data
        };

        static_assert(sizeof(BC2) == 16, "Mismatch block size");

        struct BC3
        {
            uint8_t     alpha[2];   // alpha values
            uint8_t     bitmap[6];  // 3bpp alpha bitmap
            BC1         bc1;        // BC1 rgb data
        };

        static_assert(sizeof(BC3) == 16, "Mismatch block size");

        struct BC5
        {
#pragma pack(push, 1) 
            struct BC5Channel
            {
                uint8_t refs[2];
                uint32_t bitmap0;
                uint16_t bitmap1;

                uint64_t toUInt64() const;
            };
#pragma pack(pop)

            static_assert(sizeof(BC5Channel) == 8, "Mismatch block size");

            BC5Channel red;
            BC5Channel green;
        };

        static_assert(sizeof(BC5) == 16, "Mismatch block size");

        enum class Compression
        {
            NO_COMPRESSION,
            BC1,
            BC2,
            BC3,
            BC5
        };

        struct RGBA8
        {
            uint8_t r, g, b, a;

            RGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {}
            RGBA8() : RGBA8(0, 0, 0, 0) {}
        };

        struct RG8
        {
            uint8_t r, g;
            RG8(uint8_t r, uint8_t g) : r(r), g(g) {}
            RG8() : RG8(0, 0) {}
        };

        static void uncompressImage(Compression compression, const unsigned char* data, VkExtent2D extent, std::vector<RGBA8>& outPixels);
        static void uncompressImage(Compression compression, const unsigned char* data, VkExtent2D extent, std::vector<RG8>& outPixels);
    };
}
