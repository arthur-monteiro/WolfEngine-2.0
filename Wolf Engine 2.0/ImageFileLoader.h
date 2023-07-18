#pragma once

#include <string>

namespace Wolf
{
	class ImageFileLoader
	{
	public:
		ImageFileLoader(const std::string& filename);
		ImageFileLoader(const ImageFileLoader&) = delete;
		~ImageFileLoader();

		[[nodiscard]] const unsigned char* getPixels() const { return m_pixels; }
		[[nodiscard]] int getWidth() const { return m_width; }
		[[nodiscard]] int getHeight() const { return m_height; }
		[[nodiscard]] int getChannelCount() const { return m_channels; }

	private:
		unsigned char* m_pixels;
		int m_width, m_height, m_channels;
	};
}
