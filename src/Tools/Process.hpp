#pragma once
#include <string>
#include <set>
#include <windows.h>

namespace AnyFSE::Tools::Process
{
    DWORD FindFirstByName(const std::wstring& processName);
    HRESULT Kill(const std::wstring& processName);
    HRESULT Kill(DWORD processId);
    DWORD Start(const std::wstring &command, const std::wstring &arguments);
    HWND  GetWindow(const std::set<DWORD>& processIds, const std::wstring& title);
    size_t FindAllByName(const std::wstring &processName, std::set<DWORD> & result);
}

namespace Process = AnyFSE::Tools::Process;