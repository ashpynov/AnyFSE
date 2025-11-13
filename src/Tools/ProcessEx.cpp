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
#include "Logging/LogManager.hpp"
#include "Tools/Process.hpp"
#include "Tools/Unicode.hpp"
#include "ProcessEx.hpp"

#pragma comment(lib, "psapi.lib")

namespace AnyFSE::Tools::ProcessEx
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
