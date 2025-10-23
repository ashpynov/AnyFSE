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
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Tools.hpp"
#include "Tools/Unicode.hpp"
#include "AppControl/AppControl.hpp"
#include "AppControl/MainWindow.hpp"


namespace AnyFSE::App::AppControl::Window
{
    static Logger log = LogManager::GetLogger("MainWindow");
    WNDCLASS MainWindow::WC;

    MainWindow::MainWindow()
        : m_hWnd(NULL)
        , m_aClass(NULL)
        , WM_TASKBARCREATED(RegisterWindowMessage(L"TaskbarCreated"))
    {

        ZeroMemory(&WC, sizeof(WC));
    }
    MainWindow::~MainWindow()
    {
        if (WC.hIcon)
        {
            DestroyIcon(WC.hIcon);
        }

        if (IsWindow(m_hWnd))
        {

            DestroyWindow(m_hWnd);
            m_hWnd = NULL;
        }
        if (m_aClass)
        {
            UnregisterClass((LPCTSTR)m_aClass, WC.hInstance);
        }
    }

    bool MainWindow::Create(LPCWSTR className, HINSTANCE hInstance, LPCTSTR windowName)
    {


        WC.lpszClassName = className;
        WC.hInstance = hInstance;
        WC.lpfnWndProc = MainWndProc;
        WC.hIcon = Tools::LoadIcon(Config::Launcher.IconFile, 16);
        //stWC.hbrBackground = NULL_BRUSH;
        WC.style = CS_HREDRAW | CS_VREDRAW;

        m_aClass = RegisterClass(&WC);
        if (!m_aClass)
        {
            log.Debug(Logger::APIError(), "Can not register class name");
            return false;
        }

        m_hWnd = CreateWindow((LPCTSTR)m_aClass, windowName, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, hInstance, this);

        if (!IsWindow(m_hWnd))
        {
            log.Debug(Logger::APIError(), "Can not create window");
            return false;
        }

        log.Debug("Window is created (hWnd=%08x)", m_hWnd);
        return true;
    };

    bool MainWindow::Show()
    {
        if (IsWindow(m_hWnd))
        {
            StartAnimation();
            AnimateWindow(m_hWnd, 200, AW_BLEND);
            ShowWindow(m_hWnd, SW_MAXIMIZE);
        }
        return true;
    }

    bool MainWindow::Hide()
    {
        return !IsWindow(m_hWnd) || StopAnimation() && AnimateWindow(m_hWnd, 200, AW_BLEND|AW_HIDE);
    }

    bool MainWindow::IsVisible()
    {
        return IsWindowVisible(m_hWnd);
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
            pWindow->m_hWnd = hWnd;
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
        if (uMsg == WM_TASKBARCREATED)
        {
            log.Debug("Explorer start taskbar ready");
            CreateTrayIcon();
            return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
        }
        switch (uMsg)
        {
        case WM_CREATE:
            OnCreate();
            return 0;
        case WM_PAINT:
            OnPaint();
            break;
        case WM_ERASEBKGND:
            return 1;
        case WM_DESTROY:
            OnDestroy();
            break;
        case WM_TRAY:
            OnTray(lParam);
            return 0;
        case WM_TIMER:
            OnTimer(wParam);
            return 0;
        case WM_COMMAND:
            if (OnCommand(LOWORD(wParam)))
            {
                return 0;
            }
            break;
        }
        return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    }

    // static
    void MainWindow::LoadStringSafe(UINT nStrID, LPTSTR szBuf, UINT nBufLen)
    {
        UINT nLen = LoadString(WC.hInstance, nStrID, szBuf, nBufLen);
        if (nLen >= nBufLen)
            nLen = nBufLen - 1;
        szBuf[nLen] = 0;
    }

    void MainWindow::OnCreate()
    {
        CreateTrayIcon();
        InitAnimationResources();
    }

    void MainWindow::CreateTrayIcon()
    {
        NOTIFYICONDATA stData;
        ZeroMemory(&stData, sizeof(stData));
        stData.cbSize = sizeof(stData);
        stData.hWnd = m_hWnd;
        stData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        stData.uCallbackMessage = WM_TRAY;
        stData.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
        LoadStringSafe(IDS_TIP, stData.szTip, _countof(stData.szTip));
        if (!Shell_NotifyIcon(NIM_ADD, &stData))
        {
            log.Debug(log.APIError(), "Can not create tray icon");
            return; // oops
        }
    }

    void MainWindow::OnPaint()
    {
        if ( Config::ShowAnimation || Config::ShowLogo ||  Config::ShowText)
        {
            OnPaintAnimated();
        }
        else
        {
            PAINTSTRUCT ps;
            BeginPaint(m_hWnd, &ps);
            EndPaint(m_hWnd, &ps);
        }
    }

    void MainWindow::OnTray(LPARAM message)
    {
        switch (message)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            SendMessage(m_hWnd, WM_COMMAND, ID_CONFIGURE, 0);
            break;

        case WM_RBUTTONDOWN:
        {
            HMENU hMenu = LoadMenu(WC.hInstance, MAKEINTRESOURCE(IDR_POPUP));
            if (hMenu)
            {
                HMENU hSubMenu = GetSubMenu(hMenu, 0);
                if (hSubMenu)
                {
                    POINT stPoint;
                    GetCursorPos(&stPoint);

                    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, stPoint.x, stPoint.y, 0, m_hWnd, NULL);
                }

                DestroyMenu(hMenu);
            }
        }
        break;
        }
    }

    BOOL MainWindow::FreeResources()
    {
        NOTIFYICONDATA stData;
        ZeroMemory(&stData, sizeof(stData));
        stData.cbSize = sizeof(stData);
        stData.hWnd = m_hWnd;
        Shell_NotifyIcon(NIM_DELETE, &stData);
        FreeAnimationResources();
        return TRUE;
    }

    BOOL MainWindow::OnCommand(WORD command)
    {
        switch (command)
        {
        case ID_CONFIGURE:
            {
                int result = AppControl::ShowSettings();
                if ( result == IDOK)
                {
                    FreeResources();
                    m_result = ERROR_RESTART_APPLICATION;
                    DestroyWindow(m_hWnd);
                }
                else if ( result == IDABORT)
                {
                    m_result = ERROR_PRODUCT_UNINSTALLED;
                    DestroyWindow(m_hWnd);
                }
            }

            return FALSE;
        case ID_QUIT:
            m_result = 0;
            DestroyWindow(m_hWnd);
            return TRUE;
        }
        return FALSE;
    }

    void MainWindow::OnDestroy()
    {
        FreeResources();
        PostQuitMessage(m_result);
    }
}