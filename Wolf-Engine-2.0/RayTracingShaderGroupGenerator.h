#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Wolf
{
	struct HitGroup
	{
		uint32_t closestHitShaderIdx = -1;
		uint32_t anyHitShaderIdx = -1;
		uint32_t intersectionShaderIdx = -1;
	};

	class RayTracingShaderGroupGenerator
	{
	public:
		void addRayGenShaderStage(uint32_t shaderIdx);
		void addMissShaderStage(uint32_t shaderIdx);
		void addHitGroup(HitGroup hitGroup);

		std::vector<VkRayTracingShaderGroupCreateInfoKHR>& getShaderGroups() { return m_shaderGroups; }

	private:
		// Shader groups
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shaderGroups;

		// Indices
		uint32_t m_rayGenIndex;
		std::vector<uint32_t> m_missIndices;
		std::vector<uint32_t> m_hitGroups;
	};
}

