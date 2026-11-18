#include "Tools/Event.hpp"
#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "SupportPage.hpp"
#include "Tools/Paths.hpp"
#include "Tools/Localization.hpp"


namespace AnyFSE::App::AppSettings::Settings::Page
{
    void SupportPage::AddPage(std::list<SettingsLine>& settingPageList, ULONG &top)
    {
        m_dialog.AddSettingsLine(settingPageList, top,
                Translate(L"settingsRelatedSupport"),
                L"",
                Layout::LineHeightSmall, 0, 0)
            .SetState(FluentDesign::SettingsLine::Caption);

        SettingsLine &support = m_dialog.AddSettingsLine(settingPageList, top,
                Translate(L"settingsSupportAndCommunity"),
                L"",
                Layout::LineHeight, 0, 0)
            .SetIcon(L'\xF6FA');
        support.OnChanged += delegateparam(m_dialog.UpdateLine, &support);

        support.AddGroupItem(&m_dialog.AddSettingsLine(settingPageList, top,
                L"",
                L"",
                Layout::LineHeightSmall, Layout::LinePadding, Layout::LineSmallMargin))
            // .SetMaxColumns(1)
            .AddLinkButton(Translate(L"settingsDiscordCommunity"), L"https://discord.gg/hnVwuTzDmk")
            .AddLinkButton(Translate(L"settingsNavigateLogsFolder"), Tools::Paths::GetLogsPath() + L"\\")
            .AddLinkButton(Translate(L"settingsSourceCodeGithub"), L"https://github.com/ashpynov/AnyFSE/")
            .AddLinkButton(Translate(L"settingsNavigateConfigFolder"), Tools::Paths::GetConfigPath() + L"\\")
            .AddLinkButton(Translate(L"settingsReportIssueGithub"), L"https://github.com/ashpynov/AnyFSE/issues")
            .AddLinkButton(Translate(L"settingsNavigateSplashFolder"), Tools::Paths::GetSplashDefaultPath() + L"\\");

        support.SetState(FluentDesign::SettingsLine::Opened);

    }
};
