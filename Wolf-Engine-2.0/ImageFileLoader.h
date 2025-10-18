#pragma once

#include <string>

#include <Formats.h>

#include "ImageCompression.h"

namespace Wolf
{
	class ImageFileLoader
	{
	public:
		ImageFileLoader(const std::string& fullFilePath, bool loadFloat = false);
		ImageFileLoader(const ImageFileLoader&) = delete;
		~ImageFileLoader();

		[[nodiscard]] unsigned char* getPixels() const { return m_pixels; }
		[[nodiscard]] uint32_t getWidth() const { return m_width; }
		[[nodiscard]] uint32_t getHeight() const { return m_height; }
		[[nodiscard]] uint32_t getDepth() const { return m_depth; }
		[[nodiscard]] uint32_t getChannelCount() const { return m_channels; }
		[[nodiscard]] ImageCompression::Compression getCompression() const { return m_compression; }
		[[nodiscard]] Format getFormat() const { return m_format; }
		[[nodiscard]] const std::vector<std::vector<uint8_t>>& getMipPixels() const { return m_mipPixels; }

	private:
		void loadDDS(const std::string& fullFilePath);
		bool loadCube(const std::string& fullFilePath);

		unsigned char* m_pixels = nullptr;
		uint32_t m_width, m_height, m_channels;
		uint32_t m_depth = 1;
		ImageCompression::Compression m_compression = ImageCompression::Compression::NO_COMPRESSION;

		// DDS related
		std::vector<std::vector<uint8_t>> m_mipPixels;
		Format m_format = Format::UNDEFINED;
	};
}
