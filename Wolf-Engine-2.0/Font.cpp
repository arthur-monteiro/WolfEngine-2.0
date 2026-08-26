#include "Font.h"

#ifdef __ANDROID__

#include <Debug.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "AndroidCacheHelper.h"

Wolf::Font::Font(const std::string& path, int ySize)
{
    const std::string& inputFilename = path;
    std::string outputFilename = path;
#ifdef __ANDROID__
    Wolf::copyCompressedFileToStorage(inputFilename, "bin_cache", outputFilename);
#endif

    m_maxYSize = ySize;

    uint32_t texWidth(0), texHeight(0), texChannels(0);
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
        Wolf::Debug::sendCriticalError("FreeType init failed");

    FT_Face face;
    if (FT_New_Face(ft, outputFilename.c_str(), 0, &face))
        Wolf::Debug::sendCriticalError("Font loading failed");

    FT_Set_Pixel_Sizes(face, 0, ySize);

    std::wstring extendedChars = L"abcdefghijklmnopqrstuvwxyz"
                                 L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 L"0123456789"
                                 L"éèêëàâùûôîïçÉÈÊËÀÂÙÛÔÎÏÇ" // French
                                 L"áéíóúüñÁÉÍÓÚÜÑ¿¡"         // Spanish (unique: á, í, ó, ú, ñ, ¿, ¡)
                                 L"åäöÅÄÖ"                   // Swedish / Finnish (å, ä, ö)
                                 L"!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    for (wchar_t c : extendedChars)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            Wolf::Debug::sendCriticalError("character loading failed");

        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) /* convert to an anti-aliased bitmap */
            Wolf::Debug::sendCriticalError("Render glyph failed");

        texWidth = face->glyph->bitmap.width;
        texHeight = face->glyph->bitmap.rows;

        if (texWidth == 0 || texHeight == 0)
            Wolf::Debug::sendCriticalError("Create pixel from character failed");

        auto* pixels = new unsigned char[static_cast<size_t>(texWidth) * texHeight];
        memcpy(pixels, face->glyph->bitmap.buffer, static_cast<size_t>(texWidth) * texHeight);

        m_characters[c].m_xSize = texWidth;
        m_characters[c].m_ySize = texHeight;
        m_characters[c].m_bearingX = face->glyph->bitmap_left;
        m_characters[c].m_bearingY = face->glyph->bitmap_top;

        CreateImageInfo createImageInfo{};
        createImageInfo.extent = { texWidth, texHeight, 1 };
        createImageInfo.format = Format::R8_UNORM;
        createImageInfo.usage = ImageUsageFlagBits::TRANSFER_DST | ImageUsageFlagBits::SAMPLED;
        m_images.emplace_back().reset(Image::createImage(createImageInfo));
        m_images.back()->copyCPUBuffer(pixels, Wolf::Image::SampledInFragmentShader());

        m_characters[c].m_imageIdx = static_cast<unsigned int>(m_images.size() - 1);

        delete[] pixels;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

Wolf::ResourceNonOwner<Wolf::Image> Wolf::Font::getCharacterImage(uint32_t characterIdx)
{
    return m_images[characterIdx].createNonOwnerResource();
}

#endif