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
                    Popup::PopupItem(L"\xE2B4", L"View release", delegate(OnShowVersion)),
                    Popup::PopupItem(L"\xEDAB", L"Download & update", delegate(OnUpdate))
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
        std::wstring cVersion = (hasVersion ? L"Current version v" : L"Version v") + Unicode::to_wstring(VER_VERSION_STR);
        std::wstring aVersion = hasVersion ? L"Available version " + version : L"";

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
            cVersion = L"Current version v" + Unicode::to_wstring(VER_VERSION_STR);
            aVersion =   (uiInfo.uiState == UpdaterState::CheckingUpdate)   ? L"Checking for new version"
                       : (command == UpdaterState::NetworkFailed)           ? L"Network failed"
                       : (uiInfo.uiState == UpdaterState::NetworkFailed)    ? L"Network failed"
                       : (uiInfo.uiState == UpdaterState::Done
                          && !uiInfo.newVersion.empty())                    ? L"New version " + uiInfo.newVersion + L" available"
                                                                            : L"No new version available";
        }
        else if (command == UpdaterState::NetworkFailed && !enableAnimation)
        {
            icon = L"\xF305\xE871";
        }
        else if (uiInfo.uiState == UpdaterState::Downloading || command == UpdaterState::Downloading || command == UpdaterState::ReadyForUpdate)
        {
            icon = L"\xF16A";
            aVersion = (uiInfo.uiState == UpdaterState::NetworkFailed)  ? L"Failed to download" : L"";
            cVersion = ((uiInfo.uiState == UpdaterState::Downloading) ? L"Downloading version " : L"Updating to version ") + uiInfo.newVersion;
            enableAnimation = true;
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

        if (enableAnimation)
        {
            m_updateCheckButton.Animate(0, 360, 750, true);
        }
        else
        {
            m_updateCheckButton.CancelAnimation();
        }

        UpdateWindow(m_updateCheckButton.GetHwnd());
        UpdateWindow(m_updateButton.GetHwnd());

        return !delayed;
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
        Updater::UpdateInfo info = Updater::GetLastUpdateInfo();

        if (SetVersionStatus(Updater::GetLastUpdateInfo()))
        {
            UpdateVersionStatusDelay(100);
        }
        Config::SaveUpdateVersion(info.newVersion);
        Updater::Subscribe(m_hDialog, WM_UPDATE_NOTIFICATION);
    }

    void SettingsDialog::UpdateVersionStatusDelay(UINT delay)
    {
        SetTimer(m_hDialog, 0, (UINT)delay,
        [](HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
        {
            KillTimer(hWnd, idEvent);
            SendMessage(hWnd, WM_UPDATE_NOTIFICATION, 0, 0);
        });
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