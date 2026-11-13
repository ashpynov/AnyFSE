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

        UpdateSplashActivation();

        if (Config::SplashShowVideo && m_videoPlayer.GetPlayCount() > 0)
            StopAnimation();

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
        if (ShouldKeepSplashActive())
        {
            log.Trace("Waiting for the first video playback before launcher timeout");
            return;
        }

        if (Config::SplashShowVideo && Config::SplashTillEnd && m_launcherDetected
            && !Launchers::IsLauncherActiveOrMinimized() && Launchers::HasLauncherProcess())
        {
            log.Trace("Launcher window is absent, but its process is still running; keep waiting");
            return;
        }

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
        const bool playAtLeastOnce = Config::SplashShowVideo && Config::SplashTillEnd;
        const bool isActive = Launchers::IsLauncherActiveOrMinimized();
        const bool becameActive = isActive && !m_bLauncherWasActive;
        const bool becameInactive = m_bLauncherWasActive && !isActive;

        // Preserve detection across a disappearing window only in play-at-least-once mode.
        if (isActive && playAtLeastOnce)
            m_launcherDetected = true;

        if (becameInactive && Launchers::HasLauncherProcess())
        {
            // While completing the video, keep the window history until the process also disappears.
            if (playAtLeastOnce)
            {
                log.Trace("Launcher window disappeared, but its process is still running; keep waiting");
                return;
            }

            // Outside that mode, only restart-capable presets may wait for another window.
            if (Config::Launcher.OnRestating.HasSubscribe())
            {
                log.Trace("Launcher window disappeared; waiting for a supported restart");
                m_bLauncherWasActive = false;
                return;
            }
        }

        if (!isActive && !m_bLauncherWasActive)
        {
            log.Trace("No launcher active");
            return;
        }

        m_bLauncherWasActive = true;

        if (isActive && playAtLeastOnce && m_videoPlayer.ShouldWaitForEnd())
        {
            log.Trace("Waiting Splash Video to complete");
            return;
        }

        KillTimer(m_hWnd, m_launcherCheckTimerId);

        m_closing = true;
        log.Trace("Destroy Splash Window now by launcherCheckTimer");
        DestroyWindow(m_hWnd);

        // Activating a protocol launcher after its window closed could start it again.
        if (!isActive)
            return;

        if (becameActive)
        {
            log.Trace("Notify Launcher is started");
            Config::Launcher.OnStarted.Notify();
        }

        Launchers::FocusLauncher();

        return;
    }
}
