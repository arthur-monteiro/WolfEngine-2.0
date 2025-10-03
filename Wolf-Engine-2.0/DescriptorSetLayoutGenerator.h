#pragma once

#include <vector>

#include <Pipeline.h>

#include "DescriptorLayout.h"

namespace Wolf
{
	class DescriptorSetLayoutGenerator
	{
	public:
		void addUniformBuffer(ShaderStageFlags accessibility, uint32_t binding);
		void addCombinedImageSampler(ShaderStageFlags accessibility, uint32_t binding);
		void addStorageBuffer(ShaderStageFlags accessibility, uint32_t binding);
		void addStorageBuffers(ShaderStageFlags accessibility, uint32_t binding, uint32_t count, VkDescriptorBindingFlags bindingFlags = 0);
		void addStorageImage(ShaderStageFlags accessibility, uint32_t binding);
		void addSampler(ShaderStageFlags accessibility, uint32_t binding);
		void addImages(DescriptorType descriptorType, ShaderStageFlags accessibility, uint32_t binding, uint32_t count, VkDescriptorBindingFlags bindingFlags = 0);
		void addAccelerationStructure(ShaderStageFlags accessibility, uint32_t binding);

		[[nodiscard]] const std::vector<DescriptorLayout>& getDescriptorLayouts() const { return m_descriptorLayouts; }

		void reset() { m_descriptorLayouts.clear(); }

	private:
		std::vector<DescriptorLayout> m_descriptorLayouts;
	};
}
