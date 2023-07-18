#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

namespace Wolf
{
#ifdef _WIN32
	LONG WINAPI unhandledExceptionFilter(struct _EXCEPTION_POINTERS* apExceptionInfo);
#endif // WIN32
}
