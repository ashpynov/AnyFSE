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
#include "AppControl/Launchers.hpp"
#include "Tools/Admin.hpp"

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
        log.Debug("Triggered Show settings");
        HWND hAdminWnd = FindWindow(AnyFSEAdminWndClassName, nullptr);

        if (hAdminWnd)
        {
            log.Debug("Post WM_SHOWSETTINGS to 0x%x", hAdminWnd);
            PostMessage(hAdminWnd, UserMessage::WM_SHOWSETTINGS, 0, 0);
            return 0;
        }

        log.Debug("Admin window is not found");
        if (!Tools::Admin::IsRunningAsAdministrator() &&!Tools::Admin::RequestAdminElevation(L"/Settings"))
        {
            Tools::Admin::ShowAdminError();
            return -1;
        }

        return 0;
    }

    bool AppControl::AsCombo(LPSTR lpCmdLine)
    {
        return _strcmpi(lpCmdLine, "/Combo") == 0;
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
        return _strcmpi(lpCmdLine, "/Settings") == 0 || !Config::IsConfigured();
    }

    bool AppControl::NeedAdmin(LPSTR lpCmdLine)
    {
        return AsSettings(lpCmdLine) || (!AsControl(lpCmdLine) && !IsServiceAvailable());
    }

    int AppControl::StartControl(AppControlStateLoop & AppControlStateLoop, Window::MainWindow& mainWindow)
    {

        if (!Config::QuickStart && !Process::FindFirstByName(L"explorer.exe"))
        {
            log.Info("Explorer not found => Delay start");
            return 0;
        }

        if (GamingExperience::IsActive())
        {
            mainWindow.Show();
        }
        else
        {
            mainWindow.Hide();
        }

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

        if (!Config::AggressiveMode)
        {
            AppControlStateLoop.NotifyRemote(AppEvents::MONITOR_REGISTRY);
        }

        if (Config::Launcher.Type != LauncherType::None || Config::Launcher.Type != LauncherType::Xbox)
        {
            AppControlStateLoop.Notify(AppEvents::START);
            AppControlStateLoop.Start();
        }
        else
        {
            log.Info("Launcher is Not configured. Exiting\n");
            AppControlStateLoop.NotifyRemote(AppEvents::EXIT_SERVICE);
            return -1;
        }
        return 0;
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

        // ShowSettings();

        HWND hAppWnd = FindWindow(AnyFSESplashWndClassName, NULL);
        if (hAppWnd)
        {
            log.Debug("Application control is executed already, exiting\n");
            PostMessage(hAppWnd, UserMessage::WM_TRAY, 0, WM_LBUTTONDBLCLK);
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

        if (AsCombo(lpCmdLine)
            && !Tools::Admin::IsRunningAsAdministrator()
            && !Tools::Admin::RequestAdminElevation(AsSettings(lpCmdLine) ? L"/Settings" : L"")
        )
        {
            Tools::Admin::ShowAdminError();
            return -1;
        }

        if (!AsCombo(lpCmdLine) && !IsServiceAvailable())
        {
            log.Debug("\n\nNeed Restart Task");
            if( !Tools::Admin::RequestAdminElevation(L"/RestartTask") )
            {
                Tools::Admin::ShowAdminError();
                return -1;
            }
            return 0;
        }

        log.Debug("\n\nApplication control is started (hInstance=%08x)", hInstance);

        log.Debug("Messages ID: WM_RESTARTTASK: %u, WM_SHOWSETTINGS: %u", UserMessage::WM_RESTARTTASK, UserMessage::WM_SHOWSETTINGS );

        Launchers::LauncherOnBoot();

        DWORD xBoxProcess = Process::FindFirstByName(Config::XBoxProcessName);
        if (xBoxProcess)
        {
            log.Info("Xbox App is already ran: %x", xBoxProcess);
        }

        Window::MainWindow mainWindow;
        AppControlStateLoop AppControlStateLoop(mainWindow);
        std::thread serviceThread;
        std::thread settingsThread;

        if (AsCombo(lpCmdLine))
        {
            if (IsServiceAvailable())
            {
                AppControlStateLoop.NotifyRemote(AppEvents::EXIT_SERVICE);
            }
            serviceThread = std::thread([&]()
            {
                App::CallLibrary(L"AnyFSE.Service.dll", hInstance, hPrevInstance, NULL, nCmdShow);
            });

            settingsThread = std::thread([&]()
            {
                App::CallLibrary(L"AnyFSE.Settings.dll", hInstance, hPrevInstance, "/Settings", nCmdShow);
            });
        }

        mainWindow.OnExplorerDetected += ([&AppControlStateLoop, &mainWindow]()
        {
            if (!AppControlStateLoop.IsRunning())
            {
                SetLastError(StartControl(AppControlStateLoop, mainWindow));
            }
            AppControlStateLoop.Notify(AppEvents::START_APPS);
        });

        mainWindow.OnQueryEndSession += ([&AppControlStateLoop]()
        {
            AppControlStateLoop.Notify(AppEvents::QUERY_END_SESSION);
        });

        mainWindow.OnEndSession += ([&AppControlStateLoop]()
        {
            AppControlStateLoop.Notify(AppEvents::END_SESSION);
        });

        if (!mainWindow.Create(AnyFSESplashWndClassName, hInstance, (Config::Launcher.Name + L" is launching").c_str()))
        {
            return (int)GetLastError();
        }

        if (!AppControlStateLoop.IsRunning())
        {
            SetLastError(StartControl(AppControlStateLoop, mainWindow));
            mainWindow.ExitOnError();
        }

        GamingExperience fseMonitor;

        fseMonitor.OnExperienseChanged += ([&AppControlStateLoop, &mainWindow]()
        {
            log.Debug("Mode is changed to %s\n", GamingExperience::IsActive() ? "Fullscreeen experience" : "Windows Desktop");
            if (!AppControlStateLoop.IsRunning())
            {
                SetLastError(StartControl(AppControlStateLoop, mainWindow));
                mainWindow.ExitOnError();
            }
            AppControlStateLoop.Notify(GamingExperience::IsActive() ? AppEvents::GAMEMODE_ENTER : AppEvents::GAMEMODE_EXIT);
        });

        log.Debug("Run window loop.");

        // if (AsSettings(lpCmdLine))
        // {
        //     PostMessage(FindWindow(AnyFSESplashWndClassName, NULL), UserMessage::WM_TRAY, 0, WM_LBUTTONDBLCLK);
        // }

        exitCode = Window::MainWindow::RunLoop();

        if (exitCode != ERROR_RESTART_APPLICATION)
        {
            log.Info("Stopping application, notify service exit");
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

        if (settingsThread.joinable())
        {
            log.Debug("Stop homebrew Settings service");

            HWND hSettingsWnd = FindWindow(App::AnyFSEAdminWndClassName, NULL);
            if (hSettingsWnd)
            {
                log.Debug("Settings window is found");
                PostMessage(hSettingsWnd, WM_DESTROY, 0, 0);
            }
            log.Debug("Joining Settings thread");
            settingsThread.join();

            log.Debug("Homebrew Settings service is completed");
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
            Process::StartProcess(modulePath, L"");
        }
        return (int)exitCode;
    }
};
