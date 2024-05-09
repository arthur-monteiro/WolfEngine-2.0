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

	private:
		VkDescriptorSetLayout m_descriptorSetLayout;
	};
}
