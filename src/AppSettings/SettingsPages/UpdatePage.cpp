#include "Updater/Updater.AppSettings.hpp"
#include "Tools/Event.hpp"
#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "UpdatePage.hpp"


namespace AnyFSE::App::AppSettings::Settings::Page
{
    void UpdatePage::AddPage(std::list<SettingsLine>& settingPageList, ULONG &top)
    {
        RECT rect;
        GetClientRect(m_dialog.GetHwnd(), &rect);

        SettingsLine & dialogLine = m_dialog.AddSettingsLine(settingPageList, top,
            L"Update settings",
            L"Change period and settings of new version check",
            Layout::LineHeight, Layout::LinePadding, 0);

        dialogLine.SetState(FluentDesign::SettingsLine::Next);
        dialogLine.SetIcon(L'\xEDAB');
        dialogLine.OnChanged += delegate(OpenUpdateSettingsPage);

        ULONG topPage = 0;

        m_dialog.AddSettingsLine(m_pageLinesList, topPage,
            L"Check period",
            L"New version check period",
            m_checkIntervalCombo,
            Layout::LineHeight, Layout::LinePadding, 0);

        m_checkIntervalCombo.OnChanged = delegate(UpdateSettingsChangedCheck);

        m_checkIntervalCombo.AddItem(L"Manual", L"", {(wchar_t)-2, 0});
        m_checkIntervalCombo.AddItem(L"On settings open", L"", {(wchar_t)-1, 0});
        m_checkIntervalCombo.AddItem(L"On windows boot", L"", {(wchar_t)0, 0});

        m_checkIntervalCombo.AddItem(L"Hourly", L"", {(wchar_t)1, 0});
        m_checkIntervalCombo.AddItem(L"Daily", L"", {(wchar_t)24, 0});
        m_checkIntervalCombo.AddItem(L"Weekly", L"", {(wchar_t)168, 0});
        m_checkIntervalCombo.AddItem(L"Monthly", L"", {(wchar_t)720, 0});


        m_dialog.AddSettingsLine(m_pageLinesList, topPage,
            L"Include pre-release versions",
            L"Pre-release versions are less stable",
            m_preReleaseToggle,
            Layout::LineHeight, Layout::LinePadding, 0);

        m_preReleaseToggle.OnChanged = delegate(UpdateSettingsChangedCheck);

        m_dialog.AddSettingsLine(m_pageLinesList, topPage,
            L"Show notifications",
            L"Show popup notifications if new version available",
            m_notificationsToggle,
            Layout::LineHeight, Layout::LinePadding, 0);

        m_notificationsToggle.OnChanged = delegate(SaveControls);
    }

    void UpdatePage::LoadControls()
    {
        m_checkIntervalCombo.SelectItem({(wchar_t)Config::UpdateCheckInterval, 0});
        m_preReleaseToggle.SetCheck(Config::UpdatePreRelease);
        m_notificationsToggle.SetCheck(Config::UpdateNotifications);
    }

    void UpdatePage::SaveControls()
    {
        Config::UpdateCheckInterval = (int)(short)m_checkIntervalCombo.GetCurentValue().c_str()[0];
        Config::UpdatePreRelease = m_preReleaseToggle.GetCheck();
        Config::UpdateNotifications = m_notificationsToggle.GetCheck();
        Updater::NotifyConfigUpdated();
    }

    void UpdatePage::OpenUpdateSettingsPage()
    {
        m_dialog.SwitchActivePage(L"Update settings", &m_pageLinesList);
    }

    void UpdatePage::UpdateSettingsChangedCheck()
    {
        UpdateSettingsChanged();
        if (Config::UpdateCheckInterval == -1)
        {
            Updater::CheckUpdateAsync(Config::UpdatePreRelease, m_dialog.GetHwnd(), SettingsDialog::WM_UPDATE_NOTIFICATION);
        }
    }

    void UpdatePage::UpdateSettingsChanged()
    {
        SaveControls();
        PostMessage(m_dialog.GetHwnd(), SettingsDialog::WM_UPDATE_NOTIFICATION, 0, 0);
    }
};