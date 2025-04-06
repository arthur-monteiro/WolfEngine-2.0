#include "RuntimeContext.h"

#include "Debug.h"

Wolf::RuntimeContext* Wolf::g_runtimeContext = nullptr;

Wolf::RuntimeContext::RuntimeContext()
{
	if (g_runtimeContext)
		Debug::sendCriticalError("Can't instantiate RuntimeContext twice");

	g_runtimeContext = this;
}
