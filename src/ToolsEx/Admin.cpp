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

#include "Admin.hpp"
#include "Tools/Process.hpp"
#include "Tools/Paths.hpp"

namespace AnyFSE::ToolsEx::Admin
{
    BOOL IsRunningAsAdministrator()
    {
        BOOL fRet = FALSE;
        HANDLE hToken = NULL;

        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            TOKEN_ELEVATION Elevation;
            DWORD cbSize = sizeof(TOKEN_ELEVATION);

            if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize))
            {
                fRet = Elevation.TokenIsElevated;
            }
        }

        if (hToken)
        {
            CloseHandle(hToken);
        }

        return fRet;
    }

    BOOL RequestAdminElevation(const std::wstring& args)
    {
        std::wstring modulePath = Tools::Paths::GetExeFileName();

        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"runas";  // Request UAC elevation
        sei.lpFile = modulePath.c_str();
        sei.nShow = SW_SHOWNORMAL;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        if (!args.empty())
        {
            sei.lpParameters = args.c_str();
        }

        if (ShellExecuteEx(&sei))
        {
            if (sei.hProcess)
            {
                // Wait for the process to start (but not necessarily show window)
                WaitForInputIdle(sei.hProcess, 5000);

                HWND hWnd = 0;
                DWORD timeout = GetTickCount() + 3000;
                do
                {
                    hWnd = Tools::Process::FindAppWindow(sei.hProcess);
                    if (hWnd)
                    {
                        Tools::Process::BringWindowToForeground(hWnd);
                        break;
                    }
                    else
                    {
                        Sleep(0);
                    }
                } while (!hWnd && GetTickCount() < timeout );

                CloseHandle(sei.hProcess);
            }
            exit(0);
        }

        return false;

    }
}
