// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include <tchar.h>
#include <windows.h>
#include "resource.h"
#include "MainWindow.hpp"
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Tools.hpp"
#include "Tools/GdiPlus.hpp"
#include "Tools/DoubleBufferedPaint.hpp"


namespace AnyFSE::App::AppControl::Window
{
    bool MainWindow::InitAnimationResources()
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
        LoadLogoImage();
        return TRUE;
    }

    BOOL MainWindow::LoadLogoImage()
    {
        if (!Config::ShowLogo)
        {
            return FALSE;
        }

        if (HICON hIcon = Tools::LoadIcon(Config::Launcher.IconFile))
        {
            m_pLogoImage = new Gdiplus::Bitmap(hIcon);
            DestroyIcon(hIcon);
        }

        if (!m_pLogoImage || m_pLogoImage->GetLastStatus() != Gdiplus::Status::Ok)
        {
            m_pLogoImage = Gdiplus::LoadImageFromResource(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_LOGO), L"PNG");
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

        if (Config::ShowLogo && m_pLogoImage && m_pLogoImage->GetLastStatus() == Gdiplus::Ok)
        {
            REAL imageWidth = 150 /*pLogoImage->GetWidth()*/ * m_currentZoom;
            REAL imageHeight = 150 /*pLogoImage->GetHeight()*/ * m_currentZoom;
            REAL xPos = (rect.Width - imageWidth) / 2;
            REAL yPos = (rect.Height - imageHeight) / 2;

            // Draw centered image
            graphics.DrawImage(m_pLogoImage, xPos, yPos, imageWidth, imageHeight);
        }

        if (Config::ShowText)
        {
            // Display text
            Font font(L"Arial", 16);
            SolidBrush textBrush(Gdiplus::Color::White);
            std::wstring name = wstring(L"Launching ") + Config::Launcher.Name;

            // Use StringFormat for precise centering
            StringFormat format;
            format.SetAlignment(Gdiplus::StringAlignmentCenter);
            format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

            Gdiplus::RectF textArea = rect;
            textArea.Y = textArea.Height * 0.8f;
            textArea.Height = 40;
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
        if (Config::ShowAnimation && Config::ShowLogo && !m_hTimer && m_pLogoImage)
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