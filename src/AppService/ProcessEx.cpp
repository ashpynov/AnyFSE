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

#include "Tools/Process.hpp"
#include "ProcessEx.hpp"

#pragma comment(lib, "psapi.lib")

namespace AnyFSE::Tools::ProcessEx
{
    HRESULT Kill(const std::wstring &processName)
    {
        DWORD processId = Process::FindFirstByName(processName);
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
}
