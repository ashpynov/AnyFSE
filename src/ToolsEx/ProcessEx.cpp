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
#include "Logging/LogManager.hpp"
#include "Tools/Process.hpp"
#include "Tools/Unicode.hpp"
#include "ProcessEx.hpp"

#pragma comment(lib, "psapi.lib")

namespace AnyFSE::ToolsEx::ProcessEx
{
    static Logger log = LogManager::GetLogger("ProcessEx");

    HRESULT Kill(const std::wstring &processName)
    {
        DWORD processId = Process::FindFirstByName(processName);
        if (processId == 0)
        {
            log.Error("Kill process failed: %s process tot found", Unicode::to_string(processName).c_str());
            return ERROR_PROC_NOT_FOUND;
        }

        log.Debug("Process %s found with id: %u", Unicode::to_string(processName).c_str(), processId);
        return Kill(processId);
    }

    HRESULT KillSystem(DWORD processId)
    {
        if (processId == 0)
        {
            return ERROR_INVALID_PARAMETER;
        }

        // Method 2: Run taskkill with runas for elevated privileges
        std::wstring command = L"/C taskkill /F /PID " + std::to_wstring(processId);

        SHELLEXECUTEINFOW sei = {sizeof(sei)};
        sei.lpFile = L"cmd.exe";
        sei.lpParameters = command.c_str();
        sei.nShow = SW_HIDE;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        if (!ShellExecuteExW(&sei))
        {
            DWORD error = GetLastError();
            return HRESULT_FROM_WIN32(error);
        }

        if (sei.hProcess)
        {
            // Wait for the process to complete
            WaitForSingleObject(sei.hProcess, 10000); // Wait up to 10 seconds
            CloseHandle(sei.hProcess);
        }
        return NULL;
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
            log.Error(log.APIError(), "Kill process. OpenProcess failed");
            return GetLastError();
        }

        if (!TerminateProcess(hProcess, 0))
        {
            log.Error(log.APIError(), "Kill process. TerminateProcess failed");
            HRESULT error = GetLastError();
            CloseHandle(hProcess);
            return error;
        }

        CloseHandle(hProcess);
        return ERROR_SUCCESS;
    }

}
