#pragma once

#include <cstdint>
#include <glm/detail/func_common.hpp>
#include <glm/detail/func_geometric.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Debug.h"

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

            uint8_t operator[](uint32_t index) const
            {
	            switch (index)
	            {
	            	case 0:
                        return r;
	            	case 1:
                        return g;
	            	case 2:
                        return b;
	            	case 3:
                        return a;
	            	default:
                        Debug::sendError("Wrong channel");
                        return 0;
	            }
            }

            uint8_t& operator[](uint32_t index)
            {
                switch (index)
                {
                case 0:
                    return r;
                case 1:
                    return g;
                case 2:
                    return b;
                case 3:
                    return a;
                default:
                    Debug::sendError("Wrong channel");
                    return r;
                }
            }

            static RGBA8 mixColors(const RGBA8& color0, const RGBA8& color1, float ratio)
            {
                RGBA8 color;
                color.r = static_cast<uint8_t>(glm::mix(static_cast<float>(color0.r) / 255.0f, static_cast<float>(color1.r) / 255.0f, ratio) * 255.0f);
                color.g = static_cast<uint8_t>(glm::mix(static_cast<float>(color0.g) / 255.0f, static_cast<float>(color1.g) / 255.0f, ratio) * 255.0f);
                color.b = static_cast<uint8_t>(glm::mix(static_cast<float>(color0.b) / 255.0f, static_cast<float>(color1.b) / 255.0f, ratio) * 255.0f);

                return color;
            }

            float computeDistanceWithRef(const RGBA8& ref) const
            {
	            const glm::vec3 refAsVec(ref.r, ref.g, ref.b);
	            const glm::vec3 thisAsVec(r, g, b);
                return glm::distance(refAsVec, thisAsVec);
            }
        };

        struct RG8
        {
            uint8_t r, g;
            RG8(uint8_t r, uint8_t g) : r(r), g(g) {}
            RG8() : RG8(0, 0) {}
        };

        static void compressBC1(const VkExtent3D& extent, const std::vector<RGBA8>& pixels, std::vector<BC1>& outBlocks);

        static void uncompressImage(Compression compression, const unsigned char* data, VkExtent2D extent, std::vector<RGBA8>& outPixels);
        static void uncompressImage(Compression compression, const unsigned char* data, VkExtent2D extent, std::vector<RG8>& outPixels);
    };
}