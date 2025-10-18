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

    HRESULT Kill(const std::wstring &processName)
    {
        DWORD processId = FindFirstByName(processName);
        if (processId == 0)
        {
            return ERROR_PROC_NOT_FOUND;
        }

        return Kill(processId);
    }

    HRESULT Kill(DWORD processId)
    {
        if (processId == 0)
        {
            return ERROR_INVALID_PARAMETER;
        }

        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
        if (hProcess == NULL)
        {
            return GetLastError();
        }

        if (!TerminateProcess(hProcess, 0))
        {
            HRESULT error = GetLastError();
            CloseHandle(hProcess);
            return error;
        }

        CloseHandle(hProcess);
        return ERROR_SUCCESS;
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
        HWND foundWindow;
    };

    BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        WindowSearchData *searchData = reinterpret_cast<WindowSearchData *>(lParam);

        DWORD windowProcessId;
        GetWindowThreadProcessId(hwnd, &windowProcessId);
        if ( searchData->processIds->find(windowProcessId) != searchData->processIds->end())
        {
            // If no specific window title provided, return the first main window found
            if (searchData->windowTitle.empty())
            {
                searchData->foundWindow = hwnd;
                return FALSE; // Stop enumeration
            }

            if (GetParent(hwnd) != NULL || !IsWindowVisible(hwnd))
            {
                return TRUE;
            }
            // Get window title
            wchar_t title[256];
            int titleLength = GetWindowTextW(hwnd, title, sizeof(title));

            if (titleLength > 0)
            {
                std::wstring currentTitle(title, titleLength);

                // Compare titles (case-sensitive)
                if (currentTitle == searchData->windowTitle)
                {
                    searchData->foundWindow = hwnd;
                    return FALSE; // Stop enumeration
                }
            }
        }

        return TRUE; // Continue enumeration
    }

    HWND GetWindow(const std::set<DWORD>& processIds, const std::wstring &windowTitle)
    {
        if (processIds.size() == 0)
        {
            return NULL;
        }

        WindowSearchData searchData;
        searchData.processIds = &processIds;
        searchData.windowTitle = windowTitle;
        searchData.foundWindow = NULL;

        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&searchData));

        return searchData.foundWindow;
    }
}
