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

#include "AppControl/AppControl.hpp"
#include "AppControl/GamingExperience.hpp"
#include "AppControl/MainWindow.hpp"
#include "AppControl/AppControlStateLoop.hpp"
#include "AppControl.hpp"
#include "App/Application.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    return TRUE;
}

__declspec(dllexport)
int WINAPI Main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return AnyFSE::App::AppControl::AppControl::WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

namespace AnyFSE::App::AppControl
{
    static Logger log = LogManager::GetLogger("AppControl");

    int AppControl::ShowSettings()
    {
        return IsRunningAsAdministrator(true, true)
            ? App::CallLibrary(L"AnyFSE.Settings.dll", GetModuleHandle(NULL), NULL, NULL, 0)
            : FALSE;
    }

    bool AppControl::AsControl(LPSTR lpCmdLine)
    {
        return _strcmpi(lpCmdLine, "/Control") == 0;
    }

    bool AppControl::IsServiceAvailable()
    {
        static bool available = IPCChannel::IsServerAvailable(L"AnyFSEPipe");
        return available;
    }

    bool AppControl::AsSettings(LPSTR lpCmdLine)
    {
        return _strcmpi(lpCmdLine, "/Settings") == 0;
    }

    bool AppControl::NeedAdmin(LPSTR lpCmdLine)
    {
        return AsSettings(lpCmdLine) || (!AsControl(lpCmdLine) && !IsServiceAvailable());
    }

    BOOL AppControl::IsRunningAsAdministrator(bool elevate, bool configure)
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

    BOOL AppControl::RequestAdminElevation(bool configure) {
        wchar_t modulePath[MAX_PATH];
        GetModuleFileName(NULL, modulePath, MAX_PATH);

        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"runas";  // Request UAC elevation
        sei.lpFile = modulePath;
        sei.nShow = SW_NORMAL;

        if (configure)
        {
            sei.lpParameters = L"/Settings";
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

    void AppControl::InitCustomControls()
    {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_STANDARD_CLASSES;
        ::InitCommonControlsEx(&icex);
    }

    int WINAPI AppControl::WinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine,
                    int nCmdShow)
    {
        int exitCode = -1;
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        Config::Load();

        AnyFSE::Logging::LogManager::Initialize("AnyFSE", Config::LogLevel, Config::LogPath);

        LPCTSTR className = L"AnyFSE";

        HWND hAppWnd = FindWindow(className, NULL);
        if (hAppWnd)
        {
            log.Debug("Application control is executed already, exiting\n");
            PostMessage(hAppWnd, MainWindow::WM_TRAY, 0, WM_LBUTTONDBLCLK);
            return 0;
        }

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

        if (NeedAdmin(lpCmdLine) && !IsRunningAsAdministrator(true, AsSettings(lpCmdLine)))
        {
            return -1;
        }

        log.Debug("Application control is started (hInstance=%08x)", hInstance);

        std::thread serviceThread;

        if (!AsControl(lpCmdLine) && !IsServiceAvailable())
        {
            serviceThread = std::thread([&]()
            {
                App::CallLibrary(L"AnyFSE.Service.dll", hInstance, hPrevInstance, NULL, nCmdShow);
            });
        }

        Window::MainWindow mainWindow;

        if (!mainWindow.Create(className, hInstance, (Config::Launcher.Name + L" is launching").c_str()))
        {
            return (int)GetLastError();
        }

        if (GamingExperience::IsActive())
        {
            mainWindow.Show();
        }
        else
        {
            mainWindow.Hide();
        }

        AppControlStateLoop AppControlStateLoop(mainWindow);
        if (!AppControlStateLoop.NotifyRemote(AppEvents::CONNECT, 5000))
        {
            log.Error("Cant connect to service, exiting");
            return -1;
        }

        if (!GamingExperience::ApiIsAvailable)
        {
            log.Critical("Fullscreen Gaming API is not detected, exiting\n");
            AppControlStateLoop.NotifyRemote(AppEvents::EXIT_SERVICE);
            return -1;
        }

        AppControlStateLoop.NotifyRemote(AppEvents::MONITOR_REGISTRY);

        if (Config::Launcher.Type != LauncherType::None || Config::Launcher.Type != LauncherType::Xbox)
        {
            AppControlStateLoop.Notify(AppEvents::START);
            AppControlStateLoop.Start();
        }
        else
        {
            AppControlStateLoop.NotifyRemote(AppEvents::EXIT_SERVICE);
        }

        GamingExperience fseMonitor;

        fseMonitor.OnExperienseChanged += ([&AppControlStateLoop]()
        {
            log.Debug(
                "Mode is changed to %s\n",
                GamingExperience::IsActive() ? "Fullscreeen experience" : "Windows Desktop"
            );
            AppControlStateLoop.Notify(GamingExperience::IsActive() ? AppEvents::GAMEMODE_ENTER : AppEvents::GAMEMODE_EXIT);
        });

        log.Debug("Run window loop.");

        if (AsSettings(lpCmdLine))
        {
            PostMessage(FindWindow(className, NULL), MainWindow::WM_TRAY, 0, WM_LBUTTONDBLCLK);
        }

        exitCode = Window::MainWindow::RunLoop();

        if (exitCode != ERROR_RESTART_APPLICATION)
        {
            AppControlStateLoop.NotifyRemote(AppEvents::EXIT_SERVICE);
        }

        AppControlStateLoop.Stop();

        log.Debug("Loop finished. Time to exit");

        if (serviceThread.joinable())
        {
            log.Debug("Stop homebrew service");

            HWND hServiceWnd = FindWindow(L"AnyFSEService", NULL);
            if (hServiceWnd)
            {
                log.Debug("Service window is found");
                PostMessage(hServiceWnd, WM_DESTROY, 0, 0);
            }
            log.Debug("Joining service thread");
            serviceThread.join();

            log.Debug("Homebrew service is completed");
        }

        if (exitCode)
        {
            log.Warn(log.APIError(exitCode),"Exiting with code: (%d) error", exitCode);
        }
        else
        {
            log.Debug("Job is done!");
        }

        if (exitCode == ERROR_RESTART_APPLICATION)
        {
            wchar_t modulePath[MAX_PATH];
            GetModuleFileName(NULL, modulePath, MAX_PATH);
            Process::Start(modulePath, L"");
        }
        return (int)exitCode;
    }
};
