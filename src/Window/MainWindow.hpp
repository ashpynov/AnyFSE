#pragma once
#include <windows.h>
#include <string>

namespace Gdiplus { class Image; }

namespace AnyFSE::Window
{
    class MainWindow
    {
    public:
        static const UINT WM_TRAY = WM_USER + 1;
    private:
        const UINT WM_TASKBARCREATED;
        HICON hIcon = NULL;
        HWND hWnd;
        ATOM aClass;

        static WNDCLASS stWC;
        static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static void LoadStringSafe(UINT nStrID, LPTSTR szBuf, UINT nBufLen);
        void OnCreate();
        void CreateTrayIcon();
        void OnPaint();

        void OnTray(LPARAM message);
        BOOL OnCommand(WORD command);
        void OnDestroy();

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

        UINT_PTR animationTimerId = 1;
        UINT_PTR hTimer = NULL;

        int ZOOM_INTERVAL_MS = 20;

        float currentZoom = 0.96f;
        float zoomStep = 0.002f;
        float zoomDelta = 0.06f;

        ULONG_PTR gdiplusToken;

        Gdiplus::Image * pLogoImage;
        COLORREF themeBackgroundColor=RGB(22,22,22);


        bool InitAnimationResources();
        BOOL LoadLogoImage();
        void OnPaintAnimated();
        void OnTimer(UINT_PTR timerId);
        BOOL FreeAnimationResources();
        BOOL StartAnimation();
        BOOL StopAnimation();
    };
}

using namespace AnyFSE::Window;