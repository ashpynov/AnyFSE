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

        SettingsLine& acPressLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"PRESS Armoury Crate button",
            L"Define Short press right button action",
            m_acPressCombo,
            Layout::LineHeight, 0, 0, 240);

        acPressLine.SetIcon(L"@/Assets/asus_cac_keymap_ac.png");

        m_pAllyHidLine->AddGroupItem(&acPressLine);

        SettingsLine& acHoldLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"HOLD Armoury Crate button",
            L"Define Long press right button action",
            m_acHoldCombo,
            Layout::LineHeight, 0, 0, 240);

        m_pAllyHidLine->AddGroupItem(&acHoldLine);
        acHoldLine.SetIcon(L"@/Assets/asus_cac_keymap_ac.png");

        SettingsLine& ccPressLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"PRESS Command Center button",
            L"Define Short press left button action",
            m_ccPressCombo,
            Layout::LineHeight, Layout::LinePadding, 0, 240);

        ccPressLine.SetIcon(L"@/Assets/asus_cac_keymap_cc.png");
        m_pAllyHidLine->AddGroupItem(&ccPressLine);

        m_pAllyHidLine->SetState(SettingsLine::State::Opened);
    }

    void AllyHidPage::LoadControls()
    {
        if (!Ally::IsSupported())
        {
            return;
        }

        for ( auto h : Ally::Handlers::KnownHandlers)
        {
            m_acPressCombo.AddItem(h.name, L"", h.code);
            m_acHoldCombo.AddItem(h.name, L"", h.code);
            m_ccPressCombo.AddItem(h.name, L"", h.code);
        }

        m_enableAllyHidToggle.SetCheck(Config::AllyHidEnable);
        m_acPressCombo.SelectItem(Config::AllyHidACPress);
        m_acHoldCombo.SelectItem(Config::AllyHidACHold);
        m_ccPressCombo.SelectItem(Config::AllyHidCCPress);
    }

    void AllyHidPage::SaveControls()
    {
        if (!Ally::IsSupported())
        {
            return;
        }

        Config::AllyHidEnable = m_enableAllyHidToggle.GetCheck();
        Config::AllyHidACPress = m_acPressCombo.GetCurentValue();
        Config::AllyHidACHold = m_acHoldCombo.GetCurentValue();
        Config::AllyHidCCPress = m_ccPressCombo.GetCurentValue();

        Ally::EnableNativeHandler(!Config::AllyHidEnable);
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
};
