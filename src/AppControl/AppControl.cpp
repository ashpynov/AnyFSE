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

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return AnyFSE::App::AppControl::AppControl::WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

namespace AnyFSE::App::AppControl
{
    static Logger log = LogManager::GetLogger("AppControl");

    int WINAPI AppControl::WinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine,
                    int nCmdShow)
    {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        int exitCode = -1;
        Config::Load();

        AnyFSE::Logging::LogManager::Initialize("AnyFSEControl");

        LPCTSTR className = L"AnyFSE_AppControl";

        if (FindWindow(className, NULL))
        {
            log.Debug("Application is executed already, exiting\n");
            return 0;
        }

        log.Debug("Application is started (hInstance=%08x)", hInstance);

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

        exitCode = Window::MainWindow::RunLoop();

        if (exitCode != ERROR_RESTART_APPLICATION)
        {
            AppControlStateLoop.NotifyRemote(AppEvents::EXIT_SERVICE);
        }


        AppControlStateLoop.Stop();

        log.Debug("Loop finished. Time to exit");


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
