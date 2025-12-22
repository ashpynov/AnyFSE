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


#pragma once

#include <windows.h>
#include <string>
#include "VideoPlayer.hpp"
#include "Tools/Event.hpp"

namespace Gdiplus { class Image; }

namespace AnyFSE::App::AppControl::Window
{
    class MainWindow
    {
    public:
        static const UINT WM_TRAY = WM_USER + 1;
        static const UINT WM_UPDATE_NOTIFICATION = WM_USER + 2;
        static const UINT WM_UPDATE_CHECK = WM_USER + 3;
        static const UINT WM_UPDATE_COMMAND = WM_USER + 4;
    private:
        const UINT WM_TASKBARCREATED;
        const UINT WM_UPDATER_COMMAND;

        HICON m_hIcon = NULL;
        HWND m_hWnd;
        ATOM m_aClass;
        SimpleVideoPlayer m_videoPlayer;
        std::wstring m_currentVideo;

        int m_result = ERROR_RESTART_APPLICATION;
        bool m_empty = false;
        bool m_successChecked = false;
        bool m_suspended = false;
        bool m_trayCreated = false;

        static WNDCLASS WC;
        static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static void LoadStringSafe(UINT nStrID, LPTSTR szBuf, UINT nBufLen);
        void OnCreate();

        void OnPaint();

        void OnTray(LPARAM message);
        BOOL FreeResources();
        BOOL OnCommand(WORD command);
        void OnDestroy();

        void OnUpdateNotification();
        void OnUpdateCheck();
        void ScheduleCheck(int delay = 60);
        void SelectNextVideo();

        LRESULT CALLBACK HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    public:
        MainWindow();
        ~MainWindow();
        bool Show(bool empty = false);
        bool Start();
        bool Hide();
        void Suspend(bool bSuspend);
        bool Create(LPCWSTR className, HINSTANCE hInstance, LPCTSTR windowName);
        static int RunLoop();

        bool IsVisible();
        int ExitOnError();

        void CreateTrayIcon();

        void StartUpdateCheck();
        Event OnStartWindow;
        Event OnQueryEndSession;
        Event OnEndSession;
        Event OnReconfigure;

    private: // Animation

        UINT_PTR m_animationTimerId = 1;
        UINT_PTR m_updateTimerId = 2;
        UINT_PTR m_hAnimationTimer = NULL;
        UINT_PTR m_hUpdateTimer = NULL;

        const int ZOOM_INTERVAL_MS = 20;

        float m_currentZoom = 0.96f;
        float m_zoomStep = 0.002f;
        float m_zoomDelta = 0.06f;

        ULONG_PTR m_gdiplusToken;

        Gdiplus::Image * m_pLogoImage;
        const COLORREF THEME_BACKGROUND_COLOR = RGB(22,22,22);


        bool InitAnimationResources();
        BOOL LoadLogoImage();
        void OnPaintAnimated();
        void OnTimer(UINT_PTR timerId);
        BOOL FreeAnimationResources();
        BOOL StartAnimation();
        BOOL StopAnimation();
    };
}

using namespace AnyFSE::App::AppControl::Window;