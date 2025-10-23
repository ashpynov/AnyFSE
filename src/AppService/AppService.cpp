// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include "AppService/AppService.hpp"
#include "AppService/ETWMonitor.hpp"
#include "Logging/LogManager.hpp"
#include "AppService/ProcessEx.hpp"

#include <windows.h>
#include <wtsapi32.h>
#include <UserEnv.h>
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "userenv.lib")

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{

    return TRUE;
}

__declspec(dllexport)
int WINAPI Main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return AnyFSE::App::AppService::AppService::ServiceMain(hInstance, hPrevInstance, lpCmdLine);
}

namespace AnyFSE::App::AppService
{
    static Logger log = LogManager::GetLogger("Service");

    bool                                AppService::xboxIsDenied = false;
    const std::wstring                  AppService::XBoxProcessName(L"XboxPcApp.exe");
    ETWMonitor                          AppService::etwMonitor(XBoxProcessName);
    AppServiceStateLoop                 AppService::AppControlStateLoop;
    HWND                                AppService::m_hwnd;

    BOOL AppService::ExitService()
    {
        PostMessage(m_hwnd, WM_DESTROY, 0, 0);
        return true;
    }

    int WINAPI AppService::ServiceMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine)
    {
        AnyFSE::Logging::LogManager::Initialize("AnyFSE/Service");
        log.Debug("Service is started (hInstance=%08x)", hInstance);

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

    void AppService::CreateBackgroundWindow()
    {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = BackgroundWindowProc;
        wc.lpszClassName = L"AnyFSEService";
        wc.hInstance = GetModuleHandle(NULL);

        RegisterClassEx(&wc);

        m_hwnd = CreateWindowEx(0, L"AnyFSEService",
                                  L"Early start of AnyFSE",  // Descriptive title
                                  0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

        if (m_hwnd)
        {
            log.Debug("Logon monitor background window is created");
            WTSRegisterSessionNotification(m_hwnd, NOTIFY_FOR_ALL_SESSIONS);
        }
    }

    //static
    LRESULT CALLBACK AppService::BackgroundWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_WTSSESSION_CHANGE && wParam == WTS_SESSION_LOGON)
        {
            LaunchAppInUserSession((DWORD)lParam);
        }
        else if (msg == WM_DESTROY)
        {
            AppControlStateLoop.Stop();
            WTSUnRegisterSessionNotification(hwnd);
            PostQuitMessage(0);
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void AppService::MonitorSessions()
    {
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    BOOL AppService::LaunchAppInUserSession(DWORD sessionId)
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

        std::wstring fullCommand = std::wstring(L"\"") + modulePath + L"\" /Control";

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

    void AppService::StartMonitoring()
    {
        etwMonitor.OnProcessExecuted += ([]()
        {
            log.Debug("Xbox process execution is detected\n" );
            AppControlStateLoop.NotifyRemote(AppEvents::XBOX_DETECTED);
            if (xboxIsDenied)
            {
                KillXbox();
            }
        });

        etwMonitor.OnHomeAppTouched += ([]()
        {
            log.Debug("Attempt to open Home App detected!" );
            AppControlStateLoop.NotifyRemote(AppEvents::OPEN_HOME);
        });

        etwMonitor.OnDeviceFormTouched += ([]()
        {
            log.Debug("Access to DeviceForm detected!" );
            AppControlStateLoop.NotifyRemote(AppEvents::OPEN_DEVICE_FORM);
        });

        etwMonitor.OnFailure += ([]()
        {
            ExitService();
        });

        AppControlStateLoop.OnMonitorRegistry += ([]()
        {
            log.Debug("Monitor registry recieved!" );
            etwMonitor.EnableRegistryProvider();
        });

        AppControlStateLoop.OnExit += ([]()
        {
            log.Debug("Exiting!" );
            ExitService();
        });

        AppControlStateLoop.OnXboxDeny += ([]()
        {
            log.Debug("XboxDeny message recieved!" );
            xboxIsDenied = true;
            KillXbox();
        });

        AppControlStateLoop.OnXboxAllow += ([]()
        {
            log.Debug("XboxAllow message recieved!" );
            xboxIsDenied = false;
        });

        etwMonitor.Start();
        AppControlStateLoop.Start();
    }

    void AppService::StopMonitoring()
    {
        etwMonitor.Stop();
        AppControlStateLoop.Stop();
    }

    void AppService::KillXbox()
    {
        log.Debug("Killing Xbox");
        if (ERROR_SUCCESS != ProcessEx::Kill(XBoxProcessName))
        {
            log.Error(log.APIError(), "Can't Kill XBOX: ");
        }
    }
}

