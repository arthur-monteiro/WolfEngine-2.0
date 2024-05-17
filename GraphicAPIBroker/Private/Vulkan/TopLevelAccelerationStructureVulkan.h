#pragma once

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30

#include "AccelerationStructureVulkan.h"
#include "../../Public/TopLevelAccelerationStructure.h"

namespace Wolf
{
	class TopLevelAccelerationStructureVulkan : public AccelerationStructureVulkan, public TopLevelAccelerationStructure
	{
	public:
		TopLevelAccelerationStructureVulkan(std::span<BLASInstance> blasInstances);
		TopLevelAccelerationStructureVulkan(const TopLevelAccelerationStructureVulkan&) = delete;

	private:
		std::unique_ptr<BufferVulkan> m_instanceBuffer;
	};
}

#endif