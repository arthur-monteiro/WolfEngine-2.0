#pragma once

#include <cstdint>
#include <memory>

#include <ResourceNonOwner.h>
#include <ResourceReference.h>

#include <DescriptorSet.h>
#include <DescriptorSetLayout.h>

namespace Wolf
{
	class DescriptorSetBindInfo
	{
	public:
		DescriptorSetBindInfo(const ResourceNonOwner<const DescriptorSet>& descriptorSet, const ResourceReference<const DescriptorSetLayout>& descriptorSetLayout, uint32_t bindingSlot)
			: m_bindingSlot(bindingSlot), m_descriptorSet(descriptorSet), m_descriptorSetLayout(descriptorSetLayout) {}
		DescriptorSetBindInfo(DescriptorSetBindInfo&&) noexcept = default;
		DescriptorSetBindInfo(const DescriptorSetBindInfo& other)
			: m_bindingSlot(other.m_bindingSlot), m_descriptorSet(other.m_descriptorSet), m_descriptorSetLayout(other.m_descriptorSetLayout)
		{}

		[[nodiscard]] uint32_t getBindingSlot() const { return m_bindingSlot; }
		[[nodiscard]] ResourceNonOwner<const DescriptorSet> getDescriptorSet() const { return m_descriptorSet; }
		[[nodiscard]] ResourceReference<const DescriptorSetLayout> getDescriptorSetLayout() const { return m_descriptorSetLayout; }

		void setBindingSlot(uint32_t bindingSlot) { m_bindingSlot = bindingSlot; }

	private:
		uint32_t m_bindingSlot;
		ResourceNonOwner<const DescriptorSet> m_descriptorSet;
		ResourceReference<const DescriptorSetLayout> m_descriptorSetLayout;
	};
}
