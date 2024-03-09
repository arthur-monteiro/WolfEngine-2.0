#pragma once

#include "BindlessDescriptor.h"
#include "ResourceNonOwner.h"
#include "ResourceUniqueOwner.h"

namespace Wolf
{
	class MaterialsGPUManager
	{
	public:
		MaterialsGPUManager(const std::vector<DescriptorSetGenerator::ImageDescription>& firstImages);

		void addNewMaterials(const std::vector<DescriptorSetGenerator::ImageDescription>& images);
		void pushMaterialsToGPU();

		void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t descriptorSlot) const;
		[[nodiscard]] static VkDescriptorSetLayout getDescriptorSetLayout() { return LazyInitSharedResource<DescriptorSetLayout, BindlessDescriptor>::getResource()->getDescriptorSetLayout(); }
		[[nodiscard]] const DescriptorSet* getDescriptorSet() const { return m_descriptorSet.get(); }

		uint32_t getCurrentMaterialCount() const { return m_currentMaterialCount + static_cast<uint32_t>(m_newMaterialsInfo.size()); }

	private:
		uint32_t addImagesToBindless(const std::vector<DescriptorSetGenerator::ImageDescription>& images);

		// Bindless resources
		static constexpr uint32_t MAX_IMAGES = 1000;
		static constexpr uint32_t BINDING_SLOT = 0;

		uint32_t m_currentBindlessCount = 0;

		// Material layout
		static constexpr uint32_t MAX_MATERIAL_COUNT = 1024;
		struct MaterialInfo
		{
			uint32_t albedoIdx;
			uint32_t normalIdx;
			uint32_t roughnessIdx;
			uint32_t metalnessIdx;
			uint32_t aoIdx;
		};
		uint32_t m_currentMaterialCount = 0;

		// Info to add
		std::vector<MaterialInfo> m_newMaterialsInfo;

		// GPU resources
		ResourceUniqueOwner<Buffer> m_materialsBuffer;
		std::unique_ptr<LazyInitSharedResource<DescriptorSetLayout, BindlessDescriptor>> m_descriptorSetLayout;
		std::unique_ptr<DescriptorSet> m_descriptorSet;

		std::unique_ptr<Sampler> m_sampler;
	};
}
