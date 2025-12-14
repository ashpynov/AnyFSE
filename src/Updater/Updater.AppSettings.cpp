#include <windows.h>
#include <string>

#include "Configuration/Config.hpp"
#include "Updater/Updater.hpp"
#include "Updater/Updater.AppControl.hpp"
#include "Updater/Updater.Command.hpp"
#include "Updater.AppSettings.hpp"

namespace AnyFSE::Updater
{
    // No global updater instance here; prefer per-instance Updater passed to callers.
    static UINT WM_UPDATER_COMMAND = RegisterWindowMessage(L"AnyFSE.Updater.Command");

    static LRESULT UpdaterSendCommand(COMMAND command, LPARAM lParam)
    {
        static HWND hostWindow = NULL;
        if (!hostWindow || !IsWindow(hostWindow))
        {
            hostWindow = FindWindow(L"AnyFSE", NULL);
        }

        return SendMessage(hostWindow, WM_UPDATER_COMMAND, (WPARAM) command, lParam);
    }

    void Subscribe(HWND hWnd, UINT uMsg)
    {
        SUBSCRIPTION s{hWnd, uMsg};
        UpdaterSendCommand(COMMAND::SUBSCRIBE, (LPARAM)&s);
    }

    void CheckUpdateAsync(bool includePreRelease, HWND hWnd, UINT uMsg)
    {
        UPDATECHECK uc{includePreRelease, hWnd, uMsg};
        UpdaterSendCommand(COMMAND::CHECK, (LPARAM)&uc);
    }

    void UpdateAsync(const std::wstring &tag, bool silentUpdate, HWND hWnd, UINT uMsg)
    {
        UPDATEUPDATE uu{tag, silentUpdate, hWnd, uMsg };
        UpdaterSendCommand(COMMAND::UPDATE, (LPARAM)&uu);
    }

    Updater::UpdateInfo GetLastUpdateInfo()
    {
        UPDATEINFO ui;
        UpdaterSendCommand(COMMAND::GETUPDATEINFO, (LPARAM)&ui);

        return Updater::UpdateInfo{
            ui.uiState,
            ui.newVersion,
            ui.downloadPath,
            ui.lCommandAge,
            ui.uiCommand
        };
    }

    void AnyFSE::Updater::NotifyConfigUpdated()
    {
        CONFIGUPDATED cu{
            Config::UpdateCheckInterval,
            Config::UpdatePreRelease,
            Config::UpdateNotifications,
        };
        UpdaterSendCommand(COMMAND::NOTIFYCONFIGUPDATED, (LPARAM)&cu);
    }
}

