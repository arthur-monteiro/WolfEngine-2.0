#pragma once

#ifdef WOLF_USE_VULKAN
#include <vulkan/vulkan.h>
#endif

namespace Wolf
{
#ifdef WOLF_USE_VULKAN
	using ImageView = void*;
#else

#endif

}
