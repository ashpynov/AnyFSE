#include "Ally/Ally.hpp"
#include "Ally/Handlers.hpp"
#include "Tools/Event.hpp"
#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "AllyHidPage.hpp"


namespace AnyFSE::App::AppSettings::Settings::Page
{
    void AllyHidPage::AddPage(std::list<SettingsLine>& settingPageList, ULONG &top)
    {
        if (!Ally::IsSupported())
        {
            return;
        }

        m_theme.OnThemeChanged += delegate(ReloadIcons);

        SettingsLine & rogAllySupport = m_dialog.AddSettingsLine(settingPageList, top,
            L"ROG Ally features",
            L"ASUS ROG Ally and ROG AllyX customization experimental features",
            Layout::LineHeight, Layout::LinePadding, 0
        );
        rogAllySupport.SetState(FluentDesign::SettingsLine::Next);
        rogAllySupport.SetIcon(L"@B9ECED6F.ASUSCommandCenter_qmba6cd70vzyy");
        rogAllySupport.OnChanged += delegate(OpenAllyHidPage);

        ULONG pageTop = 0;
        m_pAllyHidLine = &m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Enable alternative ASUS ROG Ally button handling",
            L"Alternative handling of some ASUS Optimization service functions",
            m_enableAllyHidToggle,
            Layout::LineHeight, 0, 0);
        m_pAllyHidLine->SetIcon(L"@B9ECED6F.ASUSCommandCenter_qmba6cd70vzyy");
        m_enableAllyHidToggle.OnChanged = delegate(EnableAllyHidChanged);

        m_pACPressLine = &m_pAllyHidLine->AddGroupItem(
            &m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
                L"PRESS Armoury Crate button",
                L"Define Short press right button action",
                m_acPressCombo,
                Layout::LineHeight, 0, 0, 240));

         m_pACHoldLine = &m_pAllyHidLine->AddGroupItem(
            &m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
                L"HOLD Armoury Crate button",
                L"Define Long press right button action",
                m_acHoldCombo,
                Layout::LineHeight, 0, 0, 240));

        m_pCCPressLine = &m_pAllyHidLine->AddGroupItem(
            &m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
                L"PRESS Command Center button",
                L"Define Short press left button action",
                m_ccPressCombo,
                Layout::LineHeight, 0, 0, 240));

        m_pAllyHidLine->AddGroupItem(
            &m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
                L"Secondary buttons actions (Mode+)",
                L"",
                Layout::LinePadding * 4, 0, 0));

        m_pModeACPressLine = &m_pAllyHidLine->AddGroupItem(
            &m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
                L"Mode+PRESS Armoury Crate button",
                L"Define Short press right button action with Mode key",
                m_modeACPressCombo,
                Layout::LineHeight, 0, 0, 240));

        m_pModeACHoldLine = &m_pAllyHidLine->AddGroupItem(
            &m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
                L"Mode+HOLD Armoury Crate button",
                L"Define Long press right button action with Mode key",
                m_modeACHoldCombo,
                Layout::LineHeight, 0, 0, 240));

        m_pModeCCPressLine = &m_pAllyHidLine->AddGroupItem(
            &m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
                L"Mode+PRESS Command Center button",
                L"Define Short press left button action with Mode key",
                m_modeCCPressCombo,
                Layout::LineHeight, Layout::LinePadding, 0, 240));

        ReloadIcons();

        m_pAllyHidLine->SetState(SettingsLine::State::Opened);
    }

    void AllyHidPage::LoadControls()
    {
        if (!Ally::IsSupported())
        {
            return;
        }

        for (auto h : Ally::Handlers::KnownHandlers)
        {
            m_acPressCombo.AddItem(h.name, L"", h.code);
            m_acHoldCombo.AddItem(h.name, L"", h.code);
            m_ccPressCombo.AddItem(h.name, L"", h.code);
            m_modeACPressCombo.AddItem(h.name, L"", h.code);
            m_modeACHoldCombo.AddItem(h.name, L"", h.code);
            m_modeCCPressCombo.AddItem(h.name, L"", h.code);
        }

        m_enableAllyHidToggle.SetCheck(Config::AllyHidEnable);
        m_acPressCombo.SelectItem(Config::AllyHidACPress);
        m_acHoldCombo.SelectItem(Config::AllyHidACHold);
        m_ccPressCombo.SelectItem(Config::AllyHidCCPress);
        m_modeACPressCombo.SelectItem(Config::AllyHidModeACPress);
        m_modeACHoldCombo.SelectItem(Config::AllyHidModeACHold);
        m_modeCCPressCombo.SelectItem(Config::AllyHidModeCCPress);

        EnableAllyHidChanged();
    }

    void AllyHidPage::SaveControls()
    {
        if (!Ally::IsSupported())
        {
            return;
        }

        bool changed = Config::AllyHidEnable != m_enableAllyHidToggle.GetCheck();
        Config::AllyHidEnable = m_enableAllyHidToggle.GetCheck();
        Config::AllyHidACPress = m_acPressCombo.GetCurentValue();
        Config::AllyHidACHold = m_acHoldCombo.GetCurentValue();
        Config::AllyHidCCPress = m_ccPressCombo.GetCurentValue();
        Config::AllyHidModeACPress = m_modeACPressCombo.GetCurentValue();
        Config::AllyHidModeACHold = m_modeACHoldCombo.GetCurentValue();
        Config::AllyHidModeCCPress = m_modeCCPressCombo.GetCurentValue();

        if (changed || Config::AllyHidEnable)
        {
            Ally::EnableNativeHandler(!Config::AllyHidEnable);
        }
    }

    void AllyHidPage::OpenAllyHidPage()
    {
        m_dialog.SwitchActivePage(L"ROG Ally", &m_pageLinesList);
    }

    void AllyHidPage::EnableAllyHidChanged()
    {
        bool enabled = m_enableAllyHidToggle.GetCheck();
        for (SettingsLine * child : m_pAllyHidLine->GetGroupItems())
        {
            child->Enable(enabled);
        }
    }

    void AllyHidPage::ReloadIcons()
    {
        std::wstring path = L"@/Assets/asus_cac_keymap_";
        std::wstring suffix = m_theme.IsDarkThemeEnabled() ? L".png" : L".theme-light.png";
        m_pACHoldLine->SetIcon(path + L"ac" + suffix);
        m_pCCPressLine->SetIcon(path + L"cc" + suffix);
        m_pACPressLine->SetIcon(path + L"ac" + suffix);
        m_pModeACPressLine->SetIcon(path + L"ac" + suffix);
        m_pModeACHoldLine->SetIcon(path + L"cc" + suffix);
        m_pModeCCPressLine->SetIcon(path + L"ac" + suffix);
    }
};
