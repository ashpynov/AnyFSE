// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <set>
#include <sstream>

#include "Process.hpp"

#pragma comment(lib, "psapi.lib")

namespace AnyFSE::Tools::Process
{
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

    DWORD Start(const std::wstring &command, const std::wstring &arguments)
    {
        STARTUPINFOW si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(si);

        std::wstring fullCommand = L"\"" + command;
        if (!arguments.empty())
        {
            fullCommand += L"\" " + arguments;
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
        const std::set<DWORD> * processIds;
        std::wstring windowTitle;
        std::wstring className;
        DWORD style;
        DWORD noStyle;
        DWORD exStyle;
        HWND foundWindow;
    };

    BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        WindowSearchData *searchData = reinterpret_cast<WindowSearchData *>(lParam);

        DWORD windowProcessId;
        GetWindowThreadProcessId(hwnd, &windowProcessId);
        if ( searchData->processIds->find(windowProcessId) != searchData->processIds->end())
        {
            BOOL classMatch = searchData->windowTitle.empty();
            BOOL titleMatch = searchData->className.empty();
            BOOL exStyleMatch = searchData->exStyle == 0;
            BOOL styleMatch = searchData->style == 0;
            BOOL noStyleMatch = searchData->noStyle == 0;
            
            if (!classMatch)
            {
                wchar_t className[MAX_PATH + 1];
                int classNameLength = RealGetWindowClassW(hwnd, className, MAX_PATH);

                if (classNameLength > 0)
                {
                    std::wstring classNameStr(className, classNameLength);

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

            if (!styleMatch)
            {
                DWORD style = GetWindowLong(hwnd, GWL_STYLE);
                styleMatch = ((searchData->style & style) == searchData->style );
            }
            
            if (!noStyleMatch)
            {
                DWORD style = GetWindowLong(hwnd, GWL_STYLE);
                noStyleMatch = ((searchData->noStyle & style) == 0);
            }

            if (styleMatch && noStyleMatch && exStyleMatch && titleMatch && classMatch)
            {
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

        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&searchData));

        return searchData.foundWindow;
    }
}
