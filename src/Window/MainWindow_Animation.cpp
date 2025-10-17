
#include <tchar.h>
#include <windows.h>
#include "resource.h"
#include "MainWindow.hpp"
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Settings/FluentDesign/GdiPlus.hpp"
#include "Settings/FluentDesign/DoubleBufferedPaint.hpp"


namespace AnyFSE::Window
{
    bool MainWindow::InitAnimationResources()
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        LoadLogoImage();
        return TRUE;
    }

    BOOL MainWindow::LoadLogoImage()
    {
        if (!Config::ShowLogo)
        {
            return FALSE;
        }

        pLogoImage = Gdiplus::LoadImageFromResource(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_LOGO), L"PNG");
        return (pLogoImage && pLogoImage->GetLastStatus() == Gdiplus::Status::Ok);
    }


    void MainWindow::OnPaintAnimated()
    {
        using namespace Gdiplus;
        FluentDesign::DoubleBuferedPaint paint(hWnd);
        Graphics graphics(paint.MemDC());

        // Fill background
        Color backgroundColor;
        backgroundColor.SetFromCOLORREF(themeBackgroundColor);
        SolidBrush backgroundBrush(themeBackgroundColor);

        RectF rect = ToRectF(paint.ClientRect());
        graphics.FillRectangle(&backgroundBrush, 0.0f, 0.0f, rect.Width, rect.Height);

        if (Config::ShowLogo && pLogoImage && pLogoImage->GetLastStatus() == Gdiplus::Ok)
        {
            REAL imageWidth = pLogoImage->GetWidth() * currentZoom;
            REAL imageHeight = pLogoImage->GetHeight() * currentZoom;
            REAL xPos = (rect.Width - imageWidth) / 2;
            REAL yPos = (rect.Height - imageHeight) / 2;

            // Draw centered image
            graphics.DrawImage(pLogoImage, xPos, yPos, imageWidth, imageHeight);
        }

        if (Config::ShowText)
        {
            // Display text
            Font font(L"Arial", 16);
            SolidBrush textBrush(Gdiplus::Color::White);
            std::wstring name = wstring(L"Launching ") + Config::LauncherName;

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
        if (timerId == animationTimerId)
        {
            currentZoom += (zoomStep > 0 ? zoomStep * 5 : zoomStep);
            if (currentZoom > 1 + zoomDelta && zoomStep > 0
                || (currentZoom < 1 - zoomDelta && zoomStep < 0))
            {
                zoomStep = -zoomStep;
            }

            // Force repaint
            InvalidateRect(hWnd, NULL, FALSE);
        }
    }

    BOOL MainWindow::FreeAnimationResources()
    {
        if (pLogoImage)
        {
            delete pLogoImage;
            pLogoImage = NULL;
        }
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return TRUE;
    }


    BOOL MainWindow::StartAnimation()
    {
        if (Config::ShowAnimation && Config::ShowLogo && !hTimer && pLogoImage)
        {
            hTimer = SetTimer(hWnd, animationTimerId, ZOOM_INTERVAL_MS, NULL);
        }
        return TRUE;
    }
    BOOL MainWindow::StopAnimation()
    {
        if (hTimer)
        {
            KillTimer(hWnd, animationTimerId);
            hTimer = NULL;
        }
        return TRUE;
    }
}