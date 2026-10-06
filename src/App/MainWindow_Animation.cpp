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
#include <cmath>
#include <windows.h>
#include <dwmapi.h>
#include "resource.h"
#include "MainWindow.hpp"
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Icon.hpp"
#include "Tools/GdiPlus.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/Localization.hpp"
#include "App/Launchers.hpp"

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "dwmapi.lib")

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

        m_pLogoImage = Icon::LoadBitmapFromIcon(Config::Launcher.IconFile);

        return (m_pLogoImage && m_pLogoImage->GetLastStatus() == Gdiplus::Status::Ok);
    }

    RECT MainWindow::LogoRect()
    {
        // Fixed integer box of the largest logo (+2 px filter margin), centered with the client's width parity:
        // its edges never move, the breathing scale happens inside it with sub-pixel precision
        RECT client;
        GetClientRect(m_hWnd, &client);
        LONG size = (LONG)ceilf(m_theme.DpiScaleF(LOGO_SIZE * (1.0f + ZOOM_DELTA))) + 2;
        size += (client.right - size) & 1;
        LONG x = (client.right - size) / 2, y = (client.bottom - size) / 2;
        return RECT{x, y, x + size, y + size};
    }

    void MainWindow::DrawClient(HDC hdc, const RECT& clientRect)
    {
        using namespace Gdiplus;
        Graphics graphics(hdc);

        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        // Prefiltered bicubic + pixel-center sampling: smooth logo edges at any zoom and sub-pixel position
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);
        // Fill background
        Color backgroundColor;
        backgroundColor.SetFromCOLORREF(m_theme.GetColorRef(FluentDesign::Theme::Dialog));
        SolidBrush backgroundBrush(backgroundColor);

        RectF rect = ToRectF(clientRect);
        graphics.FillRectangle(&backgroundBrush, 0.0f, 0.0f, rect.Width, rect.Height);

        if (m_empty)
            return;

        if (Config::SplashShowLogo && m_pLogoImage && m_pLogoImage->GetLastStatus() == Gdiplus::Ok)
        {
            if (m_animating)
            {
                // Sampled at paint time, so every presented frame shows the exact phase of the breath:
                // faster inhale, slower exhale, eased to rest at both turning points
                float t = fmodf(std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - m_animationStart).count(),
                    ZOOM_GROW_MS + ZOOM_SHRINK_MS);
                float phase = t < ZOOM_GROW_MS ? t / ZOOM_GROW_MS : 1.0f - (t - ZOOM_GROW_MS) / ZOOM_SHRINK_MS;
                m_currentZoom = 1.0f - ZOOM_DELTA * cosf(phase * 3.14159265f);
            }

            // GDI+ rounds destination rects but not source rects: keep the destination fixed and enlarge the source
            // symmetrically around the image center, so the logo scales continuously instead of in whole-pixel steps
            RectF box = ToRectF(LogoRect());
            REAL imageWidth = (REAL)m_pLogoImage->GetWidth(), imageHeight = (REAL)m_pLogoImage->GetHeight();
            REAL scale = box.Width / m_theme.DpiScaleF(LOGO_SIZE * m_currentZoom);

            // Transparent outside the image, so the enlarged source rect pads with nothing
            ImageAttributes attributes;
            attributes.SetWrapMode(WrapModeClamp, Color(0, 0, 0, 0));

            graphics.DrawImage(m_pLogoImage, box, imageWidth * (1 - scale) / 2, imageHeight * (1 - scale) / 2,
                imageWidth * scale, imageHeight * scale, UnitPixel, &attributes);
        }

        if (Config::SplashShowText)
        {
            // Display text
            Font font(L"Segoe UI", m_theme.DpiScaleF(14));
            SolidBrush textBrush(m_theme.GetColor(FluentDesign::Theme::Text));
            std::wstring name = Config::SplashCustomText.empty()
                ? TranslateF(L"splashIsLaunchingFmt", Config::Launcher.Name.c_str())
                : Config::SplashCustomText;

            // Use StringFormat for precise centering
            StringFormat format;
            format.SetAlignment(Gdiplus::StringAlignmentCenter);
            format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

            Gdiplus::RectF textArea = rect;
            textArea.Y = textArea.Height * 0.8f;
            textArea.Height = m_theme.DpiScaleF(50);
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
        StopAnimation();
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
        if (Config::SplashShowAnimation && Config::SplashShowLogo && !m_animating && m_pLogoImage)
        {
            m_animationStart = std::chrono::steady_clock::now();
            m_animating = true;
            // Frames paced by DWM composition (the display refresh) instead of the coarse, irregular WM_TIMER tick
            m_animationThread = std::thread([this]
            {
                while (m_animating)
                {
                    auto flushStart = std::chrono::steady_clock::now();
                    if (FAILED(DwmFlush()) || std::chrono::steady_clock::now() - flushStart < std::chrono::milliseconds(1))
                    {
                        Sleep(ZOOM_INTERVAL_MS); // no composition to wait on: do not spin
                    }
                    if (m_animating)
                        PostMessage(m_hWnd, WM_ANIMATION_FRAME, 0, 0);
                }
            });
        }
        return TRUE;
    }

    BOOL MainWindow::StopAnimation()
    {
        if (m_animationThread.joinable())
        {
            m_animating = false;
            m_animationThread.join();
        }
        return TRUE;
    }
}
