#pragma once

#include <string>
#include <windows.h>

namespace AnyFSE::Updater
{
    int GetSince(const std::wstring &lastCheck);
    int ScheduledCheckAsync(const std::wstring &lastCheck, int checkInterval, bool includePreRelease, HWND hWnd, UINT uMsg);
    bool UpdaterHandleCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
}
