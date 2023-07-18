#include "Dump.h"

#ifdef _WIN32

#include <windows.h>
#include <Dbghelp.h>
#include <tchar.h>

typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

void createMinidump(struct _EXCEPTION_POINTERS* apExceptionInfo)
{
    HMODULE mhLib = ::LoadLibrary(_T("dbghelp.dll"));
    MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress(mhLib, "MiniDumpWriteDump");

    HANDLE  hFile = ::CreateFile(_T("core.dmp"), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);

    _MINIDUMP_EXCEPTION_INFORMATION ExInfo;
    ExInfo.ThreadId = ::GetCurrentThreadId();
    ExInfo.ExceptionPointers = apExceptionInfo;
    ExInfo.ClientPointers = FALSE;

    pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL);
    ::CloseHandle(hFile);
}

LONG WINAPI Wolf::unhandledExceptionFilter(struct _EXCEPTION_POINTERS* apExceptionInfo)
{
    createMinidump(apExceptionInfo);
    return EXCEPTION_CONTINUE_SEARCH;
}

#endif