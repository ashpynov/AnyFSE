// MIT License
//
// Copyright (c) 2025 Artem Shpynov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


#pragma once

#include <string>
#include <set>
#include <windows.h>

namespace AnyFSE::Tools::Process
{
    HWND FindAppWindow(HANDLE hProcess);
    DWORD FindFirstByExe(const std::wstring &processPath);
    DWORD FindFirstByName(const std::wstring& processName);
    DWORD StartProtocol(const std::wstring &command);
    DWORD StartProcess(const std::wstring &command, const std::wstring &arguments);
    HWND  GetWindow(const std::wstring &processName, DWORD exStyle, const std::wstring &className, const std::wstring &windowTitle, DWORD style=0, DWORD noStyle=0);
    HWND  GetWindow(const std::set<DWORD>& processIds, DWORD exStyle, const std::wstring &className =L"", const std::wstring &windowTitle=L"", DWORD style=0, DWORD noStyle=0);
    size_t FindAllByName(const std::wstring &processName, std::set<DWORD> & result);
    BOOL EnumWindowsAlt(HWND start, BOOL (*callback)(HWND, LPARAM), LPARAM lParam);
    bool BringWindowToForeground(HWND hWnd, int nShowCmd = SW_SHOWDEFAULT);
}

namespace Process = AnyFSE::Tools::Process;