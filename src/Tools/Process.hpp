// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#pragma once

#include <string>
#include <set>
#include <windows.h>

namespace AnyFSE::Tools::Process
{
    DWORD FindFirstByName(const std::wstring& processName);
    DWORD Start(const std::wstring &command, const std::wstring &arguments);
    HWND  GetWindow(const std::wstring &processName, DWORD exStyle, const std::wstring &className, const std::wstring &windowTitle, DWORD style=0, DWORD noStyle=0);
    HWND  GetWindow(const std::set<DWORD>& processIds, DWORD exStyle, const std::wstring &className, const std::wstring &windowTitle, DWORD style=0, DWORD noStyle=0);
    size_t FindAllByName(const std::wstring &processName, std::set<DWORD> & result);
}

namespace Process = AnyFSE::Tools::Process;