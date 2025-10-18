
#include <windows.h>
#include <iostream>
#include "resource.h"
#include <tchar.h>
#include <commctrl.h>
#include <strsafe.h>
#include "Application.hpp"
#include "Logging/LogManager.hpp"
#include "Monitors/GamingExperience.hpp"
#include "Monitors/ETWMonitor.hpp"
#include "Window/MainWindow.hpp"
#include "Manager/ManagerState.hpp"
#include "Configuration/Config.hpp"
#include "Settings/SettingsDialog.hpp"
#include "Tools/Process.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace AnyFSE
{
    static Logger log = LogManager::GetLogger("Main");

    BOOL Application::IsRunningAsAdministrator(bool elevate, bool configure)
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

    BOOL Application::RequestAdminElevation(bool configure) {
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
        AnyFSE::Application::InitCustomControls();
        TaskDialog(NULL, GetModuleHandle(NULL),
            L"Insufficient permissons",
            L"Please run AnyFSE as Administrator",
            L"Escalated privileges is required to monitor XBox application execution "
            L"or instaling application as schedulled autorun task.\n\n",
            TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL);

        return false;
    }

    void Application::InitCustomControls()
    {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_STANDARD_CLASSES;
        ::InitCommonControlsEx(&icex);
    }

    BOOL Application::ShouldRunAsApplication(LPSTR lpCmdLine)
    {
        return !_strcmpi(lpCmdLine, "--application");
    }

    BOOL Application::ShouldRunAsConfiguration(LPSTR lpCmdLine)
    {
        return !_strcmpi(lpCmdLine, "--configure") || !Config::IsConfigured();
    }

    int WINAPI Application::WinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine,
                    int nCmdShow)
    {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        int exitCode = -1;

        AnyFSE::Logging::LogManager::Initialize("AnyFSE");

        LPCTSTR className = L"AnyFSE";
        HWND hAppWnd = FindWindow(className, NULL);

        if (hAppWnd)
        {
            log.Info("Application is executed already, exiting\n");
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

        log.Info("Application is started (hInstance=%08x)", hInstance);
        try
        {
            Config::Load();
        }
        catch (std::exception ex)
        {
            log.Error(ex, "Fail to load config:");
        }
 
        if (!ShouldRunAsApplication(lpCmdLine) && !AnyFSE::Application::IsRunningAsAdministrator(true))
        {
            return -1;
        }

        MainWindow mainWindow;

        if (!mainWindow.Create(className, hInstance, (Config::LauncherName + L" is launching").c_str()))
        {
            return (int)GetLastError();
        }

        if (ShouldRunAsConfiguration(lpCmdLine))
        {
            hAppWnd = FindWindow(className, NULL);
            PostMessage(hAppWnd, MainWindow::WM_TRAY, 0, WM_LBUTTONDBLCLK);
        }

        if (GamingExperience::IsActive())
        {
            mainWindow.Show();
        }
        else
        {
            mainWindow.Hide();
        }

        ManagerState managerState(mainWindow);
        if (!managerState.NotifyRemote(StateEvent::CONNECT, 5000))
        {
            log.Error("Cant connect to service, exiting");
            return -1;
        }

                if (!GamingExperience::ApiIsAvailable)
        {
            log.Critical("Fullscreen Gaming API is not detected, exiting\n");
            managerState.NotifyRemote(StateEvent::EXIT_SERVICE);
            return -1;
        }
        //managerState.NotifyRemote(StateEvent::MONITOR_REGISTRY);
        
        if (Config::Type != LauncherType::None || Config::Type != LauncherType::Xbox)
        {
            managerState.Notify(StateEvent::START);
            managerState.Start();
        }
        else
        {
            managerState.NotifyRemote(StateEvent::EXIT_SERVICE);
        }

        GamingExperience fseMonitor;

        fseMonitor.OnExperienseChanged += ([&managerState]()
        {
            log.Info(
                "Mode is changed to %s\n",
                GamingExperience::IsActive() ? "Fullscreeen experience" : "Windows Desktop"
            );
            managerState.Notify(GamingExperience::IsActive() ? StateEvent::GAMEMODE_ENTER : StateEvent::GAMEMODE_EXIT);
        });

        log.Info("Run window loop.");

        exitCode = MainWindow::RunLoop();

        if (exitCode != ERROR_RESTART_APPLICATION)
        {
            managerState.NotifyRemote(StateEvent::EXIT_SERVICE);
        }


        managerState.Stop();

        log.Info("Loop finished. Time to exit");


        if (exitCode)
        {
            log.Warn(log.APIError(exitCode),"Exiting with code: (%d) error", exitCode);
        }
        else
        {
            log.Info("Job is done!");
        }

        if (exitCode == ERROR_RESTART_APPLICATION)
        {
            wchar_t modulePath[MAX_PATH];
            GetModuleFileName(NULL, modulePath, MAX_PATH);
            Process::Start(modulePath, ShouldRunAsApplication(lpCmdLine) ? L"--application" : L"" );
        }
        return (int)exitCode;
    }
};