#include "resource.h"
#include <windows.h>
#include "Logging/LogManager.hpp"
#include "SettingsDialog.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Event.hpp"
#include "Updater/Updater.AppSettings.hpp"


namespace AnyFSE::App::AppSettings::Settings
{
    static Logger log = LogManager::GetLogger("Settings");

    void SettingsDialog::AddUpdateSettingsPage(ULONG &top)
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);

        m_pUpdateSettingsLine = &AddSettingsLine(m_settingPageList, top,
            L"Update settings",
            L"Change period and settings of new version check",
            Layout_LineHeight, Layout_LinePadding, 0);

        m_pUpdateSettingsLine->SetState(FluentDesign::SettingsLine::Next);
        m_pUpdateSettingsLine->SetIcon(L'\xEDAB');
        m_pUpdateSettingsLine->OnChanged += delegate(OpenUpdateSettingsPage);

        AddUpdateSettingsPageControls();
    }

    void SettingsDialog::AddUpdateSettingsPageControls()
    {

        ULONG top = 0;

        AddSettingsLine(m_updateSettingPageList, top,
            L"Check period",
            L"New version check period",
            m_updateSettingsPeriodCombo,
            Layout_LineHeight, Layout_LinePadding, 0);

        m_updateSettingsPeriodCombo.OnChanged = delegate(UpdateSettingsChangedCheck);

        m_updateSettingsPeriodCombo.AddItem(L"Manual", L"", {(wchar_t)-2, 0});
        m_updateSettingsPeriodCombo.AddItem(L"On settings open", L"", {(wchar_t)-1, 0});
        m_updateSettingsPeriodCombo.AddItem(L"On windows boot", L"", {(wchar_t)0, 0});

        m_updateSettingsPeriodCombo.AddItem(L"Hourly", L"", {(wchar_t)1, 0});
        m_updateSettingsPeriodCombo.AddItem(L"Daily", L"", {(wchar_t)24, 0});
        m_updateSettingsPeriodCombo.AddItem(L"Weekly", L"", {(wchar_t)168, 0});
        m_updateSettingsPeriodCombo.AddItem(L"Monthly", L"", {(wchar_t)720, 0});


        AddSettingsLine(m_updateSettingPageList, top,
            L"Include pre-release versions",
            L"Pre-release versions are less stable",
            m_updateSettingsPreReleaseToggle,
            Layout_LineHeight, Layout_LinePadding, 0);

        m_updateSettingsPreReleaseToggle.OnChanged = delegate(UpdateSettingsChangedCheck);

        AddSettingsLine(m_updateSettingPageList, top,
            L"Show notifications",
            L"Show popup notifications if new version available",
            m_updateSettingsNotificationsToggle,
            Layout_LineHeight, Layout_LinePadding, 0);

        m_updateSettingsNotificationsToggle.OnChanged = delegate(UpdateSettingsSaveControls);

        AddSettingsLine(m_updateSettingPageList, top,
            L"Update on click",
            L"Run updater on click, instead of go to release page",
            m_updateSettingsUpdateToggle,
            Layout_LineHeight, Layout_LinePadding, 0);

        m_updateSettingsUpdateToggle.OnChanged = delegate(UpdateSettingsChanged);
    }

    void SettingsDialog::OpenUpdateSettingsPage()
    {
        m_pageName = L"Update settings";
        SwitchActivePage(&m_updateSettingPageList);
    }

    void SettingsDialog::UpdateSettingsLoadControls()
    {
        m_updateSettingsPeriodCombo.SelectItem({(wchar_t)Config::UpdateCheckInterval, 0});
        m_updateSettingsPreReleaseToggle.SetCheck(Config::UpdatePreRelease);
        m_updateSettingsNotificationsToggle.SetCheck(Config::UpdateNotifications);
        m_updateSettingsUpdateToggle.SetCheck(Config::UpdateOnClick);
    }

    void SettingsDialog::UpdateSettingsChangedCheck()
    {
        UpdateSettingsChanged();
        if (Config::UpdateCheckInterval == -1)
        {
            OnCheckUpdate();
        }
    }

    void SettingsDialog::UpdateSettingsChanged()
    {
        UpdateSettingsSaveControls();
        UpdateVersionStatusDelay(10);
    }

    void SettingsDialog::UpdateSettingsSaveControls()
    {
        Config::UpdateCheckInterval = (int)(short)m_updateSettingsPeriodCombo.GetCurentValue().c_str()[0];
        Config::UpdatePreRelease = m_updateSettingsPreReleaseToggle.GetCheck();
        Config::UpdateNotifications = m_updateSettingsNotificationsToggle.GetCheck();
        Config::UpdateOnClick = m_updateSettingsUpdateToggle.GetCheck();

        Updater::NotifyConfigUpdated();
    }
}