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
			ImageCompression::Compression normalCompression;
		};

		static constexpr ImageCompression::Compression DEFAULT_ALBEDO_COMPRESSION = ImageCompression::Compression::BC1;
		static constexpr ImageCompression::Compression DEFAULT_NORMAL_COMPRESSION = ImageCompression::Compression::BC5;

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

		void transferImageTo(uint32_t idx, ResourceUniqueOwner<Image>& output);
		[[nodiscard]] const std::string& getOutputSlicesFolder(uint32_t idx) const { return m_outputFolders[idx]; }
		void assignCache(uint32_t idx, std::vector<unsigned char>& output);

	private:
		std::array<ResourceUniqueOwner<Image>, MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL> m_outputImages;
		std::array<std::string, MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL> m_outputFolders;
		std::array<std::vector<unsigned char>, MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL> m_imagesData;
		bool m_useCache;

		Format findFormatFromCompression(ImageCompression::Compression compression, bool sRGB);

		// Load image full on GPU
		[[nodiscard]] bool createImageFileFromCache(const std::string& filename, bool sRGB, ImageCompression::Compression compression, uint32_t imageIdx);
		template <typename PixelType>
		void createImageFileFromSource(const std::string& filename, bool sRGB, ImageCompression::Compression compression, uint32_t imageIdx);

		// Use virtual texture
		template <typename PixelType, typename CompressionType>
		void createSlicedCache(const std::string& filename, bool sRGB, ImageCompression::Compression compression, uint32_t imageIdx);

		void loadImageFile(const std::string& filename, Format format, std::vector<ImageCompression::RGBA8>& pixels, std::vector<std::vector<ImageCompression::RGBA8>>& mipLevels, Extent3D& outExtent) const;
		void loadImageFile(const std::string& filename, Format format, std::vector<ImageCompression::RG32F>& pixels, std::vector<std::vector<ImageCompression::RG32F>>& mipLevels, Extent3D& outExtent) const;
		void createImageFromData(Extent3D extent, Format format, const unsigned char* pixels, const std::vector<const unsigned char*>& mipLevels, uint32_t idx);

		template <typename CompressionType, typename PixelType>
		void compressAndCreateImage(std::vector<std::vector<PixelType>>& mipLevels, const std::vector<PixelType>& pixels, Extent3D& extent, Format format, const std::string& filename, std::fstream& outCacheFile,
			uint32_t imageIdx);
	};
}
