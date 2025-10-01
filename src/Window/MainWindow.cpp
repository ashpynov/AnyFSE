
#include <tchar.h>
#include <windows.h>
// #include <uxtheme.h>
// #include <Vsstyle.h>
// #include <vssym32.h>
#include "resource.h"
#include "MainWindow.h"
#include "Logging/LogManager.h"
#include "Configuration/Config.h"
#include "Tools/Tools.h"

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
// #pragma comment(lib, "uxtheme.lib")

namespace AnyFSE::Window
{
    static Logger log = LogManager::GetLogger("MainWindow");
    WNDCLASS MainWindow::stWC;

    MainWindow::MainWindow() : hWnd(NULL),
                               hInstance(NULL),
                               aClass(NULL)
    {
        ZeroMemory(&stWC, sizeof(stWC));
    }
    MainWindow::~MainWindow()
    {
        FreeTheme();

        if (stWC.hIcon)
        {
            DestroyIcon(stWC.hIcon);
        }

        if (IsWindow(hWnd))
        {

            DestroyWindow(hWnd);
            hWnd = NULL;
        }
        if (aClass)
        {
            UnregisterClass((LPCTSTR)aClass, stWC.hInstance);
        }
    }

    void MainWindow::LoadTheme(LPCWSTR className)
    {
        // TO DO apply mica theme or other hTheme stuff
        themeBackgroundColor.SetFromCOLORREF(Config::BackgroundColor);
        hThemeBackgroundBrush = CreateSolidBrush(Config::BackgroundColor);
    }

    void MainWindow::FreeTheme()
    {
        DeleteObject(hThemeBackgroundBrush);
    }

    BOOL MainWindow::LoadLogoImage(const std::wstring& filename)
    {
        if (!Config::ShowLogo)
        {
            return FALSE;
        }

        pLogoImage = new Gdiplus::Image(filename.c_str());
        return (pLogoImage && pLogoImage->GetLastStatus() == Gdiplus::Status::Ok);
    }

    bool MainWindow::Create(LPCWSTR className, HINSTANCE hIstance, LPCTSTR windowName)
    {
        LoadTheme(className);
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        LoadLogoImage(Config::LauncherLogoPath);

        stWC.lpszClassName = className;
        stWC.hInstance = hInstance;
        stWC.lpfnWndProc = MainWndProc;
        stWC.hIcon = Tools::LoadIcon(Config::LauncherIcon);
        stWC.hbrBackground = hThemeBackgroundBrush;
        stWC.style = CS_HREDRAW | CS_VREDRAW;

        aClass = RegisterClass(&stWC);
        if (!aClass)
        {
            log.Info(Logger::APIError(), "Can not register class name");
            return false;
        }

        hWnd = CreateWindow((LPCTSTR)aClass, windowName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, NULL, NULL, hInstance, this);

        if (!IsWindow(hWnd))
        {
            log.Info(Logger::APIError(), "Can not create window");
            return false;
        }

        log.Info("Window is created (hWnd=%08x)", hWnd);
        return true;
    };

    bool MainWindow::Show(int mode)
    {
        bool res = false;
        if (IsWindow(hWnd))
        {
            res = ShowWindow(hWnd, mode);
            UpdateWindow(hWnd);
            if (mode)
            {
                StartAnimation();
            }
        }
        return res;
    }

    bool MainWindow::Hide()
    {
        StopAnimation();
        return !IsWindow(hWnd) || Show(SW_HIDE);
    }

    // static
    int MainWindow::RunLoop()
    {
        MSG stMsg;
        while (GetMessage(&stMsg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&stMsg);
            DispatchMessage(&stMsg);
        }
        return (int)stMsg.wParam;
    }

    // static
    LRESULT CALLBACK MainWindow::MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow *pWindow = nullptr;

        if (uMsg == WM_NCCREATE)
        {
            CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
            pWindow = (MainWindow *)pCreate->lpCreateParams;
            pWindow->hWnd = hWnd;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pWindow);
        }
        else
        {
            pWindow = (MainWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        }

        if (pWindow)
        {
            return pWindow->HandleMessage(uMsg, wParam, lParam);
        }

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {

        switch (uMsg)
        {
        case WM_CREATE:
            OnCreate();
            return 0;
        case WM_PAINT:
            OnPaint();
            break;
        case WM_TIMER:
            OnTimer(wParam);
            break;
        case WM_DESTROY:
            OnDestroy();
            break;
        case WM_TRAY:
            OnTray(lParam);
            return 0;
        case WM_COMMAND:
            if (OnCommand(LOWORD(wParam)))
            {
                return 0;
            }
            break;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    // static
    void MainWindow::LoadStringSafe(UINT nStrID, LPTSTR szBuf, UINT nBufLen)
    {
        UINT nLen = LoadString(stWC.hInstance, nStrID, szBuf, nBufLen);
        if (nLen >= nBufLen)
            nLen = nBufLen - 1;
        szBuf[nLen] = 0;
    }

    void MainWindow::OnCreate()
    {
        NOTIFYICONDATA stData;
        ZeroMemory(&stData, sizeof(stData));
        stData.cbSize = sizeof(stData);
        stData.hWnd = hWnd;
        stData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        stData.uCallbackMessage = WM_TRAY;
        stData.hIcon = Tools::LoadIcon(Config::LauncherIcon);
        LoadStringSafe(IDS_TIP, stData.szTip, _countof(stData.szTip));
        if (!Shell_NotifyIcon(NIM_ADD, &stData))
        {
            log.Info(log.APIError(), "Can not create tray icon");
            return; // oops
        }
    }

    void MainWindow::OnPaint()
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int clientWidth = clientRect.right - clientRect.left;
        int clientHeight = clientRect.bottom - clientRect.top;

        // Double buffering
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, clientWidth, clientHeight);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

        Gdiplus::Graphics graphics(hdcMem);

        // Fill background
        Gdiplus::SolidBrush backgroundBrush(themeBackgroundColor);
        graphics.FillRectangle(&backgroundBrush, 0, 0, clientWidth, clientHeight);

        if (Config::ShowLogo && pLogoImage && pLogoImage->GetLastStatus() == Gdiplus::Ok)
        {
            Gdiplus::REAL imageWidth = pLogoImage->GetWidth() * currentZoom;
            Gdiplus::REAL imageHeight = pLogoImage->GetHeight() * currentZoom;
            Gdiplus::REAL xPos = (clientWidth - imageWidth) / 2;
            Gdiplus::REAL yPos = (clientHeight - imageHeight) / 2;

            // Draw centered image
            graphics.DrawImage(pLogoImage, xPos, yPos, imageWidth, imageHeight);
        }

        if (Config::ShowText)
        {
            // Display text
            Gdiplus::Font font(L"Arial", 16);
            Gdiplus::SolidBrush textBrush(Gdiplus::Color::White);
            std::wstring name = wstring(L"Launching ") + Config::LauncherName;

            // Use StringFormat for precise centering
            Gdiplus::StringFormat format;
            format.SetAlignment(Gdiplus::StringAlignmentCenter);
            format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            Gdiplus::RectF textArea(0, (Gdiplus::REAL)clientHeight * 0.8f, (Gdiplus::REAL)clientWidth, 40); // Bottom 80 pixels
            graphics.DrawString(name.c_str(), -1, &font, textArea, &format, &textBrush);
        }

        // Copy to screen
        BitBlt(hdc, 0, 0, clientWidth, clientHeight, hdcMem, 0, 0, SRCCOPY);

        // Cleanup
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(hWnd, &ps);
    }

    void MainWindow::OnTimer(UINT_PTR timerId)
    {
        if (timerId == zoomTimerId)
        {
            currentZoom += zoomStep;
            if (abs(currentZoom - 1) > zoomDelta)
            {
                zoomStep = -zoomStep;
            }

            // Force repaint
            InvalidateRect(hWnd, NULL, FALSE);
        }
    }

    void MainWindow::OnTray(LPARAM message)
    {
        switch (message)
        {
        case WM_LBUTTONDBLCLK:
            SendMessage(hWnd, WM_COMMAND, ID_SHOW, 0);
            break;

        case WM_RBUTTONDOWN:
        {
            HMENU hMenu = LoadMenu(stWC.hInstance, MAKEINTRESOURCE(IDR_POPUP));
            if (hMenu)
            {
                HMENU hSubMenu = GetSubMenu(hMenu, 0);
                if (hSubMenu)
                {
                    POINT stPoint;
                    GetCursorPos(&stPoint);

                    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, stPoint.x, stPoint.y, 0, hWnd, NULL);
                }

                DestroyMenu(hMenu);
            }
        }
        break;
        }
    }

    BOOL MainWindow::OnCommand(WORD command)
    {
        switch (command)
        {
        case ID_QUIT:
            PostQuitMessage(0);
            return TRUE;
        }
        return FALSE;
    }

    void MainWindow::OnDestroy()
    {
        NOTIFYICONDATA stData;
        ZeroMemory(&stData, sizeof(stData));
        stData.cbSize = sizeof(stData);
        stData.hWnd = hWnd;
        Shell_NotifyIcon(NIM_DELETE, &stData);

        if (pLogoImage)
        {
            delete pLogoImage;
            pLogoImage = NULL;
        }
        Gdiplus::GdiplusShutdown(gdiplusToken);
        PostQuitMessage(0);
    }

    BOOL MainWindow::StartAnimation()
    {
        if (Config::ShowAnimation && Config::ShowLogo && !hTimer && pLogoImage)
        {
            hTimer = SetTimer(hWnd, zoomTimerId, ZOOM_INTERVAL_MS, NULL);
        }
        return TRUE;
    }
    BOOL MainWindow::StopAnimation()
    {
        if (hTimer)
        {
            KillTimer(hWnd, zoomTimerId);
            hTimer = NULL;
        }
        return TRUE;
    }
}