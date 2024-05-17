#pragma once

#ifdef WOLF_VULKAN
#include <vulkan/vulkan.h>
#endif

namespace Wolf
{
#ifdef WOLF_VULKAN
#if (VK_USE_64_BIT_PTR_DEFINES==1)
	using ImageView = void*;
	constexpr ImageView IMAGE_VIEW_NULL = nullptr;
#else
	using ImageView = uint64_t;
	constexpr ImageView IMAGE_VIEW_NULL = 0;
#endif
#else

#endif

}
