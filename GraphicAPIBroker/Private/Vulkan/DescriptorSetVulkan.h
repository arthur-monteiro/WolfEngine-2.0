#pragma once

#include "../../Public/DescriptorSet.h"

namespace Wolf
{
	
	class DescriptorSetVulkan : public DescriptorSet
	{
	public:
		DescriptorSetVulkan(const DescriptorSetLayout& descriptorSetLayout);
		DescriptorSetVulkan(const DescriptorSetVulkan&) = delete;

		void update(const DescriptorSetUpdateInfo& descriptorSetCreateInfo) const override;

		[[nodiscard]] const VkDescriptorSet* getDescriptorSet(uint32_t idx = 0) const { return &m_descriptorSet; }

	private:
		void initDescriptorSet(const DescriptorSetLayout& descriptorSetLayout);

		VkDescriptorSet m_descriptorSet;
	};
}

