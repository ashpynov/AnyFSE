#pragma once
#include <Windows.h>
#include <string>
#include <gdiplus.h>

namespace AnyFSE::Window
{
    class MainWindow
    {
    public:
        static const UINT WM_TRAY = WM_USER + 1;
    private:
        HICON hIcon = NULL;
        HWND hWnd;
        HINSTANCE hInstance;        
        ATOM aClass;           

        UINT_PTR zoomTimerId = 1;
        int ZOOM_INTERVAL_MS = 20;
        
        float currentZoom = 1.0f;
        float zoomStep = 0.005f;
        float zoomDelta = 0.1f;
        UINT_PTR hTimer = NULL;

        static WNDCLASS stWC;
        static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);   
        static void LoadStringSafe(UINT nStrID, LPTSTR szBuf, UINT nBufLen);
        void OnCreate();
        void OnPaint();
        void OnTimer(UINT_PTR timerId);
        void OnTray(LPARAM message);
        BOOL OnCommand(WORD command);
        void OnDestroy();
        BOOL StartAnimation();
        BOOL StopAnimation();
        LRESULT CALLBACK HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

        HBRUSH hThemeBackgroundBrush;
        Gdiplus::Color themeBackgroundColor;
        void LoadTheme(LPCWSTR className);
        void FreeTheme();

        // Global variables
        Gdiplus::Image* pLogoImage = nullptr;

        // Initialize GDI+
        ULONG_PTR gdiplusToken;

        // Function to load PNG image
        BOOL LoadLogoImage(const std::wstring& filename);

    public:
        MainWindow();
        ~MainWindow();
        bool Create(LPCWSTR className, HINSTANCE hInstance, LPCTSTR windowName);
        static int RunLoop();

        bool Show(int mode = SW_SHOW);
        bool Hide();
    };
 
}

using namespace AnyFSE::Window;