// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include "Service.hpp"
#include "Application.hpp"
#include "Monitors/ETWMonitor.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/Process.hpp"
#include "Window/MainWindow.hpp"

#include <windows.h>
#include <wtsapi32.h>
#include <UserEnv.h>
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "userenv.lib")

namespace AnyFSE
{
    static Logger log = LogManager::GetLogger("Service");

    bool Service::xboxIsDenied = false;
    const std::wstring Service::XBoxProcessName(L"XboxPcApp.exe");
    AnyFSE::Monitors::ETWMonitor Service::etwMonitor(XBoxProcessName);
    AnyFSE::Manager::State::ManagerService Service::managerState;
    HWND Service::m_hwnd;

    BOOL Service::ExitService()
    {
        PostMessage(m_hwnd, WM_DESTROY, 0, 0);
        return true;
    }

    BOOL Service::ShouldRunAsService(LPSTR lpCmdLine)
    {
        return !_strcmpi(lpCmdLine, "--service");
    }

    int WINAPI Service::ServiceMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine)
    {
        AnyFSE::Logging::LogManager::Initialize("AnyFSEService");
        log.Debug("Service is started (hInstance=%08x)", hInstance);

        if (!Application::IsRunningAsAdministrator())
        {
            log.Critical("Service should be executed as Adminisrator, exiting\n");
            return -1;
        };

        CreateBackgroundWindow();
        StartMonitoring();
        DWORD activeSession = WTSGetActiveConsoleSessionId();
        if (lpCmdLine != NULL && activeSession != 0xFFFFFFFF)
        {
            log.Debug("Session is started already force to run\n");
            LaunchAppInUserSession(activeSession);
        }
        MonitorSessions();
        StopMonitoring();
        log.Debug("Work is done");

        return 0;
    }

    void Service::CreateBackgroundWindow()
    {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = BackgroundWindowProc;
        wc.lpszClassName = L"AnyFSELogonMonitor";
        wc.hInstance = GetModuleHandle(NULL);

        RegisterClassEx(&wc);

        m_hwnd = CreateWindowEx(0, L"AnyFSELogonMonitor",
                                  L"Early start of AnyFSE",  // Descriptive title
                                  0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

        if (m_hwnd)
        {
            log.Debug("Logon monitor background window is created");
            WTSRegisterSessionNotification(m_hwnd, NOTIFY_FOR_ALL_SESSIONS);
        }
    }

    //static
    LRESULT CALLBACK Service::BackgroundWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_WTSSESSION_CHANGE && wParam == WTS_SESSION_LOGON)
        {
            LaunchAppInUserSession((DWORD)lParam);
        }
        else if (msg == WM_DESTROY)
        {
            managerState.Stop();
            WTSUnRegisterSessionNotification(hwnd);
            PostQuitMessage(0);
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void Service::MonitorSessions()
    {
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    BOOL Service::LaunchAppInUserSession(DWORD sessionId)
    {
        log.Debug(log.APIError(), "LaunchAppInUserSession");

        HANDLE hUserToken = NULL;
        if (!WTSQueryUserToken(sessionId, &hUserToken))
        {
            log.Error(log.APIError(), "Token Fail");
            return FALSE;
        }

        STARTUPINFO si = {sizeof(si)};
        PROCESS_INFORMATION pi = {0};

        // Set up environment block for the target session
        LPVOID pEnvironment = NULL;
        if (!CreateEnvironmentBlock(&pEnvironment, hUserToken, FALSE))
        {
            log.Error(log.APIError(), "Fail CreateEnvironmentBlock...");
            return FALSE;
        }

        // Set session ID for the process
        si.lpDesktop = L"winsta0\\default";

        // Path to your ETW monitoring application
        wchar_t modulePath[MAX_PATH];
        GetModuleFileName(NULL, modulePath, MAX_PATH);

        std::wstring fullCommand = std::wstring(L"\"") + modulePath + L"\" --application";

        // Create writable buffer for command line
        std::vector<wchar_t> cmdLine(MAX_PATH);
        wcscpy_s(cmdLine.data(), MAX_PATH, fullCommand.c_str());


        log.Debug(log.APIError(), "To CreateProcessAsUser...");
        // Create process with elevated token
        BOOL success = CreateProcessAsUser(
            hUserToken,                  // Elevated token
            NULL,                        // Application name
            cmdLine.data(),              // Command line
            NULL,                        // Process attributes
            NULL,                        // Thread attributes
            FALSE,                       // Inherit handles
            CREATE_UNICODE_ENVIRONMENT | // Creation flags
                CREATE_NEW_CONSOLE,
            pEnvironment, // Environment
            NULL,         // Current directory
            &si,          // Startup info
            &pi           // Process info
        );

        log.Debug(log.APIError(), "User session application is executed");

        if (success)
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }

        DestroyEnvironmentBlock(pEnvironment);

        CloseHandle(hUserToken);
        return TRUE;
    }

    void Service::StartMonitoring()
    {
        etwMonitor.OnProcessExecuted += ([]()
        {
            log.Debug("Xbox process execution is detected\n" );
            managerState.NotifyRemote(StateEvent::XBOX_DETECTED);
            if (xboxIsDenied)
            {
                KillXbox();
            }
        });

        etwMonitor.OnHomeAppTouched += ([]()
        {
            log.Debug("Attempt to open Home App detected!" );
            managerState.NotifyRemote(StateEvent::OPEN_HOME);
        });

        etwMonitor.OnDeviceFormTouched += ([]()
        {
            log.Debug("Access to DeviceForm detected!" );
            managerState.NotifyRemote(StateEvent::OPEN_DEVICE_FORM);
        });

        etwMonitor.OnFailure += ([]()
        {
            ExitService();
        });

        managerState.OnMonitorRegistry += ([]()
        {
            log.Debug("Monitor registry recieved!" );
            etwMonitor.EnableRegistryProvider();
        });

        managerState.OnExit += ([]()
        {
            log.Debug("Exiting!" );
            ExitService();
        });

        managerState.OnXboxDeny += ([]()
        {
            log.Debug("XboxDeny message recieved!" );
            xboxIsDenied = true;
            KillXbox();
        });

        managerState.OnXboxAllow += ([]()
        {
            log.Debug("XboxAllow message recieved!" );
            xboxIsDenied = false;
        });

        etwMonitor.Start();
        managerState.Start();
    }

    void Service::StopMonitoring()
    {
        etwMonitor.Stop();
        managerState.Stop();
    }

    void Service::KillXbox()
    {
        log.Debug("Killing Xbox");
        if (ERROR_SUCCESS != Process::Kill(XBoxProcessName))
        {
            log.Error(log.APIError(), "Can't Kill XBOX: ");
        }
    }
}
