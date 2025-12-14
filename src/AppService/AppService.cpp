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


#include "AppService/AppService.hpp"
#include "AppService/ETWMonitor.hpp"
#include "Logging/LogManager.hpp"
#include "ToolsEx/ProcessEx.hpp"
#include "ToolsEx/TaskManager.hpp"
#include "ToolsEx/Admin.hpp"
#include "ToolsEx/Minidump.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/PowerEfficiency.hpp"

#include "Configuration/Config.hpp"

#include <windows.h>
#include <wtsapi32.h>
#include <UserEnv.h>
#include "AppService.hpp"
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "userenv.lib")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    ToolsEx::InstallUnhandledExceptionHandler();
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

    static const int COMPLETE_EXIT = 0;
    static const int COMPLETE_RESTART = 1;
    static const int COMPLETE_RELOAD = 2;
    static const int COMPLETE_SUSPEND = 3;

    BOOL AppService::ExitService(int nState)
    {
        PostMessage(m_hwnd, WM_DESTROY, nState, 0);
        return true;
    }

    bool AppService::IsSystemAccount()
    {
        HANDLE hToken = NULL;
        DWORD dwSize = 0;
        PTOKEN_USER pTokenUser = NULL;
        bool isSystem = false;

        // Open process token
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            return false;

        // Get token information size
        GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            pTokenUser = (PTOKEN_USER)LocalAlloc(LPTR, dwSize);
            if (pTokenUser)
            {
                // Get token user information
                if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize))
                {
                    // Lookup account name from SID
                    WCHAR name[256];
                    WCHAR domain[256];
                    DWORD nameSize = 256;
                    DWORD domainSize = 256;
                    SID_NAME_USE sidType;

                    if (LookupAccountSid(NULL, pTokenUser->User.Sid,
                                         name, &nameSize,
                                         domain, &domainSize,
                                         &sidType))
                    {

                        // Check if it's SYSTEM account
                        isSystem = (_wcsicmp(name, L"SYSTEM") == 0 &&
                                    _wcsicmp(domain, L"NT AUTHORITY") == 0);

                        log.Debug("Service is started by %s/%s",
                            Unicode::to_string(domain).c_str(),
                            Unicode::to_string(name).c_str()
                        );
                    }
                }
                LocalFree(pTokenUser);
            }
        }

        CloseHandle(hToken);
        return isSystem;
    }

    bool AppService::StartServiceTask()
    {
        if (ToolsEx::Admin::IsRunningAsAdministrator())
        {
            log.Debug("Is Elevated permissions => Create and Start task");
            ToolsEx::TaskManager::CreateTask();
            ToolsEx::TaskManager::StartTask();
        }
        else
        {
            MessageBox(NULL,
                L"AnyFSE.Service should be executed as scheduled task, not as regular application.",
                L"Error",
                MB_ICONERROR | MB_OK
            );
        }
        return true;
    }

    int WINAPI AppService::ServiceMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine)
    {
        Config::Load();
        AnyFSE::Logging::LogManager::Initialize("AnyFSE/Service", Config::LogLevel, Config::LogPath);

        if (FindWindow(L"AnyFSEService", nullptr))
        {
            log.Debug("Service message window is existed already, exiting");
            return -1;
        }
        if (!IsSystemAccount())
        {
            log.Debug("Not System account, starting task");
            StartServiceTask();
            return -1;
        }

        log.Debug("Service is started (hInstance=%08x), cmdLine: %s", hInstance, lpCmdLine);

        RegisterEvents();
        AppControlStateLoop.Start();


        DWORD activeSession = WTSGetActiveConsoleSessionId();
        if (activeSession != 0xFFFFFFFF)
        {
            log.Debug("Session is started already force to run\n");
            LaunchAppInUserSession(activeSession);
        }

        bool bNoClient = false;
        int result = 0;

        do
        {
            log.Debug("Entering Main service loop, noClient mode %s", bNoClient ? "On" : "Off");

            AppControlStateLoop.Notify((Config::AggressiveMode && !bNoClient) ? AppEvents::XBOX_DENY : AppEvents::XBOX_ALLOW);

            CreateBackgroundWindow();
            StartMonitoring(bNoClient || Config::Launcher.Type == LauncherType::None || Config::Launcher.Type == LauncherType::Xbox);

            Tools::EnablePowerEfficencyMode(true);
            int result = MonitorSessions();

            StopMonitoring();

            switch (result)
            {
                case COMPLETE_EXIT:
                    log.Debug("Work is done");
                    break;

                case COMPLETE_RELOAD:
                    log.Debug("Reload");
                    Config::Load();
                    AnyFSE::Logging::LogManager::Initialize("AnyFSE/Service", Config::LogLevel, Config::LogPath);
                    bNoClient = false;
                    break;

                case COMPLETE_SUSPEND:
                    log.Debug("Switch to suspend mode");
                    bNoClient = true;
                    break;

                case COMPLETE_RESTART:
                    log.Debug("Switch to normal mode");
                    Restart();
                    break;

                default:
                    log.Debug("Unknown error: %d, restart", result);
                    Restart();
                    break;
            }
        } while (result != COMPLETE_RELOAD && result != COMPLETE_SUSPEND);

        AppControlStateLoop.Stop();

        log.Debug("Exiting %d\n", result);

        return result;
    }

    void AppService::Restart(bool bSuspended)
    {
        // Get current executable path
        TCHAR modulePath[MAX_PATH];
        GetModuleFileName(NULL, modulePath, MAX_PATH);


        std::wstring fullCommand = std::wstring(L"\"") + modulePath + L"\"";

        if (bSuspended)
        {
            fullCommand += L" /suspend";
        }

        // Create writable buffer for command line
        std::vector<wchar_t> cmdLine(MAX_PATH);
        wcscpy_s(cmdLine.data(), MAX_PATH, fullCommand.c_str());

        // Prepare startup info
        STARTUPINFO si = { sizeof(STARTUPINFO) };
        PROCESS_INFORMATION pi;

        // Create the new process
        if (CreateProcess(NULL,         // Executable path
                        cmdLine.data(), // Command line
                        NULL,           // Process security
                        NULL,           // Thread security
                        FALSE,          // Inherit handles
                        0,              // Creation flags
                        NULL,           // Environment
                        NULL,           // Current directory
                        &si,            // Startup info
                        &pi))           // Process info
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
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
        else if (msg == WM_USER)
        {
            RegisterEvents();
            return 0;
        }
        else if (msg == WM_DESTROY)
        {
            WTSUnRegisterSessionNotification(hwnd);
            PostQuitMessage((int)wParam);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    int AppService::MonitorSessions()
    {
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return (int)msg.wParam;
    }

    BOOL AppService::LaunchAppInUserSession(DWORD sessionId)
    {
        log.Debug("LaunchAppInUserSession");

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

        wchar_t * path = wcsrchr(modulePath,'\\');
        if (path)
        {
            *path = '\0';
        }

        std::wstring fullCommand = std::wstring(L"\"") + modulePath + L"\\AnyFSE.exe\" /Control";

        // Create writable buffer for command line
        std::vector<wchar_t> cmdLine(MAX_PATH);
        wcscpy_s(cmdLine.data(), MAX_PATH, fullCommand.c_str());


        log.Debug("To CreateProcessAsUser...");
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

        log.Debug("User session application is executed");

        if (success)
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }

        DestroyEnvironmentBlock(pEnvironment);

        CloseHandle(hUserToken);
        return TRUE;
    }

    void AppService::RegisterEvents()
    {
        AppControlStateLoop.OnExit = ([]()
        {
            log.Debug("Reloading!" );
            AppControlStateLoop.NotifyRemote(AppEvents::DISCONNECT);
            ExitService(COMPLETE_EXIT);
        });

        AppControlStateLoop.OnRestart = ([]()
        {
            log.Debug("Restarting!" );
            AppControlStateLoop.NotifyRemote(AppEvents::DISCONNECT);
            ExitService(COMPLETE_RESTART);
        });

        AppControlStateLoop.OnSuspend = ([]()
        {
            log.Debug("Restarting!" );
            AppControlStateLoop.NotifyRemote(AppEvents::DISCONNECT);
            ExitService(COMPLETE_SUSPEND);
        });


        AppControlStateLoop.OnReload = ([]()
        {
            log.Debug("Config reload!" );
            ExitService(COMPLETE_RELOAD);
        });

        etwMonitor.OnProcessExecuted = ([]()
        {
            log.Debug("Xbox process execution is detected\n" );
            AppControlStateLoop.NotifyRemote(AppEvents::XBOX_DETECTED);
            if (xboxIsDenied)
            {
                KillXbox();
            }
        });

        etwMonitor.OnHomeAppTouched = ([]()
        {
            log.Trace("Attempt to open Home App detected!" );
            AppControlStateLoop.NotifyRemote(AppEvents::OPEN_HOME);
        });

        etwMonitor.OnDeviceFormTouched = ([]()
        {
            log.Trace("Access to DeviceForm detected!" );
            AppControlStateLoop.NotifyRemote(AppEvents::OPEN_DEVICE_FORM);
        });

        etwMonitor.OnLauncherStopped = ([]()
        {
            log.Debug("LauncherPtocess stopped\n" );
            AppControlStateLoop.NotifyRemote(AppEvents::LAUNCHER_STOPPED);
        });

        etwMonitor.OnFailure = ([]()
        {
            ExitService(COMPLETE_EXIT);
        });

        AppControlStateLoop.OnXboxDeny = ([]()
        {
            log.Debug("XboxDeny message recieved!" );
            xboxIsDenied = true;
            KillXbox();
        });

        AppControlStateLoop.OnXboxAllow = ([]()
        {
            log.Debug("XboxAllow message recieved!" );
            xboxIsDenied = false;
        });
    }

    void AppService::StartMonitoring(bool bSuspended)
    {
        if (!bSuspended)
        {
            etwMonitor.Start();
        }
    }

    void AppService::StopMonitoring()
    {
        etwMonitor.Stop();
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

