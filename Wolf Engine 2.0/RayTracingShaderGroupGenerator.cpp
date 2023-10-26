#include "RayTracingShaderGroupGenerator.h"

void Wolf::RayTracingShaderGroupGenerator::addRayGenShaderStage(uint32_t shaderIdx)
{
	VkRayTracingShaderGroupCreateInfoKHR groupInfo{};
	groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	groupInfo.pNext = nullptr;
	groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	groupInfo.generalShader = shaderIdx;
	groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
	m_shaderGroups.emplace_back(groupInfo);

	m_rayGenIndex = static_cast<uint32_t>(m_shaderGroups.size() - 1);
}

void Wolf::RayTracingShaderGroupGenerator::addMissShaderStage(uint32_t shaderIdx)
{
	VkRayTracingShaderGroupCreateInfoKHR groupInfo{};
	groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	groupInfo.pNext = nullptr;
	groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	groupInfo.generalShader = shaderIdx;
	groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
	groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
	groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
	m_shaderGroups.emplace_back(groupInfo);

	m_missIndices.push_back(static_cast<uint32_t>(m_shaderGroups.size()) - 1);
}

void Wolf::RayTracingShaderGroupGenerator::addHitGroup(HitGroup hitGroup)
{
	VkRayTracingShaderGroupCreateInfoKHR groupInfo{};
	groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	groupInfo.pNext = nullptr;
	groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	groupInfo.generalShader = VK_SHADER_UNUSED_KHR;
	groupInfo.closestHitShader = hitGroup.closestHitShaderIdx;
	groupInfo.anyHitShader = hitGroup.anyHitShaderIdx;
	groupInfo.intersectionShader = hitGroup.intersectionShaderIdx;
	m_shaderGroups.emplace_back(groupInfo);

	m_hitGroups.push_back(static_cast<uint32_t>(m_shaderGroups.size()) - 1);
}