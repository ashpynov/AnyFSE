#pragma once
#include "ManagerCycle.hpp"
#include "ManagerEvents.hpp"

namespace AnyFSE::Window { class MainWindow; }

namespace AnyFSE::Manager::State
{

    class ManagerState: public Cycle::ManagerCycle
    {
        private:
            MainWindow &m_splash;
            std::uint64_t m_preventTimer;
            std::uint64_t m_waitLauncherTimer;
            LONGLONG m_xboxAge;
            LONGLONG m_deviceFormAge;

            DWORD launcherPid;
            DWORD m_waitStartTime;


        public:
            ManagerState(MainWindow &splash);
            ~ManagerState();

        private:
            virtual void ProcessEvent(StateEvent event);
            void OnStart();
            void OnXboxDetected();
            void OnGameModeEnter();
            void OnGameModeExit();
            void OnDeviceForm();
            void OnOpenHome();
            void OnLauncherTimer();
            void OnPreventTimer();

            bool IsInFSEMode();
            bool IsLauncherActive();
            bool IsLauncherStarted();
            bool IsLauncherProcess();
            bool IsSplashActive();
            bool IsPreventIsActive();
            bool IsYoungXbox();
            bool IsOnTaskSwitcher();

            void ShowSplash();
            void KillXbox();
            void StartLauncher();
            void WaitLauncher();
            void CloseSplash();
            void PreventTimeout();
            void FocusLauncher();
    };
}

using namespace AnyFSE::Manager::State;