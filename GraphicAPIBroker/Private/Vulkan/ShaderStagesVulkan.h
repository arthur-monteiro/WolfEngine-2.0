#pragma once

#include <vulkan/vulkan_core.h>

#include "../../Public/ShaderStages.h"

namespace Wolf 
{
#ifdef WOLF_VULKAN
	inline VkShaderStageFlagBits wolfStageBitToVkStageBit(ShaderStageFlagBits stage)
	{
		VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		switch (stage) {
		case Wolf::ShaderStageFlagBits::VERTEX:
			shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
			break;
		case ShaderStageFlagBits::TESSELLATION_CONTROL:
			shaderStage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			break;
		case ShaderStageFlagBits::TESSELLATION_EVALUATION:
			shaderStage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			break;
		case ShaderStageFlagBits::GEOMETRY:
			shaderStage = VK_SHADER_STAGE_GEOMETRY_BIT;
			break;
		case ShaderStageFlagBits::FRAGMENT:
			shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		case ShaderStageFlagBits::COMPUTE:
			shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
			break;
		case ShaderStageFlagBits::ALL_GRAPHICS:
			shaderStage = VK_SHADER_STAGE_ALL_GRAPHICS;
			break;
		case ShaderStageFlagBits::ALL:
			shaderStage = VK_SHADER_STAGE_ALL;
			break;
		case ShaderStageFlagBits::RAYGEN:
			shaderStage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			break;
		case ShaderStageFlagBits::ANY_HIT:
			shaderStage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
			break;
		case ShaderStageFlagBits::CLOSEST_HIT:
			shaderStage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			break;
		case ShaderStageFlagBits::MISS:
			shaderStage = VK_SHADER_STAGE_MISS_BIT_KHR;
			break;
		case ShaderStageFlagBits::SHADER_STAGE_MAX:
		default:
			Debug::sendCriticalError("Unhandled shader stage");
		}

		return shaderStage;
	}

	inline VkShaderStageFlags wolfStageFlagsToVkStageFlags(ShaderStageFlags flags)
	{
		VkShaderStageFlags vkShaderStageFlags = 0;

#define ADD_FLAG_IF_PRESENT(flag) if (flags & (flag)) vkShaderStageFlags |= wolfStageBitToVkStageBit(flag)

		for (uint32_t flag = 1; flag < SHADER_STAGE_MAX; flag <<= 1)
			ADD_FLAG_IF_PRESENT(static_cast<ShaderStageFlagBits>(flag));

#undef ADD_FLAG_IF_PRESENT

		return vkShaderStageFlags;
	}
#endif
}
