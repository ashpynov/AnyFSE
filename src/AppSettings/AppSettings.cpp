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
#include <iostream>
#include "resource.h"
#include <tchar.h>
#include <commctrl.h>
#include <strsafe.h>

#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Process.hpp"

#include "AppSettings/AppSettings.hpp"
#include "AppControl/GamingExperience.hpp"


#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return AnyFSE::App::AppSettings::AppSettings::WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

namespace AnyFSE::App::AppSettings
{
    static Logger log = LogManager::GetLogger("Main");

    BOOL AppSettings::IsRunningAsAdministrator(bool elevate, bool configure)
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

        return fRet || (elevate && RequestAdminElevation(configure));
    }

    BOOL AppSettings::RequestAdminElevation(bool configure) {
        wchar_t modulePath[MAX_PATH];
        GetModuleFileName(NULL, modulePath, MAX_PATH);

        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"runas";  // Request UAC elevation
        sei.lpFile = modulePath;
        sei.nShow = SW_NORMAL;

        if (configure)
        {
            sei.lpParameters = L"--configure";
        }

        if (ShellExecuteEx(&sei))
        {
            exit(0);
        }

        log.Critical("Application should be executed as Adminisrator, exiting\n");
        InitCustomControls();
        TaskDialog(NULL, GetModuleHandle(NULL),
            L"Insufficient permissons",
            L"Please run AnyFSE as Administrator",
            L"Escalated privileges is required to monitor XBox application execution "
            L"or instaling application as schedulled autorun task.\n\n",
            TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL);

        return false;
    }

    void AppSettings::InitCustomControls()
    {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_STANDARD_CLASSES;
        ::InitCommonControlsEx(&icex);
    }

    int WINAPI AppSettings::WinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine,
                    int nCmdShow)
    {
        Config::Load();

        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        int exitCode = -1;

        AnyFSE::Logging::LogManager::Initialize("AnyFSE");

        LPCTSTR className = L"AnyFSE";
        HWND hAppWnd = FindWindow(className, NULL);

        // if (hAppWnd)
        // {
        //     log.Debug("Application is executed already, exiting\n");
        //     PostMessage(hAppWnd, AppControl::Window::MainWindow::WM_TRAY, 0, WM_LBUTTONDBLCLK);
        //     return 0;
        // }

        if (!GamingExperience::ApiIsAvailable)
        {
            log.Critical("Fullscreen Gaming API is not detected, exiting\n");
            InitCustomControls();
            TaskDialog(NULL, hInstance,
                       L"Error",
                       L"Gaming Fullscreen Experiense API is not detected",
                       L"Fullscreen experiense is not available on your version of windows.\n"
                       L"It is supported since Windows 25H2 version for Handheld Devices",
                       TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL);
            return -1;
        }

        log.Debug("Application is started (hInstance=%08x)", hInstance);
    }
};

