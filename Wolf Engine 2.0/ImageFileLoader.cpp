#include "ImageFileLoader.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Debug.h"

Wolf::ImageFileLoader::ImageFileLoader(const std::string& filename)
{
	if (filename[filename.size() - 4] == '.' && filename[filename.size() - 3] == 'h' && filename[filename.size() - 2] == 'd' && filename[filename.size() - 1] == 'r')
	{
		float* pixels = stbi_loadf(filename.c_str(), &m_width, &m_height, &m_channels, STBI_rgb_alpha);
		m_pixels = (unsigned char*)pixels;
		m_channels = 4;

		if (!pixels)
			Debug::sendError("Error loading file " + filename + " !");
	}
	else
	{
		stbi_uc* pixels = stbi_load(filename.c_str(), &m_width, &m_height, &m_channels, STBI_rgb_alpha);
		m_pixels = pixels;
		m_channels = 4;

		if (!pixels)
			Debug::sendError("Error : loading image " + filename);
	}
}

Wolf::ImageFileLoader::~ImageFileLoader()
{
	stbi_image_free(m_pixels);
}
