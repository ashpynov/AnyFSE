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


#include <tchar.h>
#include <windows.h>
#include "resource.h"
#include "MainWindow.hpp"
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Icon.hpp"
#include "Tools/GdiPlus.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "App/Launchers.hpp"

#pragma comment(lib, "Gdiplus.lib")

namespace AnyFSE::App::Window
{
    static Logger log = LogManager::GetLogger("Window/Animation");

    bool MainWindow::InitAnimationResources()
    {
        m_pLogoImage = nullptr;
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
        LoadLogoImage();
        return TRUE;
    }

    BOOL MainWindow::LoadLogoImage()
    {
        if (m_pLogoImage)
        {
            delete m_pLogoImage;
            m_pLogoImage = NULL;
        }

        if (!Config::SplashShowLogo)
        {
            return FALSE;
        }

        if (HICON hIcon = Icon::LoadIcon(Config::Launcher.IconFile))
        {
            m_pLogoImage = new Gdiplus::Bitmap(hIcon);
            DestroyIcon(hIcon);
        }

        return (m_pLogoImage && m_pLogoImage->GetLastStatus() == Gdiplus::Status::Ok);
    }

    void MainWindow::DrawClient(HDC hdc, const RECT& clientRect)
    {
        using namespace Gdiplus;
        Graphics graphics(hdc);

        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        // Fill background
        Color backgroundColor;
        backgroundColor.SetFromCOLORREF(m_theme.GetColorRef(FluentDesign::Theme::Dialog));
        SolidBrush backgroundBrush(backgroundColor);

        RectF rect = ToRectF(clientRect);
        graphics.FillRectangle(&backgroundBrush, 0.0f, 0.0f, rect.Width, rect.Height);

        if (m_empty)
            return;

        float dpi = (float)GetDpiForWindow(m_hWnd);

        if (Config::SplashShowLogo && m_pLogoImage && m_pLogoImage->GetLastStatus() == Gdiplus::Ok)
        {
            REAL imageWidth = 150 /*pLogoImage->GetWidth()*/ * m_currentZoom * dpi / 96;
            REAL imageHeight = 150 /*pLogoImage->GetHeight()*/ * m_currentZoom * dpi / 96;
            REAL xPos = (rect.Width - imageWidth) / 2;
            REAL yPos = (rect.Height - imageHeight) / 2;

            // Draw centered image
            graphics.DrawImage(m_pLogoImage, xPos, yPos, imageWidth, imageHeight);
        }

        if (Config::SplashShowText)
        {
            // Display text
            Font font(L"Segoe UI", 14 * dpi / 96);
            SolidBrush textBrush(m_theme.GetColor(FluentDesign::Theme::Text));
            std::wstring name = Config::SplashCustomText.empty()
                ? std::wstring(L"Launching ") + Config::Launcher.Name
                : Config::SplashCustomText;

            // Use StringFormat for precise centering
            StringFormat format;
            format.SetAlignment(Gdiplus::StringAlignmentCenter);
            format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

            Gdiplus::RectF textArea = rect;
            textArea.Y = textArea.Height * 0.8f;
            textArea.Height = 50 * dpi / 96;
            graphics.DrawString(name.c_str(), -1, &font, textArea, &format, &textBrush);
        }
    }

    void MainWindow::OnTimer(UINT_PTR timerId)
    {
        if (m_closing)
        {
            log.Trace("Exit from Timer on m_closing");
            return;
        }

        if (timerId == m_launcherTimeoutTimerId)
        {
            log.Trace("Destroy Window by launcherTimeout");

            m_closing = true;
            DestroyWindow(m_hWnd);

            if (!m_bLauncherWasActive && Launchers::IsLauncherActiveOrMinimized())
            {
                log.Trace("Notify started");
                Config::Launcher.OnStarted.Notify();
            }
            Launchers::FocusLauncher();
            return;
        }
        if (timerId == m_animationTimerId)
        {
            m_currentZoom += (m_zoomStep > 0 ? m_zoomStep * 5 : m_zoomStep);
            if (m_currentZoom > 1 + m_zoomDelta && m_zoomStep > 0
                || (m_currentZoom < 1 - m_zoomDelta && m_zoomStep < 0))
            {
                m_zoomStep = -m_zoomStep;
            }

            // Force repaint
            InvalidateRect(m_hWnd, NULL, FALSE);
        }
        if (timerId == m_launcherCheckTimerId)
        {
            bool isActive = Launchers::IsLauncherActiveOrMinimized();
            if (isActive || m_bLauncherWasActive)
            {
                const bool notifyStarted = !m_bLauncherWasActive && isActive;
                m_bLauncherWasActive = true;
                if (!m_launcherStartedTime)
                {
                    m_launcherStartedTime = GetTickCount64();
                }
                if (!isActive || !Config::SplashShowVideo || !Config::SplashTillEnd || !m_videoPlayer.ShouldWaitForEnd())
                {
                    const bool focusLauncher = isActive && Launchers::IsLauncherMinimized();
                    KillTimer(m_hWnd, m_launcherCheckTimerId);
                    m_hLauncherCheckTimer = NULL;

                    UINT_PTR now = GetTickCount64();
                    if (!Config::SplashDelayHide || now > m_launcherStartedTime + HIDE_DELAY_MS )
                    {
                        m_closing = true;
                        log.Trace("Destroy Window by launcherCheckTimer");
                        DestroyWindow(m_hWnd);

                        if (notifyStarted)
                        {
                            log.Trace("Notify started");
                            Config::Launcher.OnStarted.Notify();
                        }
                        Launchers::FocusLauncher();
                    }
                    else
                    {
                        log.Trace("Set Timeout timer to DestroyWindow");
                        KillTimer(m_hWnd, m_launcherTimeoutTimerId);
                        SetTimer(m_hWnd, m_launcherTimeoutTimerId, HIDE_DELAY_MS - (int)(now - m_launcherStartedTime) + 1, NULL);
                        return;
                    }

                    return;
                }
            }
        }
    }

    BOOL MainWindow::FreeAnimationResources()
    {
        if (m_pLogoImage)
        {
            delete m_pLogoImage;
            m_pLogoImage = NULL;
        }
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
        return TRUE;
    }

    BOOL MainWindow::StartAnimation()
    {
        if (Config::SplashShowAnimation && Config::SplashShowLogo && !m_hAnimationTimer && m_pLogoImage)
        {
            m_hAnimationTimer = SetTimer(m_hWnd, m_animationTimerId, ZOOM_INTERVAL_MS, NULL);
        }
        return TRUE;
    }

    BOOL MainWindow::StopAnimation()
    {
        if (m_hAnimationTimer)
        {
            KillTimer(m_hWnd, m_animationTimerId);
            m_hAnimationTimer = NULL;
        }
        return TRUE;
    }
}
