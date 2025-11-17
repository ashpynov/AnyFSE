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
#include "Tools/Tools.hpp"
#include "Tools/GdiPlus.hpp"
#include "Tools/DoubleBufferedPaint.hpp"

#pragma comment(lib, "Gdiplus.lib")

namespace AnyFSE::App::AppControl::Window
{
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
        if (!Config::SplashShowLogo)
        {
            return FALSE;
        }

        if (HICON hIcon = Tools::LoadIcon(Config::Launcher.IconFile))
        {
            m_pLogoImage = new Gdiplus::Bitmap(hIcon);
            DestroyIcon(hIcon);
        }

        return (m_pLogoImage && m_pLogoImage->GetLastStatus() == Gdiplus::Status::Ok);
    }


    void MainWindow::OnPaintAnimated()
    {
        using namespace Gdiplus;
        FluentDesign::DoubleBuferedPaint paint(m_hWnd);
        Graphics graphics(paint.MemDC());

        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        // Fill background
        Color backgroundColor;
        backgroundColor.SetFromCOLORREF(THEME_BACKGROUND_COLOR);
        SolidBrush backgroundBrush(THEME_BACKGROUND_COLOR);

        RectF rect = ToRectF(paint.ClientRect());
        graphics.FillRectangle(&backgroundBrush, 0.0f, 0.0f, rect.Width, rect.Height);

        UINT dpi = GetDpiForWindow(m_hWnd);
        
        if (Config::SplashShowLogo && m_pLogoImage && m_pLogoImage->GetLastStatus() == Gdiplus::Ok)
        {
            REAL imageWidth = MulDiv(150 /*pLogoImage->GetWidth()*/ * m_currentZoom, dpi, 96);
            REAL imageHeight = MulDiv(150 /*pLogoImage->GetHeight()*/ * m_currentZoom, dpi, 96);
            REAL xPos = (rect.Width - imageWidth) / 2;
            REAL yPos = (rect.Height - imageHeight) / 2;

            // Draw centered image
            graphics.DrawImage(m_pLogoImage, xPos, yPos, imageWidth, imageHeight);
        }

        if (Config::SplashShowText)
        {
            // Display text
            Font font(L"Segoe UI", MulDiv(16, dpi, 96));
            SolidBrush textBrush(Gdiplus::Color::White);
            std::wstring name = Config::SplashCustomText.empty()
                ? std::wstring(L"Launching ") + Config::Launcher.Name
                : Config::SplashCustomText;

            // Use StringFormat for precise centering
            StringFormat format;
            format.SetAlignment(Gdiplus::StringAlignmentCenter);
            format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

            Gdiplus::RectF textArea = rect;
            textArea.Y = textArea.Height * 0.8f;
            textArea.Height = MulDiv(40, dpi, 96);
            graphics.DrawString(name.c_str(), -1, &font, textArea, &format, &textBrush);
        }
    }

    void MainWindow::OnTimer(UINT_PTR timerId)
    {
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
        if (Config::SplashShowAnimation && Config::SplashShowLogo && !m_hTimer && m_pLogoImage)
        {
            m_hTimer = SetTimer(m_hWnd, m_animationTimerId, ZOOM_INTERVAL_MS, NULL);
        }
        return TRUE;
    }
    BOOL MainWindow::StopAnimation()
    {
        if (m_hTimer)
        {
            KillTimer(m_hWnd, m_animationTimerId);
            m_hTimer = NULL;
        }
        return TRUE;
    }
}