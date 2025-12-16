#include "Tools/Event.hpp"
#include "Tools/Unicode.hpp"
#include "Logging/LogManager.hpp"
#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "TroubleshootPage.hpp"


namespace AnyFSE::App::AppSettings::Settings::Page
{
    void TroubleshootPage::AddPage(std::list<SettingsLine>& settingPageList, ULONG &top)
    {
        SettingsLine & dialogLine = m_dialog.AddSettingsLine(settingPageList, top,
            L"Debug and Troubleshoot",
            L"Configure logs and advanced parameters",
            Layout::LineHeight, Layout::LinePadding, 0);

        dialogLine.SetState(FluentDesign::SettingsLine::Next);
        dialogLine.SetIcon(L'\xEBE8');
        dialogLine.OnChanged += delegate(OpenTroubleshootSettingsPage);

        ULONG topPage = 0;
        FluentDesign::SettingsLine &logLevel = m_dialog.AddSettingsLine(m_pageLinesList,
            topPage,
            L"Log level",
            L"Enable logs at specified level",
            m_troubleLogLevelCombo,
            Layout::LineHeight, Layout::LinePadding, 0,
            Layout::LauncherComboWidth);

        logLevel.OnLink = delegate(OnGotoLogsFolder);

        for (int i = (int)LogLevels::Disabled; i < (int)LogLevels::Max; i++)
        {
            std::wstring level = Unicode::to_wstring(LogManager::LogLevelToString((LogLevels)i));
            wchar_t buff[2] = {(wchar_t)i, 0};
            m_troubleLogLevelCombo.AddItem(level, L"", buff);
        }

        m_dialog.AddSettingsLine(m_pageLinesList,
            topPage,
            L"Aggressive Mode",
            L"More simple and robust logic on XboxApp start, but you will lose any manual access to the XboxApp",
            m_troubleAggressiveToggle,
            Layout::LineHeight, Layout::LinePadding, 0);

        m_dialog.AddSettingsLine(m_pageLinesList,
            topPage,
            L"Leave full screen on Home app exit",
            L"Exit to desktop mode after Home app was exited",
            m_troubleExitOnExitToggle,
            Layout::LineHeight, Layout::LinePadding, 0);
    }

    void TroubleshootPage::LoadControls()
    {
        m_troubleLogLevelCombo.SelectItem(min(max((int)LogLevels::Disabled, (int)Config::LogLevel), (int)LogLevels::Max));
        m_troubleAggressiveToggle.SetCheck(Config::AggressiveMode);
        m_troubleExitOnExitToggle.SetCheck(Config::ExitFSEOnHomeExit);
    }

    void TroubleshootPage::SaveControls()
    {
        Config::LogLevel = (LogLevels)m_troubleLogLevelCombo.GetSelectedIndex();
        Config::AggressiveMode = m_troubleAggressiveToggle.GetCheck();
        Config::ExitFSEOnHomeExit = m_troubleExitOnExitToggle.GetCheck();
    }

    void TroubleshootPage::OpenTroubleshootSettingsPage()
    {
        m_dialog.SwitchActivePage(L"Troubleshoot", &m_pageLinesList);
    }

    void TroubleshootPage::OnGotoLogsFolder()
    {
        std::wstring path = Config::GetModulePath() + L"\\logs";

        CreateDirectoryW(path.c_str(), NULL);
        Process::StartProtocol(L"\"" + path + L"\"");
    }

};