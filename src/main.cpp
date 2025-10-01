/*-------------------------------------------------------------------------------
    AllyG free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
---------------------------------------------------------------------------------*/

#include <windows.h>
#include <iostream>
#include "resource.h"
#include <tchar.h>
#include <commctrl.h>
#include <strsafe.h>
#include "Logging/LogManager.h"
#include "Monitors/GamingExperience.h"
#include "Monitors/ETWMonitor.h"
#include "Window/MainWindow.h"
#include "Configuration/Config.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

BOOL RequestAdminElevation();

BOOL isRunningAsAdministrator()
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

    return fRet || RequestAdminElevation();
}

BOOL RequestAdminElevation() {
    wchar_t modulePath[MAX_PATH];
    GetModuleFileName(NULL, modulePath, MAX_PATH);
    
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = L"runas";  // Request UAC elevation
    sei.lpFile = modulePath;
    sei.nShow = SW_NORMAL;
    
    if (ShellExecuteEx(&sei)) 
    {
        exit(0);
    }
    return false;
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    AnyFSE::Logging::LogManager::Initialize("AnyFSE", LogLevel::Trace);
    Logger log = LogManager::GetLogger("Main");
    log.Info("Application is started (hInstance=%08x)", hInstance);

    Config::Load();

    LPCTSTR className = _T("AnyFSE");

    HWND hAppWnd = FindWindow(className, NULL);
    if (hAppWnd)
    {
        log.Info("Application is executed already, exiting\n");
        PostMessage(hAppWnd, MainWindow::WM_TRAY, 0, WM_LBUTTONDBLCLK);
        return 0;
    }

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    if (!isRunningAsAdministrator())
    {
        log.Critical("Application should be executed as Adminisrator, exiting\n");

        TaskDialog(NULL, hInstance,
            L"Insufficient permissons",
            L"Please run AnyFSE as Administrator",
            L"Escalated privileges is required to monitor XBox application execution "
            L"or instaling application as schedulled autorun task.\n\n",
            TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL 
        );
        return -1;
    }

    if (!GamingExperience::ApiIsImplemented)
    {
        log.Critical("Fullscreen Gaming API is not detected, exiting\n");

        TaskDialog(NULL, hInstance,
            L"Error",
            L"Gaming Fullscreen Experiense API is not detected",
            L"Fullscreen expiriense is not available on your version of windows.\n"
            L"It is supported since Windows 25H2 version for Handheld Devices",
            TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL 
        );
        return -1;
    }

    MainWindow mainWindow;

    if (!mainWindow.Create(className, hInstance, className))
    {
        return (int)GetLastError();
    }
    mainWindow.Show();

    GamingExperience fseMonitor;

    fseMonitor.OnExperienseChanged += ([&log]() 
    {
        log.Info(
            "Mode is changed to %s\n",
            GamingExperience::IsActive() ? "Fullscreeen expirience" : "Windows Desktop"
        );
    });
    
    ETWMonitor etwMonitor(Config::XBoxProcessName);
    etwMonitor.OnProcessExecuted += ([&log]() 
    {
        log.Info(
            "Xbox process execution is detected\n"
        );
    });

    bool cancelToken = false;
    etwMonitor.Run(cancelToken);

    int exitCode = MainWindow::RunLoop();

    etwMonitor.Stop();
    
    if (exitCode)
    {
        log.Warn(log.APIError(exitCode),"Exiting with code: (%d) error", exitCode);
    }
    else
    {
        log.Info("Job is done!");
    }
    
    
    return (int) exitCode;
}
