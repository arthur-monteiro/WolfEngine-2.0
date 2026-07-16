#include "RuntimeContext.h"

#include "Debug.h"

Wolf::RuntimeContext* Wolf::g_runtimeContext = nullptr;

Wolf::RuntimeContext::RuntimeContext()
{
#ifndef __ANDROID__
	if (g_runtimeContext)
		Debug::sendCriticalError("Can't instantiate RuntimeContext twice");
#endif

	g_runtimeContext = this;
}
