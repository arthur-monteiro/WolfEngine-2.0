#pragma once

#include <vulkan/vulkan_core.h>

#include <Debug.h>

#include "../../Public/AccessFlags.h"

namespace Wolf 
{
#ifdef WOLF_VULKAN
	inline VkAccessFlagBits2 wolfAccessFlagBitToVkStageBit2(AccessFlagBits access)
	{
		VkAccessFlagBits2 accessFlagBits2 = VK_ACCESS_2_NONE;
		switch (access)
		{
		case AccessFlagBits::SHADER_READ:
			accessFlagBits2 = VK_ACCESS_2_SHADER_READ_BIT;
			break;
		case AccessFlagBits::SHADER_WRITE:
			accessFlagBits2 = VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
		default:
			Debug::sendCriticalError("Unhandled access flag");
		}

		return accessFlagBits2;
	}

	inline VkAccessFlags2 wolfAccessFlagsToVkAccessFlags2(AccessFlags flags)
	{
		VkAccessFlags2 vkAccessFlags2 = 0;

#define ADD_FLAG_IF_PRESENT(flag) if (flags & (flag)) vkAccessFlags2 |= wolfAccessFlagBitToVkStageBit2(flag)

		for (uint32_t flag = 1; flag < ACCESS_MAX; flag <<= 1)
			ADD_FLAG_IF_PRESENT(static_cast<AccessFlagBits>(flag));

#undef ADD_FLAG_IF_PRESENT

		return vkAccessFlags2;
	}
#endif
}
