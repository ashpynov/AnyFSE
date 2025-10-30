#pragma once
#include "App/AppStateLoop.hpp"
#include "App/AppEvents.hpp"

namespace AnyFSE::App::AppControl::Window { class MainWindow; }

namespace AnyFSE::App::AppControl::StateLoop
{

    class AppControlStateLoop: public App::StateLoop::AppStateLoop
    {
        private:
            Window::MainWindow &m_splash;
            std::uint64_t m_preventTimer;
            std::uint64_t m_waitLauncherTimer;
            std::uint64_t m_exitingTimer;


            LONGLONG m_xboxAge;
            LONGLONG m_deviceFormAge;

            DWORD launcherPid;
            DWORD m_waitStartTime;


        public:
            AppControlStateLoop(Window::MainWindow &splash);
            ~AppControlStateLoop();

        private:
            virtual void ProcessEvent(AppEvents event);
            void OnStart();
            void OnXboxDetected();
            void OnGameModeEnter();
            void OnGameModeExit();
            void OnDeviceForm();
            void OnQueryEndSession();
            void OnEndSession();
            void OnOpenHome();
            void OnLauncherTimer();
            void OnPreventTimer();
            void OnExitingTimer();

            bool IsInFSEMode();
            bool IsLauncherActive();
            bool IsLauncherStarted();
            bool IsLauncherProcess();
            bool IsSplashActive();
            bool IsPreventIsActive();
            bool IsWaitingLauncher();
            bool IsYoungXbox();
            bool IsOnTaskSwitcher();
            bool IsExiting();

            void ShowSplash();
            void KillXbox();
            void StartLauncher();
            void WaitLauncher();
            void CloseSplash();
            void PreventTimeout();
            void FocusLauncher();
    };
}

using namespace AnyFSE::App::AppControl::StateLoop;