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
#include "Tools/Unicode.hpp"


namespace AnyFSE::App::Launchers
{
    static Logger log = LogManager::GetLogger("Launchers");

    void LauncherOnBoot()
    {
        switch (Config::Launcher.Type)
        {
            case LauncherType::PlayniteDesktop:
            case LauncherType::PlayniteFullscreen:
                return PlayniteOnBoot();
        };
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

    void StartLauncher()
    {
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
        const LauncherConfig& launcher = Config::Launcher;
        return GetLauncherWindow();
    }

    void FocusLauncher()
    {
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

        HWND launcherHwnd = GetLauncherWindow();
        if (launcherHwnd)
        {
            Process::BringWindowToForeground(launcherHwnd, SW_SHOWMAXIMIZED);

            HMONITOR hMonitor = MonitorFromWindow(launcherHwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof(monitorInfo);

            if (GetMonitorInfo(hMonitor, &monitorInfo))
            {
                PostMessage(HWND_BROADCAST, WM_DISPLAYCHANGE, 32,
                    MAKELPARAM(
                        monitorInfo.rcMonitor.right-monitorInfo.rcMonitor.left,
                        monitorInfo.rcMonitor.bottom-monitorInfo.rcMonitor.top
                ));
            }
        }
    }

    HWND GetLauncherWindow()
    {
        const LauncherConfig& launcher = Config::Launcher;
        HWND launcherHwnd = Process::GetWindow(
            launcher.ProcessName, launcher.ExStyle,
            launcher.ClassName, launcher.WindowTitle,
            WS_VISIBLE, launcher.NoStyle
        );

        if (!launcherHwnd)
        {
            launcherHwnd = Process::GetWindow(
                launcher.ProcessNameAlt, launcher.ExStyleAlt,
                launcher.ClassNameAlt, launcher.WindowTitleAlt,
                WS_VISIBLE, launcher.NoStyle
            );
        }
        return launcherHwnd;
    }

    HANDLE GetLauncherProcess()
    {
        HWND hWnd = GetLauncherWindow();
        if (!hWnd)
        {
            return NULL;
        }

        DWORD processId;
        GetWindowThreadProcessId(hWnd, &processId);
        if (processId == 0)
        {
            return false;
        }

        // Open process with desired access
        return OpenProcess(
            SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION,
            FALSE,
            processId
        );
    }
}