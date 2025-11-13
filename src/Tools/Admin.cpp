#include "Admin.hpp"

namespace AnyFSE::Tools::Admin
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
        wchar_t modulePath[MAX_PATH];
        GetModuleFileName(NULL, modulePath, MAX_PATH);

        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"runas";  // Request UAC elevation
        sei.lpFile = modulePath;
        sei.nShow = SW_NORMAL;

        if (!args.empty())
        {
            sei.lpParameters = args.c_str();
        }

        if (ShellExecuteEx(&sei))
        {
            exit(0);
        }

        return false;

    }
}
