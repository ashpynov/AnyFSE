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

#include "Tools/Process.hpp"
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "App/AppStateLoop.hpp"
#include "AppControl/AppControlStateLoop.hpp"
#include "AppControl/GamingExperience.hpp"
#include "AppControl/MainWindow.hpp"
#include "AppControlStateLoop.hpp"

namespace AnyFSE::App::AppControl::StateLoop
{
    Logger log = LogManager::GetLogger("Manager/State");

    AppControlStateLoop::AppControlStateLoop(Window::MainWindow &splash)
        : AppStateLoop(false)
        , m_splash(splash)
        , m_preventTimer(0)
        , m_waitLauncherTimer(0)
        , m_exitingTimer(0)
        , m_restartDelayTimer(0)
        , m_isRestart(false)
    {

    }

    AppControlStateLoop::~AppControlStateLoop()
    {
        if (m_preventTimer) CancelTimer(m_preventTimer);
        if (m_waitLauncherTimer) CancelTimer(m_waitLauncherTimer);
    }

    void AppControlStateLoop::ProcessEvent(AppEvents event)
    {
        switch (event)
        {
            case AppEvents::START:             return OnStart();
            case AppEvents::XBOX_DETECTED:     return OnXboxDetected();
            case AppEvents::GAMEMODE_ENTER:    return OnGameModeEnter();
            case AppEvents::GAMEMODE_EXIT:     return OnGameModeExit();
            case AppEvents::OPEN_HOME:         return OnOpenHome();
            case AppEvents::OPEN_DEVICE_FORM:  return OnDeviceForm();
            case AppEvents::QUERY_END_SESSION: return OnQueryEndSession();
            case AppEvents::END_SESSION:       return OnEndSession();
            default: break;
        }
    }

    void AppControlStateLoop::OnStart()
    {
        if (IsInFSEMode())
        {
            ShowSplash();
            KillXbox();
            StartLauncher();
            WaitLauncher();
        }
    }

    void AppControlStateLoop::OnXboxDetected()
    {
        log.Debug("OnXboxDetected: XBoxStartDetected in %s Mode", IsInFSEMode() ? "FSE" : "Desktop" );

        if (IsSplashActive() || IsPreventIsActive() || IsWaitingLauncher())
        {
            log.Debug("OnXboxDetected: During %s => Kill it",
                  IsSplashActive()      ? "Splash is Active"
                : IsPreventIsActive()   ? "Prevent is Active"
                : "Waiting Launcher");
            KillXbox();
        }
        else if (!IsLauncherActive() && !Config::AggressiveMode)
        {
            log.Debug("OnXboxDetected: Launcher is not active => Kill It. Start Launcher with splash");
            if (IsInFSEMode())
            {
                ShowSplash();
                KillXbox();
                RestartLauncher();
                WaitLauncher();
            }
            else
            {
                log.Debug("OnXboxDetected: Launcher is not active, In Desktop mode");
            }
        }
        else if (Config::AggressiveMode)
        {
            log.Debug("OnXboxDetected: Aggressive mode => Kill It. Start Launcher with splash");
            ShowSplash();
            KillXbox();
            RestartLauncher();
            WaitLauncher();
        }
        else
        {
            log.Debug("OnXboxDetected: Do nothing");
        }

        m_xboxAge = GetTickCount64();
    }

    void AppControlStateLoop::OnGameModeEnter()
    {
        if (!IsLauncherStarted())
        {
            m_isRestart = false;
            ShowSplash();
            StartLauncher();
            WaitLauncher();
        }
        else if (!IsLauncherActive())
        {
            WaitLauncher();
        }
        else
        {
            PreventTimeout();
            FocusLauncher();
        }
    }
    void AppControlStateLoop::OnGameModeExit()
    {
        log.Debug("Exited Game Mode, do nothing");
    }

    void AppControlStateLoop::OnDeviceForm()
    {
        m_deviceFormAge = GetTickCount64();
    }

    void AppControlStateLoop::OnQueryEndSession()
    {
        m_exitingTimer = AppStateLoop::SetTimer(std::chrono::seconds(30), [this](){ this->OnExitingTimer();}, false );
    }

    void AppControlStateLoop::OnEndSession()
    {

    }

    void AppControlStateLoop::OnOpenHome()
    {
        if (Config::AggressiveMode)
        {
            log.Debug("During AggressiveMode detection of Accessing to HomeApp is skipped");
        }
        else if (!IsOnTaskSwitcher())
        {
            log.Debug("OnOpenHome: Accessing to HomeApp registry key was Detected");

            if (IsYoungXbox() )
            {
                log.Debug("OnOpenHome: Home App detected on young XBox, do nothing");
                return;
            }

            if (IsSplashActive() || IsPreventIsActive() || IsWaitingLauncher())
            {
                log.Debug("OnOpenHome: Already in progress %s",
                      IsSplashActive()    ? "Splash is Active"
                    : IsPreventIsActive() ? "Prevent is Active"
                    : "Waiting Launcher");
                return;
            }
            if (IsLauncherActive())
            {
                log.Debug("OnOpenHome: Launcher is already active => prevent xBox, Focus home");
                PreventTimeout();
                FocusLauncher();
            }
            else
            {
                log.Debug("OnOpenHome: Launcher is need to run  => prevent xBox, launch home");
                if (IsInFSEMode() || Config::SplashShowVideo)
                {
                    ShowSplash();
                }
                RestartLauncher();
                WaitLauncher();
            }
        }
        else
        {
            log.Trace("Home App Detected After DeviceForm: age (%d) ms", GetTickCount64() - m_deviceFormAge);
        }
    }

    void AppControlStateLoop::OnLauncherTimer()
    {
        log.Trace("Launcher check %d ms", GetTickCount() - m_waitStartTime);
        if (IsLauncherActive())
        {
            log.Debug("Launcher activated");
            AppStateLoop::CancelTimer(m_waitLauncherTimer);
            m_waitLauncherTimer = 0;
            FocusLauncher();
            PreventTimeout();
            CloseSplash();
        }
        else if (!IsLauncherProcess() || GetTickCount() - m_waitStartTime > 60000)
        {
            log.Debug("Launcher not started, starting");
            AppStateLoop::CancelTimer(m_waitLauncherTimer);
            m_waitLauncherTimer = 0;
            StartLauncher();
            WaitLauncher();
        }
    }

    void AppControlStateLoop::OnPreventTimer()
    {
        log.Debug("Prevent Timer Completed");
        m_preventTimer = 0;
        NotifyRemote(AppEvents::XBOX_ALLOW);
    }

    void AppControlStateLoop::OnExitingTimer()
    {
        log.Debug("OnExitingTimer completed and we are still alive?????");
        m_exitingTimer = 0;
        CloseSplash();
    }

    // State Checkers
    bool AppControlStateLoop::IsInFSEMode()
    {
        return GamingExperience::IsActive();
    }

    bool AppControlStateLoop::IsLauncherActive()
    {
        std::set<DWORD> processIds;
        const LauncherConfig& launcher = Config::Launcher;
        return GetLauncherWindow();
    }

    bool AppControlStateLoop::IsLauncherStarted()
    {
        return Config::Launcher.IsTrayAggressive ? IsLauncherActive() : IsLauncherProcess();
    }
    bool AppControlStateLoop::IsLauncherProcess()
    {
        return 0 != Process::FindFirstByName(Config::Launcher.ProcessName)
            || 0 != Process::FindFirstByName(Config::Launcher.ProcessNameAlt);
    }

    bool AppControlStateLoop::IsSplashActive()
    {
        return m_splash.IsVisible();
    }

    bool AppControlStateLoop::IsPreventIsActive()
    {
        return 0 != m_preventTimer;
    }

    bool AppControlStateLoop::IsWaitingLauncher()
    {
        return 0 != m_waitLauncherTimer;
    }

    bool AppControlStateLoop::IsYoungXbox()
    {
        return (GetTickCount64() - m_xboxAge) < 2000;
    }

    bool AppControlStateLoop::IsOnTaskSwitcher()
    {
        return (GetTickCount64() - m_deviceFormAge) < 1000;
    }

    bool AppControlStateLoop::IsExiting()
    {
        return 0 != m_exitingTimer;
    }

    // Actions
    void AppControlStateLoop::ShowSplash()
    {
        log.Debug("Show Splash");
        m_splash.Show(IsExiting() || m_isRestart);
    }

    void AppControlStateLoop::StartSplash()
    {
        log.Debug("Start Splash");
        m_splash.Start();
    }

    void AppControlStateLoop::KillXbox()
    {
        log.Debug("Killing Xbox");
        NotifyRemote(AppEvents::XBOX_DENY);
    }

    void AppControlStateLoop::StartLauncher()
    {
        if (IsExiting())
        {
            log.Debug("During exiting procedure ignore launcher start");
        }
        else if (!IsLauncherStarted())
        {
            log.Debug("Start Launcher");
            if (0 == Process::StartProcess(Config::Launcher.StartCommand, Config::Launcher.StartArg))
            {
                log.Error(log.APIError(), "Can't start launcher:" );
            }
        }
        else
        {
            log.Debug("Launcher is Active already");
        }
        m_isRestart = (bool)Config::RestartDelay;
    }

    void AppControlStateLoop::RestartLauncher()
    {
        if (!m_isRestart || !Config::RestartDelay)
        {
            return StartLauncher();
        }
        if (!m_restartDelayTimer )
        {
            m_restartDelayTimer = AppStateLoop::SetTimer(
                std::chrono::milliseconds(Config::RestartDelay),
                [this]()
                {
                    this->StartSplash();
                    this->StartLauncher();
                    this->m_restartDelayTimer = 0;
                },
                false );
        }
    }

    void AppControlStateLoop::WaitLauncher()
    {
        if (!m_waitLauncherTimer && !m_exitingTimer)
        {
            log.Debug("Set timer for await Launcher");
            m_waitStartTime = GetTickCount();
            m_waitLauncherTimer = AppStateLoop::SetTimer(std::chrono::milliseconds(500), [this](){ this->OnLauncherTimer(); }, true );
        }
    }

    void AppControlStateLoop::CloseSplash()
    {
        log.Debug("Close Splash");
        m_splash.Hide();
    }

    void AppControlStateLoop::PreventTimeout()
    {
        if (!m_preventTimer)
        {
            log.Debug("Set timer to prevent XBox");
            m_preventTimer = AppStateLoop::SetTimer(std::chrono::milliseconds(3000), [this](){ this->OnPreventTimer(); }, false );
        }
    }

    void AppControlStateLoop::FocusLauncher()
    {
        log.Debug("Focus Launcher");

        HWND launcherHwnd = GetLauncherWindow();
        if (launcherHwnd)
        {
            BringWindowToTop(launcherHwnd);
            SetForegroundWindow(launcherHwnd);
            SetActiveWindow(launcherHwnd);

            HMONITOR hMonitor = MonitorFromWindow(launcherHwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof(monitorInfo);

            if (GetMonitorInfo(hMonitor, &monitorInfo))
            {
                PostMessage(launcherHwnd, WM_DISPLAYCHANGE, 32,
                    MAKELPARAM(
                        monitorInfo.rcMonitor.right-monitorInfo.rcMonitor.left,
                        monitorInfo.rcMonitor.bottom-monitorInfo.rcMonitor.top
                ));
            }
        }
    }

    HWND AppControlStateLoop::GetLauncherWindow()
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

    void SuppressXboxActivation()
    {
        //Window -> Class ApplicationFrameWindow, Name Xbox

    }
}