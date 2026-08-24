#pragma once

#include <windows.h>
#include <string>

namespace AnyFSE::Tools::Notification
{
    // Show a tray balloon. If hwnd is non-NULL, reuse it (assumes app already added an icon for that hwnd).
    // If hwnd is NULL, a temporary hidden window and icon will be created for the notification.
    void Show(HWND hwnd, const std::wstring& title, const std::wstring& message, const std::wstring& launchUrl = L"");

    void ShowNewVersion(HWND hwnd, const std::wstring &version, const std::wstring &launchUrl = L"");
    void ShowCurrentVersion(HWND hwnd, bool installed);
}

namespace Notification = AnyFSE::Tools::Notification;

