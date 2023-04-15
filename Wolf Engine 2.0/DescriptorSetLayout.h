#pragma once

#include <span>

#include "DescriptorLayout.h"

namespace Wolf
{
	class DescriptorSetLayout
	{
	public:
		DescriptorSetLayout(const std::span<const DescriptorLayout> descriptorLayouts);
		DescriptorSetLayout(const DescriptorSetLayout&) = delete;
		~DescriptorSetLayout();

		[[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }

	private:
		VkDescriptorSetLayout m_descriptorSetLayout;
	};
}
