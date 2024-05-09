#pragma once

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30

#include "../../Public/BottomLevelAccelerationStructure.h"
#include "AccelerationStructureVulkan.h"

namespace Wolf
{
	class BottomLevelAccelerationStructureVulkan : public BottomLevelAccelerationStructure, public AccelerationStructureVulkan
	{
	public:
		BottomLevelAccelerationStructureVulkan(const BottomLevelAccelerationStructureCreateInfo& createInfo);
		BottomLevelAccelerationStructureVulkan(const BottomLevelAccelerationStructure&) = delete;

		~BottomLevelAccelerationStructureVulkan() override = default;

	private:
		std::vector<VkAccelerationStructureGeometryKHR> m_vertexBuffers;
	};
}

#endif