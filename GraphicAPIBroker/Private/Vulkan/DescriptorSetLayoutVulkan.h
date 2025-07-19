#pragma once

#include <span>

#include "../../Public/DescriptorSetLayout.h"

namespace Wolf
{
	VkDescriptorType wolfDescriptorTypeToVkDescriptorType(DescriptorType descriptorLayout);

	class DescriptorSetLayoutVulkan : public DescriptorSetLayout
	{
	public:
		explicit DescriptorSetLayoutVulkan(std::span<const DescriptorLayout> descriptorLayouts, VkDescriptorSetLayoutCreateFlags flags = 0);
		DescriptorSetLayoutVulkan(const DescriptorSetLayout&) = delete;
		~DescriptorSetLayoutVulkan() override;

		[[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }
		[[nodiscard]] bool needsMultipleDescriptorSets() const override { return m_containsUniformBuffer; }

	private:
		VkDescriptorSetLayout m_descriptorSetLayout = nullptr;
		bool m_containsUniformBuffer = false;
	};
}
