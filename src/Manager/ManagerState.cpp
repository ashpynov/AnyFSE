#include "Configuration/Config.hpp"
#include "Monitors/GamingExperience.hpp"
#include "Tools/Process.hpp"
#include "Logging/LogManager.hpp"
#include "Window/MainWindow.hpp"
#include "ManagerState.hpp"

namespace AnyFSE::Manager::State
{
    Logger log = LogManager::GetLogger("Manager/State");

    ManagerState::ManagerState(MainWindow &splash)
        : ManagerCycle(false)
        , m_splash(splash)
        , m_preventTimer(0)
        , m_waitLauncherTimer(0)
    {

    }

    ManagerState::~ManagerState()
    {
        if (m_preventTimer) CancelTimer(m_preventTimer);
        if (m_waitLauncherTimer) CancelTimer(m_waitLauncherTimer);
    }

    void ManagerState::ProcessEvent(StateEvent event)
    {
        switch (event)
        {
            case StateEvent::START:             return OnStart();
            case StateEvent::XBOX_DETECTED:     return OnXboxDetected();
            case StateEvent::GAMEMODE_ENTER:    return OnGameModeEnter();
            case StateEvent::GAMEMODE_EXIT:     return OnGameModeExit();
            case StateEvent::OPEN_HOME:         return OnOpenHome();
            case StateEvent::OPEN_DEVICE_FORM:  return OnDeviceForm();
            default: break;
        }
    }

    void ManagerState::OnStart()
    {
        if (IsInFSEMode())
        {
            ShowSplash();
            KillXbox();
            StartLauncher();
            WaitLauncher();
        }
    }

    void ManagerState::OnXboxDetected()
    {
        //if (IsInFSEMode() || Config::AggressiveMode)
        {
            log.Debug("OnXboxDetected: XBoxStartDetected in %s Mode", IsInFSEMode() ? "FSE" : "Desktop" );

            if (IsSplashActive() || IsPreventIsActive() || IsWaitingLauncher())
            {
                log.Debug("OnXboxDetected: During %s => Kill it", 
                      IsSplashActive()    ? "Splash is Active" 
                    : IsPreventIsActive() ? "Prevent is Active" 
                    : "Waiting Launcher");
                KillXbox();
            }
            else if (!IsLauncherActive())
            {
                log.Debug("OnXboxDetected: Launcher is not active => Kill It. Start Launcher with splash");
                if (false)
                {
                    ShowSplash();
                    KillXbox();
                    StartLauncher();
                    WaitLauncher();
                } 
                else
                {
                    log.Debug("OnXboxDetected: Launcher is not active //// BUT DISABLED");
                }
            }
            else if (Config::AggressiveMode)
            {
                log.Debug("OnXboxDetected: Aggressive mode => Kill It. Start Launcher with splash");
                ShowSplash();
                KillXbox();
                StartLauncher();
                WaitLauncher();
            }
            else 
            {
                log.Debug("OnXboxDetected: Do nothing");
            }
        }
        m_xboxAge = GetTickCount64();
    }

    void ManagerState::OnGameModeEnter()
    {
        if (!IsLauncherStarted())
        {
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
    void ManagerState::OnGameModeExit()
    {
        log.Debug("Exited Game Modoe, do nothing");
    }

    void ManagerState::OnDeviceForm()
    {
        m_deviceFormAge = GetTickCount64();
    }

    void ManagerState::OnOpenHome()
    {
        if (!IsOnTaskSwitcher())
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
                if (IsInFSEMode())
                {
                    ShowSplash();
                }
                StartLauncher();
                WaitLauncher();
            }
        }
        else
        {
            log.Debug("Home App Detected in Task switcher case (%d) ms", GetTickCount64() - m_deviceFormAge);
        }
    }

    void ManagerState::OnLauncherTimer()
    {
        log.Debug("Launcher check %d ms", GetTickCount() - m_waitStartTime);
        if (IsLauncherActive())
        {
            log.Debug("Launcher activated");
            ManagerCycle::CancelTimer(m_waitLauncherTimer);
            m_waitLauncherTimer = 0;
            FocusLauncher();
            PreventTimeout();
            CloseSplash();
        }
        else if (!IsLauncherProcess() || GetTickCount() - m_waitStartTime > 600000)
        {
            log.Debug("Launcher not started, starting");
            ManagerCycle::CancelTimer(m_waitLauncherTimer);
            m_waitLauncherTimer = 0;
            StartLauncher();
            WaitLauncher();
        }
    }

    void ManagerState::OnPreventTimer()
    {
        log.Debug("Prevent Timer Completed");
        m_preventTimer = 0;
        NotifyRemote(StateEvent::XBOX_ALLOW);
    }

    // State Checkers
    bool ManagerState::IsInFSEMode()
    {
        return GamingExperience::IsActive();
    }

    bool ManagerState::IsLauncherActive()
    {
        std::set<DWORD> processIds;
        return  (
            Process::FindAllByName(Config::LauncherProcessName, processIds)
            && Process::GetWindow(processIds, Config::LauncherWindowName)
        ) || (
            !Config::LauncherProcessNameAlt.empty()
            && Process::FindAllByName(Config::LauncherProcessNameAlt, processIds)
            && Process::GetWindow(processIds, Config::LauncherWindowNameAlt)
        );
    }

    bool ManagerState::IsLauncherStarted()
    {
        return Config::LauncherIsTrayAggressive ? IsLauncherActive() : IsLauncherProcess();
    }
    bool ManagerState::IsLauncherProcess()
    {
        return 0 != Process::FindFirstByName(Config::LauncherProcessName)
            || 0 != Process::FindFirstByName(Config::LauncherProcessNameAlt);
    }

    bool ManagerState::IsSplashActive()
    {
        return m_splash.IsVisible();
    }

    bool ManagerState::IsPreventIsActive()
    {
        return 0 != m_preventTimer;
    }

    bool ManagerState::IsWaitingLauncher()
    {
        return 0 != m_waitLauncherTimer;
    }

    bool ManagerState::IsYoungXbox()
    {
        return (GetTickCount64() - m_xboxAge) < 2000;
    }

    bool ManagerState::IsOnTaskSwitcher()
    {
        return (GetTickCount64() - m_deviceFormAge) < 50;
    }

    // Actions
    void ManagerState::ShowSplash()
    {
        log.Debug("Show Splash");
        m_splash.Show();
    }

    void ManagerState::KillXbox()
    {
        log.Debug("Killing Xbox");
        NotifyRemote(StateEvent::XBOX_DENY);
    }

    void ManagerState::StartLauncher()
    {
        if (!IsLauncherStarted())
        {
            log.Debug("Start Launcher");
            if (0 == Process::Start(Config::LauncherStartCommand, Config::LauncherStartCommandArgs))
            {
                log.Error(log.APIError(), "Can't start launcher:" );
            }
        }
        else
        {
            log.Debug("Launcher is Active already");
        }
    }

    void ManagerState::WaitLauncher()
    {
        if (!m_waitLauncherTimer)
        {
            log.Debug("Set timer for await Launcher");
            m_waitStartTime = GetTickCount();
            m_waitLauncherTimer = ManagerCycle::SetTimer(std::chrono::milliseconds(500), [this](){ this->OnLauncherTimer(); }, true );
        }
    }

    void ManagerState::CloseSplash()
    {
        log.Debug("Close Splash");
        m_splash.Hide();
    }

    void ManagerState::PreventTimeout()
    {
        if (!m_preventTimer)
        {
            log.Debug("Set timer to prevent XBox");
            m_preventTimer = ManagerCycle::SetTimer(std::chrono::milliseconds(3000), [this](){ this->OnPreventTimer(); }, false );
        }
    }

    void ManagerState::FocusLauncher()
    {
        log.Debug("Focus Launcher");

        std::set<DWORD> processIds;
        if (Process::FindAllByName(Config::LauncherProcessName, processIds))
        {
            HWND launcher = Process::GetWindow(processIds, Config::LauncherWindowName);
            if (launcher)
            {
                ShowWindow(launcher, SW_RESTORE);
                SetWindowPos(launcher, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                SetForegroundWindow(launcher);
                SetFocus(launcher);
            }
        }
    }

    void SuppressXboxActivation()
    {
        //Window -> Class ApplicationFrameWindow, Name Xbox

    }
}