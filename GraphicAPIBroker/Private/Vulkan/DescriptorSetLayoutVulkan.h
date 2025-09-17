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
#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
		VkDescriptorSetLayout m_descriptorSetLayout = nullptr;
#else
		VkDescriptorSetLayout m_descriptorSetLayout = 0;
#endif
		bool m_containsUniformBuffer = false;
	};
}
