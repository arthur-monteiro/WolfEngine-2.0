#pragma once

#include <string>

#include "Image.h"
#include "ImageCompression.h"
#include "MaterialsGPUManager.h"
#include "ResourceNonOwner.h"

namespace Wolf
{
	class MaterialLoader
	{
	public:
		enum class InputMaterialLayout
		{
			NO_MATERIAL,
			EACH_TEXTURE_A_FILE
		};

		struct MaterialFileInfo
		{
			std::string name;

			std::string albedo;
			std::string normal;
			std::string roughness;
			std::string metalness;
			std::string ao;
		};

		MaterialLoader(const MaterialFileInfo& material, InputMaterialLayout materialLayout, bool useCache);
		Image* releaseImage(uint32_t idx);
		void assignCache(uint32_t idx, std::vector<unsigned char>& output);

	private:
		std::array<ResourceUniqueOwner<Image>, MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL> m_outputs;
		std::array<std::vector<unsigned char>, MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL> m_imagesData;
		bool m_useCache;

		void loadImageFile(const std::string& filename, VkFormat format, std::vector<ImageCompression::RGBA8>& pixels, std::vector<std::vector<ImageCompression::RGBA8>>& mipLevels, VkExtent3D& outExtent) const;
		void createImageFromData(VkExtent3D extent, VkFormat format, const unsigned char* pixels, const std::vector<const unsigned char*>& mipLevels, uint32_t idx);
	};
}
