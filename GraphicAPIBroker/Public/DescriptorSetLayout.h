#pragma once

#include <span>

#include "DescriptorLayout.h"

namespace Wolf
{
	class DescriptorSetLayout
	{
	public:
		static DescriptorSetLayout* createDescriptorSetLayout(const std::span<const DescriptorLayout> descriptorLayouts, uint32_t flags = 0);

		virtual ~DescriptorSetLayout() = default;

		[[nodiscard]] virtual bool needsMultipleDescriptorSets() const = 0;
	};
}
