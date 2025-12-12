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


#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <set>
#include <sstream>
#include <filesystem>

#include "Logging/LogManager.hpp"
#include "Tools/Unicode.hpp"
#include "Process.hpp"

#pragma comment(lib, "psapi.lib")

namespace AnyFSE::Tools::Process
{
    static Logger log = LogManager::GetLogger("Process");

    DWORD FindFirstByExe(const std::wstring &processPath)
    {
        std::filesystem::path path(processPath);

        return _wcsicmp(L".exe", path.extension().wstring().c_str())
            ? 0 : FindFirstByName(path.filename().wstring());
    }

    DWORD FindFirstByName(const std::wstring &processName)
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
        {
            return 0;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (!Process32FirstW(hSnapshot, &pe32))
        {
            CloseHandle(hSnapshot);
            return 0;
        }

        DWORD processId = 0;
        std::wstring targetName = processName;

        // Convert to lowercase for case-insensitive comparison
        std::transform(targetName.begin(), targetName.end(), targetName.begin(), ::towlower);

        std::set<std::wstring> targetNames;
        std::wstringstream ss(targetName);
        std::wstring token;

        while (std::getline(ss, token, L';'))
        {
            if (!token.empty())
            {
                targetNames.insert(token);
            }
        }

        do
        {
            std::wstring currentName = pe32.szExeFile;
            std::transform(currentName.begin(), currentName.end(), currentName.begin(), ::towlower);

            if (targetNames.find(currentName) != targetNames.end())
            {
                processId = pe32.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
        return processId;
    }

    size_t FindAllByName(const std::wstring &processName, std::set<DWORD> & result)
    {
        result.clear();
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
        {
            return 0;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (!Process32FirstW(hSnapshot, &pe32))
        {
            CloseHandle(hSnapshot);
            return 0;
        }

        DWORD processId = 0;
        std::wstring targetName = processName;

        // Convert to lowercase for case-insensitive comparison
        std::transform(targetName.begin(), targetName.end(), targetName.begin(), ::towlower);

        std::set<std::wstring> targetNames;
        std::wstringstream ss(targetName);
        std::wstring token;

        while (std::getline(ss, token, L';'))
        {
            if (!token.empty())
            {
                targetNames.insert(token);
            }
        }

        do
        {
            std::wstring currentName = pe32.szExeFile;
            std::transform(currentName.begin(), currentName.end(), currentName.begin(), ::towlower);

            if (targetNames.find(currentName) != targetNames.end())
            {
                result.insert(pe32.th32ProcessID);
            }
        } while (Process32NextW(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
        return result.size();
    }

    DWORD StartProtocol(const std::wstring &ptorocol)
    {
        HINSTANCE hInstance = ShellExecuteW(NULL, L"open", ptorocol.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return (INT_PTR)hInstance > 32 ? 1 : 0;
    }

    DWORD StartProcess(const std::wstring &command, const std::wstring &arguments)
    {
        if (command.find(L"://") != std::wstring::npos)
        {
            return StartProtocol(command);
        }

        STARTUPINFOW si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(si);

        std::wstring fullCommand = L"\"" + command +L"\"";
        if (!arguments.empty())
        {
            fullCommand += L" " + arguments;
        }

        // Create writable buffer for command line
        std::vector<wchar_t> cmdLine(MAX_PATH);
        wcscpy_s(cmdLine.data(), MAX_PATH, fullCommand.c_str());

        if (!CreateProcessW(
                NULL,           // No module name (use command line)
                cmdLine.data(), // Command line
                NULL,           // Process handle not inheritable
                NULL,           // Thread handle not inheritable
                FALSE,          // Set handle inheritance to FALSE
                0,              // No creation flags
                NULL,           // Use parent's environment block
                NULL,           // Use parent's starting directory
                &si,            // Pointer to STARTUPINFO structure
                &pi))           // Pointer to PROCESS_INFORMATION structure
        {
            return 0;
        }

        DWORD processId = GetProcessId(pi.hProcess);

        // Close process and thread handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return processId;
    }

    // Callback function for EnumWindows
    struct WindowSearchData
    {
        const std::set<DWORD> * processIds = nullptr;
        std::wstring windowTitle;
        std::wstring className;
        DWORD style = 0;
        DWORD noStyle = 0;
        DWORD exStyle = 0;
        HWND foundWindow = nullptr;
    };

    BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        WindowSearchData *searchData = reinterpret_cast<WindowSearchData *>(lParam);

        DWORD windowProcessId;
        GetWindowThreadProcessId(hwnd, &windowProcessId);
        if ( searchData->processIds->find(windowProcessId) != searchData->processIds->end())
        {
            BOOL classMatch = searchData->className.empty();
            BOOL titleMatch = searchData->windowTitle.empty();
            BOOL exStyleMatch = searchData->exStyle == 0;
            BOOL styleMatch = searchData->style == 0 || (searchData->noStyle & WS_VISIBLE);
            BOOL noStyleMatch = searchData->noStyle == 0 || (searchData->noStyle & WS_VISIBLE);

            if (!classMatch)
            {
                wchar_t className[MAX_PATH + 1];
                int classNameLength = RealGetWindowClassW(hwnd, className, MAX_PATH);

                if (classNameLength > 0)
                {
                    std::wstring classNameStr(className, classNameLength);

                    TRACE("Class Name: %s", Unicode::to_string(classNameStr).c_str());

                    // Compare titles (case-sensitive)
                    if (classNameStr == searchData->className)
                    {
                        classMatch = true;
                    }
                }
            }

            if (!titleMatch)
            {
                wchar_t title[MAX_PATH + 1];
                int titleLength = GetWindowTextW(hwnd, title, MAX_PATH);

                if (titleLength > 0)
                {
                    std::wstring currentTitle(title, titleLength);

                    log.Debug("Title Name: %s", Unicode::to_string(currentTitle).c_str());
                    // Compare titles (case-sensitive)
                    if (currentTitle == searchData->windowTitle)
                    {
                        titleMatch = true;
                    }
                }
            }

            if (!exStyleMatch)
            {
                DWORD exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
                exStyleMatch = ((searchData->exStyle & exStyle) == searchData->exStyle );
            }
            TRACE("ExStyle %x -> %x", GetWindowLong(hwnd, GWL_EXSTYLE), searchData->exStyle);

            if (!styleMatch)
            {
                DWORD style = GetWindowLong(hwnd, GWL_STYLE);
                styleMatch = ((searchData->style & style) == searchData->style );
                TRACE("Style %x -> %x", style, searchData->style);
            }

            if (!noStyleMatch)
            {
                DWORD style = GetWindowLong(hwnd, GWL_STYLE);
                noStyleMatch = ((searchData->noStyle & style) == 0);
            }

            if (styleMatch && noStyleMatch && exStyleMatch && titleMatch && classMatch)
            {
                log.Debug("Found launcher window 0x%x", hwnd);
                searchData->foundWindow = hwnd;
                return FALSE; // Stop enumeration

            }
        }

        return TRUE; // Continue enumeration
    }

    HWND GetWindow(const std::wstring &processName, DWORD exStyle, const std::wstring &className, const std::wstring &windowTitle, DWORD style, DWORD noStyle)
    {
        std::set<DWORD> processIds;
        return Process::FindAllByName(processName, processIds)
            ? GetWindow(processIds, exStyle, className, windowTitle, style, noStyle)
            : NULL;
    }

    BOOL EnumWindowsAlt(HWND start, BOOL (*callback)(HWND, LPARAM), LPARAM lParam)
    {
        if (!callback)
            return FALSE;

        HWND hwnd = FindWindowEx(start, NULL, nullptr, nullptr);

        if (!hwnd)
            return TRUE;

        while (hwnd != NULL)
        {
            if (!callback(hwnd, lParam) || !EnumWindowsAlt(hwnd, callback, lParam))
            {
                return FALSE;
            }

            hwnd = FindWindowEx(start, hwnd, nullptr, nullptr);
        }

        return TRUE;
    }
    HWND GetWindow(const std::set<DWORD>& processIds, DWORD exStyle, const std::wstring &className, const std::wstring &windowTitle, DWORD style, DWORD noStyle)
    {
        if (processIds.size() == 0)
        {
            return NULL;
        }

        WindowSearchData searchData;
        searchData.processIds = &processIds;
        searchData.windowTitle = windowTitle;
        searchData.className = className;
        searchData.exStyle = exStyle;
        searchData.style = style;
        searchData.noStyle = noStyle;
        searchData.foundWindow = NULL;

        EnumWindowsAlt(NULL, EnumWindowsProc, reinterpret_cast<LPARAM>(&searchData));
        return searchData.foundWindow;
    }

    bool BringWindowToForeground(HWND hWnd)
    {
        if (!IsWindow(hWnd))
        {
            return false;
        }

        if (IsIconic(hWnd))
        {
            ShowWindow(hWnd, SW_SHOWMAXIMIZED);
        }

        if (GetForegroundWindow() == hWnd)
        {
            return true;
        }

        DWORD currentThread = GetCurrentThreadId();
        DWORD foregroundThread = GetWindowThreadProcessId(GetForegroundWindow(), 0);

        if (currentThread != foregroundThread)
        {
            AttachThreadInput(currentThread, foregroundThread, TRUE);
            SetForegroundWindow(hWnd);
            AttachThreadInput(currentThread, foregroundThread, FALSE);
        }
        else
        {
            SetForegroundWindow(hWnd);
        }

        // Final verification and attempt
        if (GetForegroundWindow() != hWnd)
        {
            // Simulate Alt key press as last resort
            keybd_event(VK_MENU, 0, 0, 0);
            keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
            SetForegroundWindow(hWnd);
        }

        return (GetForegroundWindow() == hWnd);
    }
}
