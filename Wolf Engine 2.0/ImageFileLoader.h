#pragma once

#include <string>

#include "ImageCompression.h"

namespace Wolf
{
	class ImageFileLoader
	{
	public:
		ImageFileLoader(const std::string& fullFilePath);
		ImageFileLoader(const ImageFileLoader&) = delete;
		~ImageFileLoader();

		[[nodiscard]] unsigned char* getPixels() const { return m_pixels; }
		[[nodiscard]] uint32_t getWidth() const { return m_width; }
		[[nodiscard]] uint32_t getHeight() const { return m_height; }
		[[nodiscard]] uint32_t getChannelCount() const { return m_channels; }
		[[nodiscard]] ImageCompression::Compression getCompression() const { return m_compression; }

	private:
		void loadDDS(const std::string& fullFilePath);

		unsigned char* m_pixels = nullptr;
		uint32_t m_width, m_height, m_channels;
		ImageCompression::Compression m_compression = ImageCompression::Compression::NO_COMPRESSION;
	};
}
