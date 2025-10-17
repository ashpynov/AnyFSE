
#include <tchar.h>
#include <windows.h>
#include "resource.h"
#include "MainWindow.hpp"
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Tools.hpp"
#include "Tools/Unicode.hpp"

#include "Settings/SettingsDialog.hpp"

namespace AnyFSE::Window
{
    static Logger log = LogManager::GetLogger("MainWindow");
    WNDCLASS MainWindow::stWC;

    MainWindow::MainWindow()
        : hWnd(NULL)
        , aClass(NULL)
        , WM_TASKBARCREATED(RegisterWindowMessage(L"TaskbarCreated"))
    {

        ZeroMemory(&stWC, sizeof(stWC));
    }
    MainWindow::~MainWindow()
    {
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

    bool MainWindow::Create(LPCWSTR className, HINSTANCE hInstance, LPCTSTR windowName)
    {


        stWC.lpszClassName = className;
        stWC.hInstance = hInstance;
        stWC.lpfnWndProc = MainWndProc;
        stWC.hIcon = Tools::LoadIcon(Config::LauncherIcon);
        //stWC.hbrBackground = NULL_BRUSH;
        stWC.style = CS_HREDRAW | CS_VREDRAW;

        aClass = RegisterClass(&stWC);
        if (!aClass)
        {
            log.Info(Logger::APIError(), "Can not register class name");
            return false;
        }

        hWnd = CreateWindow((LPCTSTR)aClass, windowName, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, this);

        if (!IsWindow(hWnd))
        {
            log.Info(Logger::APIError(), "Can not create window");
            return false;
        }

        log.Info("Window is created (hWnd=%08x)", hWnd);
        return true;
    };

    bool MainWindow::Show()
    {
        return IsWindow(hWnd) && StartAnimation() && AnimateWindow(hWnd, 200, AW_BLEND) ;
    }

    bool MainWindow::Hide()
    {
        return !IsWindow(hWnd) || StopAnimation() && AnimateWindow(hWnd, 200, AW_BLEND|AW_HIDE);
    }

    bool MainWindow::IsVisible()
    {
        return IsWindowVisible(hWnd);
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
        if (uMsg == WM_TASKBARCREATED)
        {
            log.Info("Explorer start taskbar ready");
            CreateTrayIcon();
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
        switch (uMsg)
        {
        case WM_CREATE:
            OnCreate();
            return 0;
        case WM_PAINT:
            OnPaint();
            break;
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
        CreateTrayIcon();
        InitAnimationResources();
    }

    void MainWindow::CreateTrayIcon()
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
        OnPaintAnimated();
    }

    void MainWindow::OnTray(LPARAM message)
    {
        switch (message)
        {
        case WM_LBUTTONDBLCLK:
            SendMessage(hWnd, WM_COMMAND, ID_CONFIGURE, 0);
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
        case ID_CONFIGURE:
            {
                SettingsDialog dialog;
                INT_PTR result = dialog.Show(stWC.hInstance);
            }

            return FALSE;
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
        FreeAnimationResources();

        PostQuitMessage(0);
    }
}