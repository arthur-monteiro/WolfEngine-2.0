#pragma once

#include <mutex>

#include "BindlessDescriptor.h"
#include "ResourceUniqueOwner.h"

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

			enum class SamplingMode { TEXTURE_COORDS = 0, TRIPLANAR = 1 };
			SamplingMode samplingMode = SamplingMode::TEXTURE_COORDS;

			glm::vec3 triplanarScale = glm::vec3(1.0f);

#ifdef MATERIAL_DEBUG
			std::string materialName;
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
		void pushMaterialsToGPU();

		void bind(const CommandBuffer& commandBuffer, const Pipeline& pipeline, uint32_t descriptorSlot) const;
		[[nodiscard]] static ResourceUniqueOwner<DescriptorSetLayout>& getDescriptorSetLayout() { return LazyInitSharedResource<DescriptorSetLayout, MaterialsGPUManager>::getResource(); }
		[[nodiscard]] const DescriptorSet* getDescriptorSet() const { return m_descriptorSet.get(); }

		uint32_t getCurrentMaterialCount() const { return m_currentMaterialCount + static_cast<uint32_t>(m_newMaterialInfo.size()); }
		uint32_t getCurrentTextureSetCount() const { return m_currentTextureSetCount + static_cast<uint32_t>(m_newTextureSetsInfo.size()); }

#ifdef MATERIAL_DEBUG
		void changeMaterialShadingModeBeforeFrame(uint32_t materialIdx, uint32_t newShadingMode) const;
		void changeTextureSetIdxBeforeFrame(uint32_t materialIdx, uint32_t indexOfTextureSetInMaterial, uint32_t newTextureSetIdx) const;
		void changeStrengthBeforeFrame(uint32_t materialIdx, uint32_t indexOfTextureSetInMaterial, float newStrength) const;

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
		void changeSamplingModeBeforeFrame(uint32_t textureSetIdx, TextureSetInfo::SamplingMode newSamplingMode) const;
		void changeTriplanarScaleBeforeFrame(uint32_t textureSetIdx, glm::vec3 newTriplanarScale) const;
#endif

	private:
		uint32_t addImagesToBindless(const std::vector<DescriptorSetGenerator::ImageDescription>& images);
		void updateImageInBindless(const DescriptorSetGenerator::ImageDescription& image, uint32_t bindlessOffset) const;

		// Bindless resources
		static constexpr uint32_t MAX_IMAGES = 1000;
		static constexpr uint32_t BINDING_SLOT = 0;

		uint32_t m_currentBindlessCount = 0;

		// Material layout
		static constexpr uint32_t MAX_TEXTURE_SET_COUNT = 1024;
		struct TextureSetGPUInfo
		{
			uint32_t albedoIdx = 0;
			uint32_t normalIdx = 1;
			uint32_t roughnessMetalnessAOIdx = 2;
			uint32_t samplingMode = 0;

			glm::vec3 triplanarScale = glm::vec3(1.0f); // TODO: this is only used for triplanar materials, can it be in a separated buffer to avoid using 3 * 4 Bytes per texture set?
			uint32_t pad;
		};
		uint32_t m_currentTextureSetCount = 0;

		// Material set layout
		static constexpr uint32_t MAX_MATERIAL_COUNT = 1024;
		struct MaterialGPUInfo
		{
			std::array<uint32_t, MaterialInfo::MAX_TEXTURE_SET_PER_MATERIAL> textureSetIndices;
			std::array<float, MaterialInfo::MAX_TEXTURE_SET_PER_MATERIAL> strengths;
			uint32_t shadingMode = 0;
		};
		uint32_t m_currentMaterialCount = 0;

		// Info to add
		std::vector<TextureSetGPUInfo> m_newTextureSetsInfo;
		std::vector<MaterialGPUInfo> m_newMaterialInfo;

		// GPU resources
		ResourceUniqueOwner<Buffer> m_textureSetsBuffer;
		ResourceUniqueOwner<Buffer> m_materialsBuffer;
		std::unique_ptr<LazyInitSharedResource<DescriptorSetLayout, MaterialsGPUManager>> m_descriptorSetLayout;
		std::unique_ptr<DescriptorSet> m_descriptorSet;

		std::unique_ptr<Sampler> m_sampler;

		// Debug cache
#ifdef MATERIAL_DEBUG
		std::vector<TextureSetCacheInfo> m_textureSetsCacheInfo;
#endif
	};
}
