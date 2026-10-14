#include <windows.h>
#include "Logging/LogManager.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "Configuration/Config.hpp"
#include "FluentDesign/Theme.hpp"
#include "Tools/Unicode.hpp"
#include "Updater/Updater.hpp"

#define byte ::byte


#ifndef VER_VERSION_STR
#define VER_VERSION_STR "0.0.0"
#endif
#include <gdiplusenums.h>
#include <string>
#include <Tools/Event.hpp>
#include "SettingsDialog.hpp"
#include "SettingsLayout.hpp"
#include "Tools/Notification.hpp"
#include "Tools/Localization.hpp"

namespace AnyFSE::App::AppSettings::Settings
{
    static Logger log = LogManager::GetLogger("Settings");

    void SettingsDialog::AddUpdateControls()
    {
        int left = Layout::MarginLeft;
        int top = -Layout::MarginBottom - Layout::UpdateHeight;

        m_updateCheckButton
            .SetAnchor(Align::BottomLeft(), GetDialogCenteredRect)
            .Create(m_hDialog, left, top, Layout::UpdateHeight, Layout::UpdateHeight)
            .SetIcon(L"\xE117")
            .SetFlat(true)
            .Enable(true)
            .OnChanged += delegate(OnCheckUpdate);

        m_updateCurrentText
            .SetAnchor(Align::BottomLeft(), GetDialogCenteredRect)
            .Create(m_hDialog, left + Layout::UpdateHeight + Layout::ButtonPadding / 4, top, 200, Layout::UpdateHeight );

        m_updateCurrentText.SetColor(Theme::Colors::TextDisabled);
        m_updateCurrentText.Format().SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);

        m_updateButton
            .SetAnchor(Align::BottomLeft(), GetDialogCenteredRect)
            .Create(m_hDialog,
                left + Layout::UpdateHeight + Layout::ButtonPadding / 4, top + Layout::UpdateHeight / 2,
                200, Layout::UpdateHeight / 2 )
            .Show(false)
            .SetLinkStyle()
            .SetMenu(
                std::vector<Popup::PopupItem>
                {
                    Popup::PopupItem(L"\xE2B4", Translate(L"settingsUpdateViewRelease"), delegate(OnShowVersion)),
                    Popup::PopupItem(L"\xE118", Translate(L"settingsUpdateDownloadAndUpdate"), delegate(OnUpdate))
                }, 400, TPM_LEFTALIGN
            );

        // m_updateButton.OnChanged += delegate(OnUpdate);
        SIZE sz = m_updateButton.GetMinSize();
        m_updateButton.SetSize(sz.cx, -1);

        OnUpdateNotification();
    }

    bool SettingsDialog::SetVersionStatus(const Updater::UpdateInfo& uiInfo)
    {
        using UpdaterState = Updater::UpdaterState;

        std::wstring icon = L"\xE895";

        std::wstring version = uiInfo.newVersion.empty() ? Config::UpdateLastVersion : uiInfo.newVersion;

        bool hasVersion = !version.empty();
        std::wstring cVersion = TranslateF(
            hasVersion ? L"settingsCurrentVersionFmt" : L"settingsVersionFmt",
            Unicode::to_wstring(VER_VERSION_STR).c_str());
        std::wstring aVersion = hasVersion ? TranslateF(L"settingsAvailableVersionFmt", version.c_str()) : L"";

        bool delayed = (LONGLONG)GetTickCount64() > uiInfo.lCommandAge + 1000;
        UpdaterState command = delayed ? uiInfo.uiState : uiInfo.uiCommand;
        bool enableAnimation =
            command == UpdaterState::CheckingUpdate || uiInfo.uiState == UpdaterState::CheckingUpdate;
        bool enableCheck = !enableAnimation;
        bool enableStatus = enableCheck;

        if (command == UpdaterState::CheckingUpdate
                || uiInfo.uiState == UpdaterState::CheckingUpdate)
        {
            icon = L"\xE895";
            cVersion = TranslateF(L"settingsCurrentVersionFmt", Unicode::to_wstring(VER_VERSION_STR).c_str());
            aVersion =   (uiInfo.uiState == UpdaterState::CheckingUpdate)   ? Translate(L"settingsCheckingNewVersion")
                       : (command == UpdaterState::NetworkFailed)           ? Translate(L"settingsNetworkFailed")
                       : (uiInfo.uiState == UpdaterState::NetworkFailed)    ? Translate(L"settingsNetworkFailed")
                       : (uiInfo.uiState == UpdaterState::Done
                          && !uiInfo.newVersion.empty())                    ? TranslateF(L"settingsNewVersionAvailableFmt", uiInfo.newVersion.c_str())
                                                                            : Translate(L"settingsNoNewVersionAvailable");
        }
        else if (command == UpdaterState::NetworkFailed && !enableAnimation)
        {
            icon = L"\xF305\xEB5B";
        }
        else if (uiInfo.uiState == UpdaterState::Downloading || command == UpdaterState::Downloading || command == UpdaterState::ReadyForUpdate)
        {
            icon = (uiInfo.uiState == UpdaterState::NetworkFailed) ? L"\xF305\xEB5B" : L"\xE118";
            aVersion = (uiInfo.uiState == UpdaterState::NetworkFailed) ? Translate(L"settingsFailedToDownload") : L"";
            cVersion = (uiInfo.uiState == UpdaterState::Downloading)
                ? TranslateF(L"settingsDownloadingVersionFmt", uiInfo.newVersion.c_str())
                : TranslateF(L"settingsUpdatingToVersionFmt", uiInfo.newVersion.c_str());
            enableAnimation = false;
            enableCheck = false;
            enableStatus = false;
        }

        bool showSecond = !aVersion.empty();
        m_updateCheckButton.Enable(enableCheck);
        m_updateCheckButton.SetIcon(icon);

        if (showSecond)
        {
            if (m_updateButton.GetText() != aVersion)
            {
                m_updateButton.SetText(aVersion);
                SIZE sz = m_updateButton.GetMinSize();
                m_updateButton.SetSize(sz.cx, m_theme.DpiScale(Layout::UpdateHeight) / 2);
            }
            m_updateButton
                .Enable(enableStatus)
                .Show(true);
        }
        else
        {
            m_updateButton
                .Show(false)
                .SetText(L"");
        }
        m_updateCurrentText.SetSize(-1, m_theme.DpiScale(Layout::UpdateHeight) / (showSecond ? 2 : 1));
        m_updateCurrentText.SetText(cVersion);

        const bool downloading = uiInfo.uiState == UpdaterState::Downloading;
        const bool preparingUpdate = uiInfo.uiState == UpdaterState::ReadyForUpdate;
        if (downloading || preparingUpdate)
        {
            if (preparingUpdate)
                m_updateCheckButton.SetProgress(1.0);
            else if (uiInfo.totalBytes > 0)
                m_updateCheckButton.SetProgress(double(uiInfo.downloadedBytes) / double(uiInfo.totalBytes));
            else
                m_updateCheckButton.SetIndeterminateProgress();
        }
        else if (enableAnimation)
        {
            m_updateCheckButton.Animate(0, 360, 750, true);
        }
        else
        {
            m_updateCheckButton.ClearProgress();
            m_updateCheckButton.CancelAnimation();
        }

        UpdateWindow(m_updateCheckButton.GetHwnd());
        UpdateWindow(m_updateButton.GetHwnd());

        return !delayed || uiInfo.uiState == UpdaterState::CheckingUpdate || downloading || preparingUpdate;
    }

    void SettingsDialog::OnCheckUpdate()
    {
        Updater::CheckUpdateAsync(Config::UpdatePreRelease, m_hDialog, WM_UPDATE_NOTIFICATION);
        UpdateVersionStatusDelay(10);
    }

    void SettingsDialog::OnUpdateNetworkRestore()
    {
        m_updateCheckButton.SetIcon(L"\xE895");
    }

    void SettingsDialog::OnUpdateNotification()
    {
        Updater::Subscribe(m_hDialog, WM_UPDATE_NOTIFICATION);
        Updater::UpdateInfo info = Updater::GetLastUpdateInfo();

        if (info.uiState == Updater::UpdaterState::UpdateExecuted)
        {
            OnClose();
            return;
        }

        // An idle updater has no result yet; preserve the version saved by an earlier check.
        if (info.uiState == Updater::UpdaterState::Done && info.uiCommand == Updater::UpdaterState::CheckingUpdate)
        {
            if (Config::UpdateNotifications
                && !info.newVersion.empty()
                && info.newVersion != Config::UpdateLastVersion)
            {
                Notification::ShowNewVersion(NULL, info.newVersion);
            }
            Config::SaveUpdateVersion(info.newVersion);
        }

        if (SetVersionStatus(info))
        {
            UpdateVersionStatusDelay(100);
        }
    }

    void SettingsDialog::UpdateVersionStatusDelay(UINT delay)
    {
        SetTimer(m_hDialog, UpdateStatusTimerId, delay,
        [](HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
        {
            KillTimer(hWnd, idEvent);
            auto* dialog = reinterpret_cast<SettingsDialog*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            if (dialog)
                dialog->OnUpdateStatusTimer();
        });
    }

    void SettingsDialog::OnUpdateStatusTimer()
    {
        const Updater::UpdateInfo info = Updater::GetLastUpdateInfo();
        if (info.uiState == Updater::UpdaterState::UpdateExecuted)
        {
            OnClose();
            return;
        }
        if (SetVersionStatus(info))
            UpdateVersionStatusDelay(100);
    }

    void SettingsDialog::OnShowVersion()
    {
        if (!Config::UpdateLastVersion.empty())
        {
            Updater::ShowVersion(Config::UpdateLastVersion);
        }
    }

    void SettingsDialog::OnUpdate()
    {
        if (!Config::UpdateLastVersion.empty())
        {
            Updater::UpdateAsync(Config::UpdateLastVersion, true, m_hDialog, WM_UPDATE_NOTIFICATION);
            UpdateVersionStatusDelay(10);
        }
    }
}
