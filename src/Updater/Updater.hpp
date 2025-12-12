#pragma once

#include <string>
#include <windows.h>

namespace AnyFSE::Updater
{
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
        std::wstring newVersion;
        std::wstring downloadPath;
        long long lCommandAge = 0;
        UpdaterState uiCommand = UpdaterState::Idle;
    };

    void Subscribe(HWND hWnd, UINT uMsg);
    void CheckUpdateAsync(bool includePreRelease, HWND hWnd, UINT uMsg);
    void UpdateAsync(const std::wstring &tag, bool silentUpdatee, HWND hWnd, UINT uMsg);
    Updater::UpdateInfo GetLastUpdateInfo();
    void ShowVersion(const std::wstring &tag);

}
