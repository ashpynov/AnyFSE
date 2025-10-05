#include "Configuration/Config.hpp"
#include "Monitors/GamingExperience.hpp"
#include "Process/Process.hpp"
#include "Logging/LogManager.hpp"
#include "Window/MainWindow.hpp"
#include "ManagerState.hpp"

namespace AnyFSE::Manager::State
{
    Logger log = LogManager::GetLogger("Manager/State");

    ManagerState::ManagerState(MainWindow &splash)
        : m_splash(splash),
        m_preventTimer(0),
        m_waitLauncherTimer(0)
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
        if (IsInFSEMode() || Config::AggressiveMode)
        {
            log.Info("XBoxStartDetected in %s Mode", Config::AggressiveMode ? "Aggressive" : "FSE");

            if (IsSplashActive() || IsPreventIsActive())
            {
                KillXbox();
            }
            else if (!IsLauncherActive()) 
            {
                ShowSplash();
                KillXbox();
                StartLauncher();
                WaitLauncher();
            }
            else if (Config::AggressiveMode)
            {
                KillXbox();
                PreventTimeout();
                FocusLauncher();
            }
        } 
    }

    void ManagerState::OnGameModeEnter()
    {
        if (!IsLauncherProcess())
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

    void ManagerState::OnLauncherTimer()
    {
        log.Info("Launcher check");
        if (IsLauncherActive())
        {
            log.Info("Launcher activated");
            ManagerCycle::CancelTimer(m_waitLauncherTimer);
            m_waitLauncherTimer = 0;
            PreventTimeout();
            CloseSplash();
        }
    }

    void ManagerState::OnPreventTimer()
    {
        log.Info("Prevent Timer Completed");
        m_preventTimer = 0;
    }

    // State Checkers
    bool ManagerState::IsInFSEMode() 
    { 
        return GamingExperience::IsActive();
    }

    bool ManagerState::IsLauncherActive()
    {
        DWORD processId = Process::FindByName(Config::LauncherProcessName);
        return processId && Process::GetWindow(processId, Config::LauncherWindowName);
    }

    bool ManagerState::IsLauncherProcess()
    {
        return 0 != Process::FindByName(Config::LauncherProcessName);
    }

    bool ManagerState::IsSplashActive()
    {
        return m_splash.IsVisible();
    }

    bool ManagerState::IsPreventIsActive()
    {
        return 0 != m_preventTimer;
    }

    // Actions
    void ManagerState::ShowSplash()
    {
        log.Info("Show Splash");
        m_splash.Show();
    }
    
    void ManagerState::KillXbox()
    {
        log.Info("Killing Xbox");
        if (ERROR_SUCCESS != Process::Kill(Config::XBoxProcessName))
        {
            log.Error(log.APIError(), "Can't Kill XBOX: ");
        }
    }

    void ManagerState::StartLauncher()
    {
        if (!IsLauncherProcess())
        {
            log.Info("Start Launcher");
            if (0 == Process::Start(Config::LauncherStartCommand, Config::LauncherStartCommandArgs))
            {
                log.Error(log.APIError(), "Can't start launcher:" );
            }
        }
    }

    void ManagerState::WaitLauncher()
    {
        if (!m_waitLauncherTimer)
        {
            log.Info("Set timer for await Launcher");
            m_waitLauncherTimer = ManagerCycle::SetTimer(std::chrono::milliseconds(500), [this](){ this->OnLauncherTimer(); }, true );
        }
    }

    void ManagerState::CloseSplash()
    {
        log.Info("Close Splash");
        m_splash.Hide();
    }

    void ManagerState::PreventTimeout()
    {
        if (!m_preventTimer)
        {
            log.Info("Set timer to prevent XBox");
            m_preventTimer = ManagerCycle::SetTimer(std::chrono::milliseconds(3000), [this](){ this->OnPreventTimer(); }, false );
        }
    }

    void ManagerState::FocusLauncher()
    {
        log.Info("Focus Launcher");
        DWORD processId = Process::FindByName(Config::LauncherProcessName);
        if (processId)
        {
            HWND launcher = Process::GetWindow(processId, Config::LauncherWindowName);
            if (launcher)
            {
                ShowWindow(launcher, SW_SHOWMAXIMIZED);
                SetForegroundWindow(launcher);
            }
        }
    }
}