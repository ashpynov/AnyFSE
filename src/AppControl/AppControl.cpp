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

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return AnyFSE::App::AppControl::AppControl::WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

namespace AnyFSE::App::AppControl
{
    static Logger log = LogManager::GetLogger("AppControl");

    int AppControl::CallLibrary(const WCHAR * library, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
    {
        HMODULE hModuleDll = NULL;
        static MainFunc *Main = nullptr;

        int result = INT_MIN;
        do
        {
            hModuleDll = LoadLibrary(library);
            if (!hModuleDll)
            {
                break;
            }

            MainFunc Main = (MainFunc)GetProcAddress(hModuleDll, "Main");
            if( !Main )
            {
                break;
            }
            result = (int)Main(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

        } while (false);

        if (result == INT_MIN)
        {
            LPSTR messageBuffer = nullptr;
            DWORD size = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&messageBuffer,
                0,
                NULL);
            MessageBoxA(NULL, messageBuffer, "Call module error", MB_OK | MB_ICONERROR);
            LocalFree(messageBuffer);
        }

        if (hModuleDll)
            FreeLibrary(hModuleDll);

        return result;
    }


    int AppControl::ShowAdminError()
    {
        log.Critical("Application should be executed as Adminisrator, exiting\n");

        InitCustomControls();

        TaskDialog(NULL, GetModuleHandle(NULL),
            L"Insufficient permissons",
            L"Please run AnyFSE as Administrator",
            L"Escalated privileges is required to monitor XBox application execution "
            L"or instaling application as schedulled autorun task.\n\n",
            TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL);

        return FALSE;
    }

    int AppControl::ShowSettings()
    {
        return CallLibrary(L"AnyFSE.Settings.dll", GetModuleHandle(NULL), NULL, NULL, 0);
    }

    bool AppControl::AsControl(LPSTR lpCmdLine)
    {
        return _strnicmp(lpCmdLine, "/Control", 8) != 0;
    }

    bool AppControl::IsServiceAvailable()
    {
        static bool available = IPCChannel::IsServerAvailable(L"AnyFSEPipe");
        return available;
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
            AppControlStateLoop.NotifyRemote(AppEvents::SUSPEND_SERVICE);
            return -1;
        }

        if (Config::Launcher.Type != LauncherType::None || Config::Launcher.Type != LauncherType::Xbox)
        {
            AppControlStateLoop.Notify(AppEvents::START);
            AppControlStateLoop.Start();
        }
        else
        {
            log.Info("Launcher is Not configured. Exiting\n");
            AppControlStateLoop.NotifyRemote(AppEvents::SUSPEND_SERVICE);
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

        if (!IsServiceAvailable())
        {
            log.Critical("AnyFSE monitoring service not executed, exiting\n");
            InitCustomControls();
            TaskDialog(NULL, hInstance,
                       L"Execution failure",
                       L"AnyFSE monitoring service is not run.",
                       L"Can not connect to AnyFSE monitoring service.\n"
                       L"It is required to track and prevent Xbox app execution.\n"
                       L"\n"
                       L"Please reinstall application.",
                       TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL);
            return -1;
        }

        log.Debug("\n\nApplication control is started (hInstance=%08x)", hInstance);

        Launchers::LauncherOnBoot();

        DWORD xBoxProcess = Process::FindFirstByName(Config::XBoxProcessName);
        if (xBoxProcess)
        {
            log.Info("Xbox App is already ran: %x", xBoxProcess);
        }

        Window::MainWindow mainWindow;
        AppControlStateLoop AppControlStateLoop(mainWindow);

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

        if (!mainWindow.Create(className, hInstance, (Config::Launcher.Name + L" is launching").c_str()))
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
            log.Debug(
                "Mode is changed to %s\n",
                GamingExperience::IsActive() ? "Fullscreeen experience" : "Windows Desktop"
            );
            if (!AppControlStateLoop.IsRunning())
            {
                SetLastError(StartControl(AppControlStateLoop, mainWindow));
                mainWindow.ExitOnError();
            }
            AppControlStateLoop.Notify(GamingExperience::IsActive() ? AppEvents::GAMEMODE_ENTER : AppEvents::GAMEMODE_EXIT);
        });

        log.Debug("Run window loop.");

        exitCode = Window::MainWindow::RunLoop();

        if (exitCode != ERROR_RESTART_APPLICATION)
        {
            log.Info("Stopping application, notify service exit");
            AppControlStateLoop.NotifyRemote(AppEvents::SUSPEND_SERVICE);
        }
        else
        {
            log.Info("Restarting application, notify service exit");
            AppControlStateLoop.NotifyRemote(AppEvents::RESTART_SERVICE);
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

        return (int)exitCode;
    }
};
