#pragma once

#include "DescriptorSet.h"
#include "DescriptorSetGenerator.h"
#include "DescriptorSetLayout.h"
#include "LazyInitSharedResource.h"
#include "Sampler.h"

namespace Wolf
{
	class Image;

	class BindlessDescriptor
	{
	public:
		BindlessDescriptor(const std::vector<DescriptorSetGenerator::ImageDescription>& firstImages);

		uint32_t addImages(const std::vector<DescriptorSetGenerator::ImageDescription>& images);
		[[nodiscard]] uint32_t getCurrentCounter() const { return m_currentCounter; }

		void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t descriptorSlot) const;
		static VkDescriptorSetLayout getDescriptorSetLayout() { return LazyInitSharedResource<DescriptorSetLayout, BindlessDescriptor>::getResource()->getDescriptorSetLayout(); }
		const DescriptorSet* getDescriptorSet() const { return m_descriptorSet.get(); }

	private:
		static constexpr uint32_t MAX_IMAGES = 1000;
		static constexpr uint32_t BINDING_SLOT = 0;
		
		std::unique_ptr<LazyInitSharedResource<DescriptorSetLayout, BindlessDescriptor>> m_descriptorSetLayout;
		std::unique_ptr<DescriptorSet> m_descriptorSet;

		std::unique_ptr<Sampler> m_sampler;

		uint32_t m_currentCounter = 0;
	};
}
