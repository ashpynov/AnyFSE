#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <vector>
#include <algorithm>

#include "Process.hpp"

#pragma comment(lib, "psapi.lib")

namespace AnyFSE::Process
{
    DWORD FindByName(const std::wstring &processName)
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

        do
        {
            std::wstring currentName = pe32.szExeFile;
            std::transform(currentName.begin(), currentName.end(), currentName.begin(), ::towlower);

            if (currentName == targetName)
            {
                processId = pe32.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
        return processId;
    }

    HRESULT Kill(const std::wstring &processName)
    {
        DWORD processId = FindByName(processName);
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
        DWORD processId;
        std::string windowTitle;
        HWND foundWindow;
    };

    BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        WindowSearchData *searchData = reinterpret_cast<WindowSearchData *>(lParam);

        DWORD windowProcessId;
        GetWindowThreadProcessId(hwnd, &windowProcessId);

        if (windowProcessId == searchData->processId)
        {
            // Check if it's a main window (has no parent and is visible)
            if (GetParent(hwnd) == NULL && IsWindowVisible(hwnd))
            {
                // If no specific window title provided, return the first main window found
                if (searchData->windowTitle.empty())
                {
                    searchData->foundWindow = hwnd;
                    return FALSE; // Stop enumeration
                }

                // Get window title
                char title[256];
                int titleLength = GetWindowTextA(hwnd, title, sizeof(title));

                if (titleLength > 0)
                {
                    std::string currentTitle(title, titleLength);

                    // Compare titles (case-sensitive)
                    if (currentTitle == searchData->windowTitle)
                    {
                        searchData->foundWindow = hwnd;
                        return FALSE; // Stop enumeration
                    }
                }
            }
        }

        return TRUE; // Continue enumeration
    }

    HWND FindWindow(DWORD processId, const std::string &windowTitle)
    {
        if (processId == 0)
        {
            return NULL;
        }

        WindowSearchData searchData;
        searchData.processId = processId;
        searchData.windowTitle = windowTitle;
        searchData.foundWindow = NULL;

        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&searchData));

        return searchData.foundWindow;
    }
}

namespace Process = AnyFSE::Process;