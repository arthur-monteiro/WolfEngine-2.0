#pragma once

#include <string>

#include "Image.h"
#include "ImageCompression.h"
#include "MaterialsGPUManager.h"
#include "ResourceNonOwner.h"

namespace Wolf
{
	class TextureSetLoader
	{
	public:
		enum class InputTextureSetLayout
		{
			NO_MATERIAL,
			EACH_TEXTURE_A_FILE
		};

		struct OutputLayout
		{
			ImageCompression::Compression albedoCompression;
		};

		struct TextureSetFileInfoGGX
		{
			std::string name;

			std::string albedo;
			std::string normal;
			std::string roughness;
			std::string metalness;
			std::string ao;
			std::string anisoStrength;
		};
		TextureSetLoader(const TextureSetFileInfoGGX& textureSet, const OutputLayout& outputLayout, bool useCache);

		struct TextureSetFileInfoSixWayLighting
		{
			std::string name;

			std::string tex0; // r = right, g = top, b = back, a = transparency
			std::string tex1; // r = left, g = bottom, b = front, a = unused (emissive ?)
		};
		TextureSetLoader(const TextureSetFileInfoSixWayLighting& textureSet, bool useCache);

		Image* releaseImage(uint32_t idx);
		void assignCache(uint32_t idx, std::vector<unsigned char>& output);

	private:
		std::array<ResourceUniqueOwner<Image>, MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL> m_outputs;
		std::array<std::vector<unsigned char>, MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL> m_imagesData;
		bool m_useCache;

		void loadImageFile(const std::string& filename, VkFormat format, std::vector<ImageCompression::RGBA8>& pixels, std::vector<std::vector<ImageCompression::RGBA8>>& mipLevels, VkExtent3D& outExtent) const;
		void createImageFromData(VkExtent3D extent, VkFormat format, const unsigned char* pixels, const std::vector<const unsigned char*>& mipLevels, uint32_t idx);

		template <typename T>
		void compressAndCreateImage(std::vector<std::vector<ImageCompression::RGBA8>>& mipLevels, const std::vector<ImageCompression::RGBA8>& pixels, VkExtent3D& extent, VkFormat format, const TextureSetFileInfoGGX& material);
	};
}
