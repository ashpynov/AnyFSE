#pragma once
#include <string>
#include <windows.h>

namespace AnyFSE::Process
{
    DWORD FindByName(const std::wstring& processName);
    HRESULT Kill(const std::wstring& processName);
    HRESULT Kill(DWORD processId);
    DWORD Start(const std::wstring &command, const std::wstring &arguments);
    HWND  FindWindow(DWORD processId, const std::string& title);
}

namespace Process = AnyFSE::Process;