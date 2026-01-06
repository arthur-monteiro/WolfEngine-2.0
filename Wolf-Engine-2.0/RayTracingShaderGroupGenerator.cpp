#include "RayTracingShaderGroupGenerator.h"

void Wolf::RayTracingShaderGroupGenerator::addRayGenShaderStage(uint32_t shaderIdx)
{
	RayTracingShaderGroupCreateInfo groupInfo{};
	groupInfo.type = RayTracingShaderGroupType::GENERAL;
	groupInfo.generalShader = shaderIdx;
	groupInfo.closestHitShader = SHADER_UNUSED;
	groupInfo.anyHitShader = SHADER_UNUSED;
	groupInfo.intersectionShader = SHADER_UNUSED;
	m_shaderGroups.emplace_back(groupInfo);

	m_rayGenIndex = static_cast<uint32_t>(m_shaderGroups.size() - 1);
}

void Wolf::RayTracingShaderGroupGenerator::addMissShaderStage(uint32_t shaderIdx)
{
	RayTracingShaderGroupCreateInfo groupInfo{};
	groupInfo.type = RayTracingShaderGroupType::GENERAL;
	groupInfo.generalShader = shaderIdx;
	groupInfo.closestHitShader = SHADER_UNUSED;
	groupInfo.anyHitShader = SHADER_UNUSED;
	groupInfo.intersectionShader = SHADER_UNUSED;
	m_shaderGroups.emplace_back(groupInfo);

	m_missIndices.push_back(static_cast<uint32_t>(m_shaderGroups.size()) - 1);
}

void Wolf::RayTracingShaderGroupGenerator::addHitGroup(HitGroup hitGroup)
{
	RayTracingShaderGroupCreateInfo groupInfo{};
	groupInfo.type = RayTracingShaderGroupType::TRIANGLES_HIT_GROUP;
	groupInfo.generalShader = SHADER_UNUSED;
	groupInfo.closestHitShader = hitGroup.closestHitShaderIdx;
	groupInfo.anyHitShader = hitGroup.anyHitShaderIdx;
	groupInfo.intersectionShader = hitGroup.intersectionShaderIdx;
	m_shaderGroups.emplace_back(groupInfo);

	m_hitGroups.push_back(static_cast<uint32_t>(m_shaderGroups.size()) - 1);
}