#pragma once

#ifdef __ANDROID__

#include <map>
#include <string>

#include <Image.h>

#include "DynamicResourceUniqueOwnerArray.h"

namespace Wolf
{
    class Font
    {
    public:
        Font(const std::string& path, int ySize);

        [[nodiscard]] uint32_t getCharacterCount() const { return m_characters.size(); }
        [[nodiscard]] uint32_t getXSize(const wchar_t character) { return m_characters[character].m_xSize; }
        [[nodiscard]] uint32_t getYSize(const wchar_t character) { return m_characters[character].m_ySize; }
        [[nodiscard]] int getBearingX(const wchar_t character) { return m_characters[character].m_bearingX; }
        [[nodiscard]] int getBearingY(const wchar_t character) { return m_characters[character].m_bearingY; }
        int getMaxSizeY() const { return m_maxYSize; }

        uint32_t getImageIdx(wchar_t character) { return m_characters[character].m_imageIdx; }
        Wolf::ResourceNonOwner<Image> getCharacterImage(uint32_t characterIdx);

    private:
        struct Character
        {
            uint32_t m_xSize = 0; // in pixel
            uint32_t m_ySize = 0;
            int m_bearingX = 0;
            int m_bearingY = 0;

            uint32_t m_imageIdx = 0;;
        };

        std::map<wchar_t, Character> m_characters;
        DynamicResourceUniqueOwnerArray<Image> m_images;
        int m_maxYSize;
    };
}

#endif