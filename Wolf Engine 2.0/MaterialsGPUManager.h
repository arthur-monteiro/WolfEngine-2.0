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

		struct MaterialInfo
		{
			std::array<ResourceUniqueOwner<Image>, TEXTURE_COUNT_PER_MATERIAL> images;
			uint32_t shadingMode;

#ifdef MATERIAL_DEBUG
			std::string materialName;
			std::vector<std::string> imageNames;
			std::string materialFolder;
#endif
		};
		void addNewMaterials(std::vector<MaterialInfo>& materials);
		void pushMaterialsToGPU();

		void bind(const CommandBuffer& commandBuffer, const Pipeline& pipeline, uint32_t descriptorSlot) const;
		[[nodiscard]] static ResourceUniqueOwner<DescriptorSetLayout>& getDescriptorSetLayout() { return LazyInitSharedResource<DescriptorSetLayout, BindlessDescriptor>::getResource(); }
		[[nodiscard]] const DescriptorSet* getDescriptorSet() const { return m_descriptorSet.get(); }

		uint32_t getCurrentMaterialCount() const { return m_currentMaterialCount + static_cast<uint32_t>(m_newMaterialsInfo.size()); }

#ifdef MATERIAL_DEBUG
		void changeMaterialShadingModeBeforeFrame(uint32_t materialIdx, uint32_t newShadingMode) const;

		struct MaterialCacheInfo
		{
			uint32_t albedoIdx = 0;
			uint32_t normalIdx = 1;
			uint32_t roughnessMetalnessAOIdx = 2;

			MaterialInfo* materialInfo = nullptr;
		};
		std::vector<MaterialCacheInfo>& getMaterialsCacheInfo() { return m_materialsInfoCache; }
		void changeExistingMaterialBeforeFrame(uint32_t materialIdx, MaterialCacheInfo& materialCacheInfo);
#endif

	private:
		uint32_t addImagesToBindless(const std::vector<DescriptorSetGenerator::ImageDescription>& images);
		void updateImageInBindless(const DescriptorSetGenerator::ImageDescription& image, uint32_t bindlessOffset) const;

		// Bindless resources
		static constexpr uint32_t MAX_IMAGES = 1000;
		static constexpr uint32_t BINDING_SLOT = 0;

		uint32_t m_currentBindlessCount = 0;

		// Material layout
		static constexpr uint32_t MAX_MATERIAL_COUNT = 1024;
		struct MaterialGPUInfo
		{
			uint32_t albedoIdx = 0;
			uint32_t normalIdx = 1;
			uint32_t roughnessMetalnessAOIdx = 2;

			uint32_t shadingMode = 0;
		};
		uint32_t m_currentMaterialCount = 0;

		// Info to add
		std::vector<MaterialGPUInfo> m_newMaterialsInfo;

		// GPU resources
		ResourceUniqueOwner<Buffer> m_materialsBuffer;
		std::unique_ptr<LazyInitSharedResource<DescriptorSetLayout, BindlessDescriptor>> m_descriptorSetLayout;
		std::unique_ptr<DescriptorSet> m_descriptorSet;

		std::unique_ptr<Sampler> m_sampler;

		// Debug cache
#ifdef MATERIAL_DEBUG
		std::vector<MaterialCacheInfo> m_materialsInfoCache;
#endif
	};
}
