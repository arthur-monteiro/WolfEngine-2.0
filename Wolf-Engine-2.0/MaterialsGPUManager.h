#pragma once

#include <mutex>

#include <glm/glm.hpp>

#include <Buffer.h>
#include <DescriptorSetLayout.h>
#include <Sampler.h>

#include "DescriptorSetGenerator.h"
#include "LazyInitSharedResource.h"
#include "ResourceUniqueOwner.h"
#include "VirtualTextureManager.h"

namespace Wolf
{
	class Pipeline;

	class MaterialsGPUManager
	{
	public:
		static constexpr uint32_t TEXTURE_COUNT_PER_MATERIAL = 3;

		MaterialsGPUManager(const std::vector<DescriptorSetGenerator::ImageDescription>& firstImages);

		struct TextureSetInfo
		{
			std::array<ResourceUniqueOwner<Image>, TEXTURE_COUNT_PER_MATERIAL> images;
			std::array<std::string, TEXTURE_COUNT_PER_MATERIAL> slicesFolders;

			enum class SamplingMode { TEXTURE_COORDS = 0, TRIPLANAR = 1 };
			SamplingMode samplingMode = SamplingMode::TEXTURE_COORDS;

			glm::vec3 triplanarScale = glm::vec3(1.0f);

#ifdef MATERIAL_DEBUG
			std::string name;
			std::vector<std::string> imageNames;
			std::string materialFolder;
#endif
		};
		void addNewTextureSet(const TextureSetInfo& textureSetInfo);

		struct MaterialInfo
		{
			static constexpr uint32_t MAX_TEXTURE_SET_PER_MATERIAL = 4;

			struct PerTextureSetInMaterialInfo
			{
				uint32_t textureSetIdx = 0;
				float strength = 1.0f;
			};
			std::array<PerTextureSetInMaterialInfo, MAX_TEXTURE_SET_PER_MATERIAL> textureSetInfos;

			enum class ShadingMode : uint32_t
			{
				GGX = 0,
				AnisoGGX = 1,
				SixWaysLighting = 2
			};
			ShadingMode shadingMode = ShadingMode::GGX;
		};
		void addNewMaterial(const MaterialInfo& material);
		void updateBeforeFrame();
		void resize(Extent2D newExtent);

		void lockTextureSets();
		void unlockTextureSets();

		void lockMaterials();
		void unlockMaterials();

		void bind(const CommandBuffer& commandBuffer, const Pipeline& pipeline, uint32_t descriptorSlot) const;
		[[nodiscard]] static ResourceUniqueOwner<DescriptorSetLayout>& getDescriptorSetLayout() { return LazyInitSharedResource<DescriptorSetLayout, MaterialsGPUManager>::getResource(); }
		[[nodiscard]] const DescriptorSet* getDescriptorSet() const { return m_descriptorSet.get(); }

		uint32_t getCurrentMaterialCount() const { return m_currentMaterialCount + static_cast<uint32_t>(m_newMaterialInfo.size()); }
		uint32_t getCurrentTextureSetCount() const { return m_currentTextureSetCount + static_cast<uint32_t>(m_newTextureSetsInfo.size()); }

#ifdef MATERIAL_DEBUG
		void changeMaterialShadingModeBeforeFrame(uint32_t materialIdx, uint32_t newShadingMode);
		void changeTextureSetIdxBeforeFrame(uint32_t materialIdx, uint32_t indexOfTextureSetInMaterial, uint32_t newTextureSetIdx);
		void changeStrengthBeforeFrame(uint32_t materialIdx, uint32_t indexOfTextureSetInMaterial, float newStrength);

		struct TextureSetCacheInfo
		{
			uint32_t textureSetIdx = 0;
			uint32_t albedoIdx = 0;
			uint32_t normalIdx = 1;
			uint32_t roughnessMetalnessAOIdx = 2;

#ifdef MATERIAL_DEBUG
			std::string materialName;
			std::vector<std::string> imageNames;
			std::string materialFolder;
#endif
		};
		std::vector<TextureSetCacheInfo>& getTextureSetsCacheInfo() { return m_textureSetsCacheInfo; }
		void changeExistingTextureSetBeforeFrame(TextureSetCacheInfo& textureSetCacheInfo, const TextureSetInfo& textureSetInfo);
		void changeSamplingModeBeforeFrame(uint32_t textureSetIdx, TextureSetInfo::SamplingMode newSamplingMode);
		void changeScaleBeforeFrame(uint32_t textureSetIdx, glm::vec3 newScale);
#endif

	private:
		uint32_t addImagesToBindless(const std::vector<DescriptorSetGenerator::ImageDescription>& images);
		void updateImageInBindless(const DescriptorSetGenerator::ImageDescription& image, uint32_t bindlessOffset) const;
		static uint32_t computeSliceCount(uint32_t textureWidth, uint32_t textureHeight);

		struct TextureCPUInfo
		{
			std::string m_slicesFolder;

			enum class TextureType { ALBEDO, NORMAL, COMBINED_ROUGHNESS_METALNESS_AO, UNDEFINED };
			TextureType m_textureType = TextureType::UNDEFINED;

			uint32_t m_width;
			uint32_t m_height;

			uint32_t m_virtualTextureIndirectionOffset;

			TextureCPUInfo() = default;
			TextureCPUInfo(const std::string& slicesFolder, TextureType textureType, uint32_t width, uint32_t height, uint32_t virtualTextureIndirectionOffset)
			: m_slicesFolder(slicesFolder), m_textureType(textureType), m_width(width), m_height(height), m_virtualTextureIndirectionOffset(virtualTextureIndirectionOffset)
			{

			}
		};
		std::vector<TextureCPUInfo> m_texturesCPUInfo;
		void addSlicedImage(const std::string& folder, TextureCPUInfo::TextureType textureType);

		// Bindless resources
		static constexpr uint32_t MAX_IMAGES = 1000;
		static constexpr uint32_t BINDING_SLOT = 0;

		uint32_t m_currentBindlessCount = 0;

		// Material layout
		static constexpr uint32_t MAX_MATERIAL_COUNT = 4096;
		struct MaterialGPUInfo
		{
			std::array<uint32_t, MaterialInfo::MAX_TEXTURE_SET_PER_MATERIAL> textureSetIndices;
			std::array<float, MaterialInfo::MAX_TEXTURE_SET_PER_MATERIAL> strengths;
			uint32_t shadingMode = 0;
		};
		uint32_t m_currentMaterialCount = 0;

		// Texture set layout
		static constexpr uint32_t MAX_TEXTURE_SET_COUNT = MAX_MATERIAL_COUNT * MaterialInfo::MAX_TEXTURE_SET_PER_MATERIAL;
		struct TextureSetGPUInfo
		{
			uint32_t albedoIdx = 0;
			uint32_t normalIdx = 1;
			uint32_t roughnessMetalnessAOIdx = 2;
			uint32_t samplingMode = 0;

			glm::vec3 scale = glm::vec3(1.0f); // for tex coords or triplanar
			uint32_t pad;
		};
		uint32_t m_currentTextureSetCount = 0;

		// Texture layout (used for virtual texture)
		static constexpr uint32_t MAX_TEXTURE_COUNT = MAX_TEXTURE_SET_COUNT * TEXTURE_COUNT_PER_MATERIAL;
		struct TextureGPUInfo
		{
			uint32_t width;
			uint32_t height;

			uint32_t virtualTextureIndirectionOffset;
		};
		uint32_t m_currentTextureInfoCount = 0;

		// Info to add
		std::vector<MaterialGPUInfo> m_newMaterialInfo;
		std::mutex m_materialsMutex;
		std::vector<TextureSetGPUInfo> m_newTextureSetsInfo;
		std::mutex m_textureSetsMutex;
		std::vector<TextureGPUInfo> m_newTextureInfo;
		std::mutex m_textureInfoMutex;

		// GPU resources
		ResourceUniqueOwner<Buffer> m_materialsBuffer;
		ResourceUniqueOwner<Buffer> m_textureSetsBuffer;
		ResourceUniqueOwner<Buffer> m_texturesInfoBuffer;
		std::unique_ptr<LazyInitSharedResource<DescriptorSetLayout, MaterialsGPUManager>> m_descriptorSetLayout;
		std::unique_ptr<DescriptorSet> m_descriptorSet;

		std::unique_ptr<Sampler> m_sampler;

		// Virtual Texture
		ResourceUniqueOwner<VirtualTextureManager> m_virtualTextureManager;
		ResourceUniqueOwner<Sampler> m_virtualTextureSampler;
		VirtualTextureManager::AtlasIndex m_albedoAtlasIdx = -1;
		VirtualTextureManager::AtlasIndex m_normalAtlasIdx = -1;

		// Debug cache
#ifdef MATERIAL_DEBUG
		std::vector<TextureSetCacheInfo> m_textureSetsCacheInfo;
#endif
	};
}
