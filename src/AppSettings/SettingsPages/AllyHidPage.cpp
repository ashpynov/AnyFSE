#include "Ally/Ally.hpp"
#include "Tools/Event.hpp"
#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "AllyHidPage.hpp"


namespace AnyFSE::App::AppSettings::Settings::Page
{
    void AllyHidPage::AddPage(std::list<SettingsLine>& settingPageList, ULONG &top)
    {
        if (!Ally::FindHIDDevice())
        {
            return;
        }

        m_isCreated = true;
        SettingsLine & rogAllySupport = m_dialog.AddSettingsLine(settingPageList, top,
            L"ROG Ally features",
            L"ASUS ROG Ally and ROG AllyX customization experimental features",
            Layout::LineHeight, Layout::LinePadding, 0
        );
        rogAllySupport.SetState(FluentDesign::SettingsLine::Next);
        rogAllySupport.SetIcon(L"@B9ECED6F.ASUSCommandCenter_qmba6cd70vzyy");
        rogAllySupport.OnChanged += delegate(OpenAllyHidPage);

        ULONG pageTop = 0;
        SettingsLine& allyHidLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Enable ASUS ROG Ally button handling",
            L"Enable customization of Comand Center and Armoury Crate buttons",
            m_enableAllyHidToggle,
            Layout::LineHeight, 0, 0);
        allyHidLine.SetIcon(L"@B9ECED6F.ASUSCommandCenter_qmba6cd70vzyy");

        SettingsLine& acPressLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"PRESS Armoury Crate button",
            L"Define Short press right button action",
            m_acPressCombo,
            Layout::LineHeight, 0, 0);

        acPressLine.SetIcon(L"@/Assets/asus_cac_keymap_ac.png");

        allyHidLine.AddGroupItem(&acPressLine);

        SettingsLine& acHoldLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"HOLD Armoury Crate button",
            L"Define Long press right button action",
            m_acHoldCombo,
            Layout::LineHeight, 0, 0);

        allyHidLine.AddGroupItem(&acHoldLine);
        acHoldLine.SetIcon(L"@/Assets/asus_cac_keymap_ac.png");

        SettingsLine& ccPressLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"PRESS Command Center button",
            L"Define Short press left button action",
            m_ccPressCombo,
            Layout::LineHeight, Layout::LinePadding, 0);

        ccPressLine.SetIcon(L"@/Assets/asus_cac_keymap_cc.png");
        allyHidLine.AddGroupItem(&ccPressLine);

        allyHidLine.SetState(SettingsLine::State::Opened);
    }

    void AllyHidPage::LoadControls()
    {
        if (!m_isCreated)
        {
            return;
        }

        m_acPressCombo.AddItem(L"GameBar Command Center", L"", L"GamebarCommandCenter");
        m_acHoldCombo.AddItem(L"Task switcher", L"", L"TaskSwitcher");
        m_ccPressCombo.AddItem(L"Home", L"", L"HomeApp");
    }

    void AllyHidPage::SaveControls()
    {
        if (!m_isCreated)
        {
            return;
        }
    }

    void AllyHidPage::OpenAllyHidPage()
    {
        m_dialog.SwitchActivePage(L"ROG Ally", &m_pageLinesList);
    }
};
