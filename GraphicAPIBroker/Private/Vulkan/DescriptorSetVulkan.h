#pragma once

#include <vulkan/vulkan_core.h>

#include "../../Public/DescriptorSet.h"

namespace Wolf
{
	
	class DescriptorSetVulkan : public DescriptorSet
	{
	public:
		DescriptorSetVulkan(const DescriptorSetLayout& descriptorSetLayout);
		DescriptorSetVulkan(const DescriptorSetVulkan&) = delete;

		void update(const DescriptorSetUpdateInfo& descriptorSetCreateInfo) const override;

		[[nodiscard]] const VkDescriptorSet& getDescriptorSet() const;

	private:
		void initDescriptorSet(const DescriptorSetLayout& descriptorSetLayout);

		std::vector<VkDescriptorSet> m_descriptorSets;
	};
}

