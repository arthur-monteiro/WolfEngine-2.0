#pragma once

#include <span>

#include "../../Public/DescriptorSetLayout.h"

namespace Wolf
{
	class DescriptorSetLayoutVulkan : public DescriptorSetLayout
	{
	public:
		DescriptorSetLayoutVulkan(const std::span<const DescriptorLayout> descriptorLayouts, VkDescriptorSetLayoutCreateFlags flags = 0);
		DescriptorSetLayoutVulkan(const DescriptorSetLayout&) = delete;
		~DescriptorSetLayoutVulkan() override;

		[[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }
		[[nodiscard]] bool needsMultipleDescriptorSets() const override { return m_containsUniformBuffer; }

	private:
		VkDescriptorSetLayout m_descriptorSetLayout;
		bool m_containsUniformBuffer = false;
	};
}
