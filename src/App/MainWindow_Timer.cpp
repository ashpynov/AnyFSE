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


#include "MainWindow.hpp"
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "App/Launchers.hpp"

namespace AnyFSE::App::Window
{
    static Logger log = LogManager::GetLogger("Window/Timer");

    void MainWindow::OnTimer(UINT_PTR timerId)
    {
        if (m_closing)
        {
            log.Trace("Exit from Timer on m_closing");
            return;
        }

        if (timerId == m_launcherTimeoutTimerId)
        {
            OnLauncherTimeoutTimer();
        }
        else if (timerId == m_launcherCheckTimerId)
        {
            OnLauncherCheckTimer();
        }
    }

    void MainWindow::OnLauncherTimeoutTimer()
    {
        log.Trace("Destroy Window by launcherTimeout");

        m_closing = true;
        DestroyWindow(m_hWnd);

        if (Launchers::IsLauncherActiveOrMinimized())
        {
            log.Trace("Notify started by Timeout timer");
            Config::Launcher.OnStarted.Notify();
            Launchers::FocusLauncher();
        }
    }


    void MainWindow::OnLauncherCheckTimer()
    {
        const bool isActive = Launchers::IsLauncherActiveOrMinimized();
        const bool becameActive = isActive && !m_bLauncherWasActive;
        const bool becameInactive = m_bLauncherWasActive && !isActive;

        if (becameInactive && Launchers::HasLauncherProcess())
        {
            log.Trace("Launcher window disappeared after detection, but launcher process still exists; keep waiting");
            return;
        }

        if (!isActive && !m_bLauncherWasActive)
        {
            log.Trace("No launcher active");
            return;
        }

        m_bLauncherWasActive = true;

        if (!m_launcherStartedTime)
        {
            m_launcherStartedTime = GetTickCount64();
        }

        if (isActive && Config::SplashShowVideo && Config::SplashTillEnd && m_videoPlayer.ShouldWaitForEnd())
        {
            log.Trace("Waiting Splash Video to complete");
            return;
        }

        KillTimer(m_hWnd, m_launcherCheckTimerId);

        UINT_PTR now = GetTickCount64();

        if (Config::SplashDelayHide && now < m_launcherStartedTime + HIDE_DELAY_MS)
        {
            log.Trace("Set Timeout timer to DestroyWindow");
            KillTimer(m_hWnd, m_launcherTimeoutTimerId);
            SetTimer(m_hWnd, m_launcherTimeoutTimerId, HIDE_DELAY_MS - (int)(now - m_launcherStartedTime) + 1, NULL);

            return;
        }

        m_closing = true;
        log.Trace("Destroy Splash Window now by launcherCheckTimer");
        DestroyWindow(m_hWnd);

        if (becameActive)
        {
            log.Trace("Notify Launcher is started");
            Config::Launcher.OnStarted.Notify();
        }

        Launchers::FocusLauncher();

        return;
    }
}
