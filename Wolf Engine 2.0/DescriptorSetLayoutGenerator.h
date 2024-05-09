#pragma once

#include <vector>

#include "DescriptorLayout.h"

namespace Wolf
{
	class DescriptorSetLayoutGenerator
	{
	public:
		void addUniformBuffer(VkShaderStageFlags accessibility, uint32_t binding);
		void addCombinedImageSampler(VkShaderStageFlags accessibility, uint32_t binding);
		void addStorageBuffer(VkShaderStageFlags accessibility, uint32_t binding);
		void addStorageImage(VkShaderStageFlags accessibility, uint32_t binding);
		void addSampler(VkShaderStageFlags accessibility, uint32_t binding);
		void addImages(VkDescriptorType descriptorType, VkShaderStageFlags accessibility, uint32_t binding, uint32_t count, VkDescriptorBindingFlags bindingFlags = 0);
		void addAccelerationStructure(VkShaderStageFlags accessibility, uint32_t binding);

		[[nodiscard]] const std::vector<DescriptorLayout>& getDescriptorLayouts() const { return m_descriptorLayouts; }

		void reset() { m_descriptorLayouts.clear(); }

	private:
		std::vector<DescriptorLayout> m_descriptorLayouts;
	};
}