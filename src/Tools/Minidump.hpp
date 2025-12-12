// Header-only minidump helper for AnyFSE
#pragma once

#include <windows.h>
#include <DbgHelp.h>
#include <string>
#include <ctime>

#pragma comment(lib, "Dbghelp.lib")

namespace ToolsEx
{
    inline bool WriteMiniDump(EXCEPTION_POINTERS* pExceptionInfo)
    {
        wchar_t modulePath[MAX_PATH] = {0};
        if (!GetModuleFileNameW(NULL, modulePath, MAX_PATH))
            return false;

        // Derive directory (exe folder) and executable base name
        wchar_t* lastSlash = wcsrchr(modulePath, L'\\');
        std::wstring exeDir = modulePath;
        std::wstring exeName = modulePath;
        if (lastSlash)
        {
            exeDir.assign(modulePath, (size_t)(lastSlash - modulePath));
            exeName = lastSlash + 1;
        }

        // strip extension from exeName
        size_t dot = exeName.rfind(L'.');
        if (dot != std::wstring::npos)
            exeName.erase(dot);

        // ensure dumps directory exists: <exeDir>\dumps
        std::wstring dumpsDir = exeDir + L"\\dumps";
        if (!CreateDirectoryW(dumpsDir.c_str(), NULL))
        {
            DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS)
            {
                // if we cannot create dumps dir, fall back to exeDir
                dumpsDir = exeDir;
            }
        }

        // timestamp
        std::time_t t = std::time(nullptr);
        struct tm tm;
        localtime_s(&tm, &t);

        wchar_t filename[MAX_PATH];
        _snwprintf_s(filename, _countof(filename), _TRUNCATE,
            L"%s-%04d%02d%02d-%02d%02d%02d.dmp",
            exeName.c_str(), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);

        std::wstring fullPath = dumpsDir + L"\\" + filename;

        HANDLE hFile = CreateFileW(fullPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        bool wrote = false;
        if (hFile != INVALID_HANDLE_VALUE)
        {
            MINIDUMP_EXCEPTION_INFORMATION mei;
            mei.ThreadId = GetCurrentThreadId();
            mei.ExceptionPointers = pExceptionInfo;
            mei.ClientPointers = FALSE;

            BOOL ok = MiniDumpWriteDump(
                GetCurrentProcess(),
                GetCurrentProcessId(),
                hFile,
                (MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithFullMemoryInfo),
                pExceptionInfo ? &mei : nullptr,
                nullptr,
                nullptr);

            CloseHandle(hFile);
            wrote = (ok == TRUE);
        }

        // Notify user about the crash and dump path (or failure)
        std::wstring msg;
        if (wrote)
        {
            msg = L"The application has crashed. A minidump was written to:\r\n" + fullPath;
        }
        else
        {
            msg = L"The application has crashed. Failed to write minidump. Attempted path:\r\n" + fullPath;
        }
        MessageBoxW(NULL, msg.c_str(), L"Application Error", MB_OK | MB_ICONERROR);

        return wrote;
    }

    inline LONG WINAPI AnyFSEUnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo)
    {
        WriteMiniDump(pExceptionInfo);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    inline void InstallUnhandledExceptionHandler()
    {
        SetUnhandledExceptionFilter(AnyFSEUnhandledExceptionFilter);
    }
}
