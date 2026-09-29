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

#include <string>
#include <filesystem>
#include <processenv.h>
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Launchers.hpp"
#include "Tools/Process.hpp"
#include "Tools/Elevated.hpp"
#include "App/Constants.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/Packages.hpp"
#include "App/GamingExperience.hpp"


namespace AnyFSE::App::Launchers
{
    static Logger log = LogManager::GetLogger("Launchers");

    bool WaitLauncherExit()
    {
        while (HANDLE hProcess = Launchers::GetLauncherProcess())
        {
            log.Debug("Start waiting process %#08x for %s", hProcess, Unicode::to_string(Config::Launcher.Name).c_str());

            DWORD waitResult = WAIT_TIMEOUT;
            do
            {
                waitResult = WaitForSingleObject(hProcess, 10000);
                log.Trace("Wait Result: %s", waitResult == WAIT_TIMEOUT ? "Timeout" : "Process completed");

                Config::LoadExitFSEOnHomeExit();

                if (!Config::ExitFSEOnHomeExit
                    || !App::GamingExperience::IsFullscreenMode())
                {
                    return false;
                }

            } while (waitResult == WAIT_TIMEOUT);

            CloseHandle(hProcess);

            return HasLauncherProcess();
        };
        return false;
    }

    void PlayniteOnBoot()
    {
        if (Config::CleanupFailedStart)
        {
            log.Debug("Cleanup Playnite safe startup flag");

            namespace fs = std::filesystem;
            fs::path path = fs::path(Config::Launcher.StartCommand);

            fs::path configPath = path.parent_path();
            bool isPortable = !fs::exists(configPath.append(L"unins000.exe"));

            if (!isPortable)
            {
                log.Debug("Playnite is not portable, config is in %%APPDATA%%");

                wchar_t buffer[MAX_PATH] = {0};
                if (ExpandEnvironmentStringsW(L"%APPDATA%\\Playnite", buffer, MAX_PATH))
                {
                    configPath = fs::path(buffer);
                }
            }

            fs::path flagFile = configPath.append(L"safestart.flag");

            if(fs::exists(flagFile))
            {
                log.Debug("Safestart flag is exist at %s, deleting", flagFile.string().c_str());
                fs::remove(flagFile);
            }
        }
    }

    void PlayniteOnStarted()
    {
        log.Debug("Sending WM_DISPLAYCHANGE");

        HWND hWnd = GetLauncherWindow(true);
        if (!hWnd || !GamingExperience::IsFullscreenMode())
        {
            return;
        }
        // send WM_DISPLAYCHANGED
        HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo = {sizeof(MONITORINFO)};
        if (GetMonitorInfo(hMonitor, &monitorInfo))
        {
            PostMessage(HWND_BROADCAST, WM_DISPLAYCHANGE, 32,
                MAKELPARAM(
                    monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                    monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top
            ));
        }
        FocusLauncher();  // Attempt to fix activation of playnite
    }

    void StartLauncher(bool elevated)
    {
        if (elevated)
        {
            Elevated::Call(Constants::ElevatedStartLauncher);
            return;
        }

        log.Debug("Start Launcher: %s params: %s",
            Unicode::to_string(Config::Launcher.StartCommand).c_str(),
            Unicode::to_string(Config::Launcher.StartArg).c_str()
        );
        if (0 == Process::StartProcess(Config::Launcher.StartCommand, Config::Launcher.StartArg))
        {
            log.Error(log.APIError(), "Can't start launcher:" );
        }
    }

    bool IsLauncherActive()
    {
        return GetLauncherWindow(true);
    }

    bool IsLauncherActiveOrMinimized()
    {
        return GetLauncherWindow(true) != NULL;
    }

    bool IsLauncherMinimized()
    {
        HWND hWnd = GetLauncherWindow(true);
        return hWnd && IsIconic(hWnd);
    }

    void FocusLauncher()
    {
        log.Debug("Focusing Launcher");

        if (!Config::Launcher.ActivationProtocol.empty())
        {
            if (Config::Launcher.ActivationProtocol[0]==L'@')
            {
                Process::StartProcess(Config::Launcher.StartCommand, Config::Launcher.StartArg);
            }
            else
            {
                Process::StartProtocol(Config::Launcher.ActivationProtocol);
            }
            return;
        }

        HWND launcherHwnd = GetLauncherWindow(true);
        if (launcherHwnd)
        {
            Process::BringWindowToForeground(launcherHwnd, SW_SHOWMAXIMIZED);
        }
    }

    HWND GetLauncherWindow(bool includeMinimized)
    {
        HWND launcherHwnd = nullptr;
        const LauncherConfig& launcher = Config::Launcher;

        if (!launcher.AppUserModelID.empty())
        {
            std::vector<DWORD> pids = Tools::Packages::GetAppProcessIds(launcher.AppUserModelID);
            std::set<DWORD> processIds(pids.begin(), pids.end());
            launcherHwnd = Process::GetWindow(processIds, 0, L"", L"", WS_VISIBLE);
        }

        if (!launcherHwnd)
        {
            launcherHwnd = Process::GetWindow(
                launcher.ProcessName, launcher.ExStyle,
                launcher.ClassName, launcher.WindowTitle,
                WS_VISIBLE, launcher.NoStyle
            );
        }

        if (!launcherHwnd && includeMinimized)
        {
            launcherHwnd = Process::GetWindow(
                launcher.ProcessName, launcher.ExStyle,
                launcher.ClassName, launcher.WindowTitle,
                WS_MINIMIZE, launcher.NoStyle
            );
        }

        if (!launcherHwnd)
        {
            launcherHwnd = Process::GetWindow(
                launcher.ProcessNameAlt, launcher.ExStyleAlt,
                launcher.ClassNameAlt, launcher.WindowTitleAlt,
                WS_VISIBLE, launcher.NoStyle
            );
        }

        if (!launcherHwnd && includeMinimized)
        {
            launcherHwnd = Process::GetWindow(
                launcher.ProcessNameAlt, launcher.ExStyleAlt,
                launcher.ClassNameAlt, launcher.WindowTitleAlt,
                WS_MINIMIZE, launcher.NoStyle
            );
        }
        return launcherHwnd;
    }

    bool HasLauncherProcess()
    {
        return (!Config::Launcher.AppUserModelID.empty() && !Tools::Packages::GetAppProcessIds(Config::Launcher.AppUserModelID).empty())
            || 0 != Process::FindFirstByName(Config::Launcher.ProcessName)
            || 0 != Process::FindFirstByName(Config::Launcher.ProcessNameAlt)
            || 0 != Process::FindFirstByExe(Config::Launcher.StartCommand)
            || IsLauncherActiveOrMinimized();
    }

    HANDLE GetLauncherProcess()
    {
        DWORD processId = 0;

        HWND hWnd = GetLauncherWindow(true);
        if (hWnd)
        {
            GetWindowThreadProcessId(hWnd, &processId);
        }

        if (processId == 0)
        {
            return NULL;
        }

        // Open process with desired access
        return OpenProcess(
            SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION,
            FALSE,
            processId
        );
    }

    void LaunchStartupApps(bool asAdmin)
    {
        log.Debug("Launching Startup Applications (AsAdmin=%d)", asAdmin);
        for (const auto &app : Config::StartupApps)
        {
            if (app.Enabled && app.AsAdmin == asAdmin)
            {
                log.Debug("Launching: %s %s", Unicode::to_string(app.Path).c_str(), Unicode::to_string(app.Args).c_str() );
                Process::StartProcess(app.Path, app.Args);
            }
        }
    }

    bool HasStartupApps(bool asAdmin)
    {
        for (const auto &app : Config::StartupApps)
        {
            if (app.Enabled && app.AsAdmin == asAdmin)
            {
                return true;
            }
        }
        return false;
    }

    bool IsPlaynite(LauncherType type)
    {
        return type == LauncherType::PlayniteFullscreen || type == LauncherType::PlayniteDesktop ;
    }

    bool PlayniteSwapLauncher()
    {
        if (Process::FindFirstByName(Config::Launcher.ProcessNameAlt) != 0)
        {
            std::wstring path = std::filesystem::path(Config::Launcher.StartCommand).parent_path().wstring();
            Config::GetLauncherDefaults(
                Config::Launcher.Type == LauncherType::PlayniteFullscreen
                    ? LauncherType::PlayniteDesktop
                    : LauncherType::PlayniteFullscreen,
                path,
                Config::Launcher
            );
        }
        return false;
    }
}
