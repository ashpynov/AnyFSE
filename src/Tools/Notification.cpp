#include "Tools/Notification.hpp"
#include "Tools/Unicode.hpp"
#include "resource.h"
#include <shellapi.h>
#include "Notification.hpp"

#ifndef VER_VERSION_STR
#define VER_VERSION_STR "0.0.0"
#endif

namespace AnyFSE::Tools
{
    void Notification::Show(HWND hwnd, const std::wstring& title, const std::wstring& message, const std::wstring& /*launchUrl*/ )
    {
        bool createdWindow = false;
        HWND hWnd = hwnd;

        // If no hwnd provided, create a temporary hidden window and add an icon
        WNDCLASSW wc = {0};
        if (!hWnd)
        {
            createdWindow = true;
            wc.lpfnWndProc = DefWindowProcW;
            wc.hInstance = GetModuleHandleW(NULL);
            wc.lpszClassName = L"AnyFSE_Updater_NotifyWnd";
            RegisterClassW(&wc);

            hWnd = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0,0,0,0, HWND_MESSAGE, NULL, wc.hInstance, NULL);
            if (!hWnd) {
                UnregisterClassW(wc.lpszClassName, wc.hInstance);
                return;
            }

            NOTIFYICONDATAW notifyData = {0};
            notifyData.cbSize = sizeof(notifyData);
            notifyData.hWnd = hWnd;
            notifyData.uID = 1;
            notifyData.uFlags = NIF_ICON | NIF_TIP | NIF_INFO;
            notifyData.hIcon = LoadIconW(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
            wcscpy_s(notifyData.szTip, _countof(notifyData.szTip), L"AnyFSE");
            Shell_NotifyIconW(NIM_ADD, &notifyData);
        }

        // Show balloon/info using provided or created hwnd. We assume uID==1 for the icon.
        NOTIFYICONDATAW notifyInfo = {0};
        notifyInfo.cbSize = sizeof(notifyInfo);
        notifyInfo.hWnd = hWnd;
        notifyInfo.uID = 1;
        notifyInfo.uFlags = NIF_INFO;
        wcscpy_s(notifyInfo.szInfoTitle, _countof(notifyInfo.szInfoTitle), title.c_str());
        wcscpy_s(notifyInfo.szInfo, _countof(notifyInfo.szInfo), message.c_str());
        notifyInfo.dwInfoFlags = NIIF_INFO;

        Shell_NotifyIconW(NIM_MODIFY, &notifyInfo);

        // If we created the window/icon, remove it after a short delay
        if (createdWindow)
        {
            Sleep(1000);
            NOTIFYICONDATAW notifyDelete = {0};
            notifyDelete.cbSize = sizeof(notifyDelete);
            notifyDelete.hWnd = hWnd;
            notifyDelete.uID = 1;
            Shell_NotifyIconW(NIM_DELETE, &notifyDelete);
            DestroyWindow(hWnd);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
        }
    }
    void Notification::ShowNewVersion(HWND hwnd, const std::wstring &version, const std::wstring &launchUrl)
    {
        if (version.empty())
        {
            return;
        }
        std::wstring msg = L"New version " + version + L" is available.";
        Tools::Notification::Show(hwnd, L"AnyFSE Update available", msg);
    }

    void Notification::ShowCurrentVersion(HWND hwnd, bool installed)
    {
        std::wstring msg = L"Current version is " + Unicode::to_wstring(VER_VERSION_STR);
        Tools::Notification::Show(hwnd, installed ? L"AnyFSE was installed" : L"AnyFSE was updated", msg);
    }
}
