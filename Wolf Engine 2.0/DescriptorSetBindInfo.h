#pragma once

#include <cstdint>
#include <memory>

#include <ResourceNonOwner.h>

#include <DescriptorSet.h>

namespace Wolf
{
	class DescriptorSetBindInfo
	{
	public:
		DescriptorSetBindInfo(const ResourceNonOwner<const DescriptorSet>& descriptorSet, const ResourceNonOwner<const DescriptorSetLayout>& descriptorSetLayout, uint32_t bindingSlot)
			: m_bindingSlot(bindingSlot), m_descriptorSet(descriptorSet), m_descriptorSetLayout(descriptorSetLayout) {}
		DescriptorSetBindInfo(DescriptorSetBindInfo&&) noexcept = default;
		DescriptorSetBindInfo(const DescriptorSetBindInfo& other)
			: m_bindingSlot(other.m_bindingSlot), m_descriptorSet(other.m_descriptorSet), m_descriptorSetLayout(other.m_descriptorSetLayout)
		{}

		uint32_t getBindingSlot() const { return m_bindingSlot; }
		ResourceNonOwner<const DescriptorSet> getDescriptorSet() const { return m_descriptorSet; }
		ResourceNonOwner<const DescriptorSetLayout> getDescriptorSetLayout() const { return m_descriptorSetLayout; }

	private:
		uint32_t m_bindingSlot;
		ResourceNonOwner<const DescriptorSet> m_descriptorSet;
		ResourceNonOwner<const DescriptorSetLayout> m_descriptorSetLayout;
	};
}
