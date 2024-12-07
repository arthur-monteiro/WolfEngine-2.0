#pragma once

#include <Tracy.hpp>

namespace Wolf
{
#ifdef TRACY_ENABLE
#define PROFILE_FUNCTION static constexpr tracy::SourceLocationData profileFunction{ nullptr, __FUNCTION__, __FILE__, 4, 2 }; \
	tracy::ScopedZone profileFunctionScopedZone(&profileFunction, true);

#define PROFILE_SCOPED(name) static constexpr tracy::SourceLocationData profileScoped { name, __FUNCTION__,  __FILE__, 4, 0 }; \
	tracy::ScopedZone profilScopedScopedZone(&profileScoped, true);
#else
#define PROFILE_FUNCTION
#define PROFILE_SCOPED(name)
#endif
}
