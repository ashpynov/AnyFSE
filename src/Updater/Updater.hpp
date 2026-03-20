#pragma once

#include <string>
#include <windows.h>

namespace AnyFSE::Updater
{
    enum COMMAND
    {
        SUBSCRIBE = 0,
        CHECK,
        UPDATE,
        GETUPDATEINFO,
        NOTIFYCONFIGUPDATED
    };

    enum UpdaterState
    {
        Idle = 0,
        Done = Idle,
        NetworkFailed,
        CheckingUpdate,
        Downloading,
        ReadyForUpdate
    };

    struct UpdateInfo
    {
        UpdaterState uiState = UpdaterState::Idle;
        std::wstring newVersion = L"\0";
        std::wstring downloadPath = L"\0";
        long long lCommandAge = 0;
        UpdaterState uiCommand = UpdaterState::Idle;
    };

    struct SUBSCRIPTION
    {
        HWND hWnd;
        UINT uMsg;
    };

    struct UPDATECHECK
    {
        BOOL bIncludePreRelease;
        HWND hWnd;
        UINT uMsg;
    };

    struct UPDATEUPDATE
    {
        std::wstring newVersion;
        BOOL bSilent;
        HWND hWnd;
        UINT uMsg;
    };

    struct UPDATEINFO
    {
        UpdaterState uiState;
        wchar_t newVersion[MAX_PATH] = L"\0";
        wchar_t downloadPath[MAX_PATH] = L"\0";
        long long lCommandAge;
        UpdaterState uiCommand;

        void put(UpdateInfo i)
        {
            uiState = i.uiState;
            lCommandAge = i.lCommandAge;
            uiCommand = i.uiCommand;
            wcsncpy_s(newVersion, i.newVersion.c_str(), MAX_PATH - 1);
            wcsncpy_s(downloadPath, i.downloadPath.c_str(), MAX_PATH - 1);
        }
    };

    struct CONFIGUPDATED
    {
        int iCheckInterval;
        bool bPreRelease;
        bool bNotifications;
    };

    void Subscribe(HWND hWnd, UINT uMsg);
    void CheckUpdateAsync(bool includePreRelease, HWND hWnd, UINT uMsg);
    void UpdateAsync(const std::wstring &tag, bool silentUpdatee, HWND hWnd, UINT uMsg);
    Updater::UpdateInfo GetLastUpdateInfo();
    void ShowVersion(const std::wstring &tag);

    int GetSince(const std::wstring &lastCheck);
    int ScheduledCheckAsync(const std::wstring &lastCheck, int checkInterval, bool includePreRelease, HWND hWnd, UINT uMsg);
    bool UpdaterHandleCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);

    void NotifyConfigUpdated();
}
