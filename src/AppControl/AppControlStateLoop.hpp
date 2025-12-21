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
            std::uint64_t m_restartDelayTimer;

            bool m_activePreventXbox;

            LONGLONG m_xboxAge;
            LONGLONG m_deviceFormAge;
            LONGLONG m_homeAge;

            DWORD launcherPid;
            LONGLONG m_waitStartTime;
            bool  m_isRestart;


        public:
            AppControlStateLoop(Window::MainWindow &splash);
            ~AppControlStateLoop();

        private:
            virtual void ProcessEvent(AppEvents event);
            void OnStart();
            void OnStartApps();
            void OnXboxDetected();
            void OnLauncherStopped();
            void OnGameModeEnter();
            void OnGameModeExit();
            void OnDeviceForm();
            void OnQueryEndSession();
            void OnEndSession();
            void OnDisconnect();
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
            bool IsOnGamebar();
            bool IsHomeLaunch();
            bool IsXboxActive();
            bool IsOnSettings();

            void ShowSplash();
            void StartSplash();
            void KillXbox();
            void ExitFSEMode();
            void StartLauncher();
            void RestartLauncher();
            void WaitLauncher();
            void CloseSplash();
            void PreventTimeout();
            void FocusLauncher();

            HWND GetLauncherWindow();
    };
}

using namespace AnyFSE::App::AppControl::StateLoop;