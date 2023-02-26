#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Wolf
{
	class ImageFileLoader
	{
	public:
		ImageFileLoader(const std::string& filename);
		ImageFileLoader(const ImageFileLoader&) = delete;
		~ImageFileLoader();

		const unsigned char* getPixels() const { return m_pixels; }
		int getWidth() const { return m_width; }
		int getHeight() const { return m_height; }
		int getChannelCount() const { return m_channels; }

	private:
		unsigned char* m_pixels;
		int m_width, m_height, m_channels;
	};
}
