// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#pragma once

#include <windows.h>
#include <string>
#include "VideoPlayer.hpp"

namespace Gdiplus { class Image; }

namespace AnyFSE::App::AppControl::Window
{
    class MainWindow
    {
    public:
        static const UINT WM_TRAY = WM_USER + 1;
    private:
        const UINT WM_TASKBARCREATED;

        HICON m_hIcon = NULL;
        HWND m_hWnd;
        ATOM m_aClass;
        SimpleVideoPlayer m_videoPlayer;
        std::wstring m_currentVideo;

        int m_result = ERROR_RESTART_APPLICATION;

        static WNDCLASS WC;
        static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static void LoadStringSafe(UINT nStrID, LPTSTR szBuf, UINT nBufLen);
        void OnCreate();
        void CreateTrayIcon();
        void OnPaint();

        void OnTray(LPARAM message);
        BOOL FreeResources();
        BOOL OnCommand(WORD command);
        void OnDestroy();
        void PreloadNextVideo();

        LRESULT CALLBACK HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    public:
        MainWindow();
        ~MainWindow();
        bool Show();
        bool Hide();
        bool Create(LPCWSTR className, HINSTANCE hInstance, LPCTSTR windowName);
        static int RunLoop();

        bool IsVisible();

    private: // Animation

        UINT_PTR m_animationTimerId = 1;
        UINT_PTR m_hTimer = NULL;

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