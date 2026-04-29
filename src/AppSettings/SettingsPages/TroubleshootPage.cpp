#include "Tools/Event.hpp"
#include "Tools/Unicode.hpp"
#include "Logging/LogManager.hpp"
#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "TroubleshootPage.hpp"
#include "Tools/Paths.hpp"
#include <filesystem>


namespace AnyFSE::App::AppSettings::Settings::Page
{
    void TroubleshootPage::AddPage(std::list<SettingsLine>& settingPageList, ULONG &top)
    {
        ULONG topPage = 0;

        FluentDesign::SettingsLine &logLevel = m_dialog.AddSettingsLine(settingPageList,
            top,
            L"Set logs for troubleshoot",
            L"Navigate to log files folder",
            m_troubleLogLevelCombo,
            Layout::LineHeight, Layout::LinePadding, 0,
            Layout::LauncherComboWidth);

        logLevel.SetIcon(L'\xEBE8');
        logLevel.OnLink = delegate(OnGotoLogsFolder);

        for (int i = (int)LogLevels::Disabled; i < (int)LogLevels::Max; i++)
        {
            std::wstring level = Unicode::to_wstring(LogManager::LogLevelToString((LogLevels)i));
            wchar_t buff[2] = {(wchar_t)i, 0};
            m_troubleLogLevelCombo.AddItem(level, L"", buff);
        }
    }

    void TroubleshootPage::LoadControls()
    {
        m_troubleLogLevelCombo.SelectItem(min(max((int)LogLevels::Disabled, (int)Config::LogLevel), (int)LogLevels::Max));
        m_troubleAggressiveToggle.SetCheck(Config::AggressiveMode);
    }

    void TroubleshootPage::SaveControls()
    {
        Config::LogLevel = (LogLevels)m_troubleLogLevelCombo.GetSelectedIndex();
        Config::AggressiveMode = m_troubleAggressiveToggle.GetCheck();
    }

    void TroubleshootPage::OpenTroubleshootSettingsPage()
    {
        m_dialog.SwitchActivePage(L"Troubleshoot", &m_pageLinesList);
    }

    void TroubleshootPage::OnGotoLogsFolder()
    {
        std::wstring path = Tools::Paths::GetLogsPath();

        std::filesystem::create_directories(path);
        Process::StartProtocol(L"\"" + path + L"\"");
    }

};