#include "Tools/Process.hpp"
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "App/AppStateLoop.hpp"
#include "AppControl/AppControlStateLoop.hpp"
#include "AppControl/GamingExperience.hpp"
#include "AppControl/MainWindow.hpp"

namespace AnyFSE::App::AppControl::StateLoop
{
    Logger log = LogManager::GetLogger("Manager/State");

    AppControlStateLoop::AppControlStateLoop(Window::MainWindow &splash)
        : AppStateLoop(false)
        , m_splash(splash)
        , m_preventTimer(0)
        , m_waitLauncherTimer(0)
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

    void AppControlStateLoop::OnGameModeEnter()
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
    void AppControlStateLoop::OnGameModeExit()
    {
        log.Debug("Exited Game Mode, do nothing");
    }

    void AppControlStateLoop::OnDeviceForm()
    {
        m_deviceFormAge = GetTickCount64();
    }

    void AppControlStateLoop::OnOpenHome()
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
        else if (!IsLauncherProcess() || GetTickCount() - m_waitStartTime > 600000)
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

    // State Checkers
    bool AppControlStateLoop::IsInFSEMode()
    {
        return GamingExperience::IsActive();
    }

    bool AppControlStateLoop::IsLauncherActive()
    {
        std::set<DWORD> processIds;
        return  (
            Process::FindAllByName(Config::Launcher.ProcessName, processIds)
            && Process::GetWindow(processIds, Config::Launcher.WindowTitle)
        ) || (
            !Config::Launcher.ProcessNameAlt.empty()
            && Process::FindAllByName(Config::Launcher.ProcessNameAlt, processIds)
            && Process::GetWindow(processIds, Config::Launcher.WindowTitleAlt)
        );
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
        return (GetTickCount64() - m_deviceFormAge) < 50;
    }

    // Actions
    void AppControlStateLoop::ShowSplash()
    {
        log.Debug("Show Splash");
        m_splash.Show();
    }

    void AppControlStateLoop::KillXbox()
    {
        log.Debug("Killing Xbox");
        NotifyRemote(AppEvents::XBOX_DENY);
    }

    void AppControlStateLoop::StartLauncher()
    {
        if (!IsLauncherStarted())
        {
            log.Debug("Start Launcher");
            if (0 == Process::Start(Config::Launcher.StartCommand, Config::Launcher.StartArg))
            {
                log.Error(log.APIError(), "Can't start launcher:" );
            }
        }
        else
        {
            log.Debug("Launcher is Active already");
        }
    }

    void AppControlStateLoop::WaitLauncher()
    {
        if (!m_waitLauncherTimer)
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

        std::set<DWORD> processIds;
        if (Process::FindAllByName(Config::Launcher.ProcessName, processIds))
        {
            HWND launcher = Process::GetWindow(processIds, Config::Launcher.WindowTitle);
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