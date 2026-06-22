#include "Tools/Event.hpp"
#include "Tools/Unicode.hpp"
#include "Logging/LogManager.hpp"
#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "ConfirmationsPage.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Localization.hpp"



namespace AnyFSE::App::AppSettings::Settings::Page
{
    SettingsPage * ConfirmationsPage::AddLine(std::list<SettingsLine>& settingPageList, ULONG &top)
    {
        FluentDesign::SettingsLine &confirmationsLine = m_dialog.AddSettingsLine(settingPageList,
            top,
            Translate(L"settingsConfirmationsSetup"),
            Translate(L"settingsConfirmationsSetupDescription"),
            Layout::LineHeight, Layout::LinePadding, 0,
            Layout::LauncherComboWidth);

        confirmationsLine.SetIcon(L'\xE9CE');
        confirmationsLine.SetState(SettingsLine::State::Next);
        confirmationsLine.OnChanged = delegate(OpenConfirmationsSettingsPage);
        return this;
    }

    void ConfirmationsPage::AddPage(std::list<SettingsLine>& settingPageList, ULONG &top)
    {
        ULONG pageTop = 0;
        m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            Translate(L"settingsConfirmationsEnterMode"),
            Translate(L"settingsConfirmationsEnterModeDescription"),
            m_confirmEnterCombo,
            Layout::LineHeight, Layout::LinePadding, 0, 330)
            .SetIcon(L'\xE93A');

        m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            Translate(L"settingsConfirmationsExitMode"),
            L"",
            m_confirmExitCombo,
            Layout::LineHeight, Layout::LinePadding, 0, 330)
            .SetIcon(L'\xEE47');

        std::vector<std::wstring> enterOptions{
            Translate(L"settingsConfirmationsAsk"),
            Translate(L"settingsConfirmationsRestartToOptimize"),
            Translate(L"settingsConfirmationsStartNowNoOptimizations")
        };
        std::vector<std::wstring> exitOptions{
            Translate(L"settingsConfirmationsAsk"),
            Translate(L"settingsConfirmationsExitWithoutConfirmation")
        };

        for (auto option: enterOptions)
        {
            m_confirmEnterCombo.AddItem(option, L"", option);
        }

        for (auto option: exitOptions)
        {
            m_confirmExitCombo.AddItem(option, L"", option);
        }

        m_confirmEnterCombo.OnChanged = delegate(SaveControls);
        m_confirmExitCombo.OnChanged = delegate(SaveControls);
    }

    void ConfirmationsPage::LoadControls()
    {
        std::wstring key = L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration\\SystemDialogResults";
        m_confirmEnterCombo.SelectItem(
            Registry::ReadDWORD(key, L"EnterGamingPostureConfirmation_Config",
                Registry::ReadDWORD(key, L"EnterGamingPostureConfirmation", 0)));
        m_confirmExitCombo.SelectItem(Registry::ReadDWORD(key, L"ExitGamingPostureConfirmation", 0));
    }

    void ConfirmationsPage::SaveControls()
    {
        std::wstring key = L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration\\SystemDialogResults";
        Registry::WriteDWORD(key, L"EnterGamingPostureConfirmation_Config", m_confirmEnterCombo.GetSelectedIndex());
        Registry::WriteDWORD(key, L"EnterGamingPostureConfirmation", m_confirmEnterCombo.GetSelectedIndex());
        Registry::WriteDWORD(key, L"ExitGamingPostureConfirmation", m_confirmExitCombo.GetSelectedIndex());
    }

    void ConfirmationsPage::OpenConfirmationsSettingsPage()
    {
        m_dialog.SwitchActivePage(Translate(L"settingsConfirmationsPageTitle"), &m_pageLinesList);
    }
};
