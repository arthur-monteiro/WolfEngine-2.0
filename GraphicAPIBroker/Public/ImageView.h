#pragma once

#include <cstdint>

namespace Wolf
{
#ifdef WOLF_VULKAN
#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
	using ImageView = void*;
	constexpr ImageView IMAGE_VIEW_NULL = nullptr;
#else
	using ImageView = uint64_t;
	constexpr ImageView IMAGE_VIEW_NULL = 0;
#endif
#else

#endif

}
