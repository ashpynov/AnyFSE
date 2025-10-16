#pragma once
#include <windows.h>
#include <string>

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
        bool Create(LPCWSTR className, HINSTANCE hInstance, LPCTSTR windowName);
        static int RunLoop();

        bool Show();
        bool Hide();
        bool IsVisible();
    };
}

using namespace AnyFSE::Window;