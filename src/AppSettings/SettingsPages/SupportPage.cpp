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
            Layout::LineHeightSmall, 0, 0
        ).SetState(FluentDesign::SettingsLine::Caption);

        SettingsLine &support = m_dialog.AddSettingsLine(settingPageList, top,
            Translate(L"settingsSupportAndCommunity"),
            L"",
            Layout::LineHeight, 0, 0);

        support.SetIcon(L'\xF6FA');
        support.OnChanged += delegateparam(m_dialog.UpdateLine, &support);

        SettingsLine &links = m_dialog.AddSettingsLine(settingPageList, top,
            L"",
            L"",
            Layout::LineHeightSmall, Layout::LinePadding, Layout::LineSmallMargin);

        support.AddGroupItem(&links);

        // links.SetMaxColumns(1);
        links.AddLinkButton(Translate(L"settingsDiscordCommunity"), L"https://discord.gg/hnVwuTzDmk");
        links.AddLinkButton(Translate(L"settingsNavigateLogsFolder"), Tools::Paths::GetLogsPath());

        links.AddLinkButton(Translate(L"settingsSourceCodeGithub"), L"https://github.com/ashpynov/AnyFSE/");
        links.AddLinkButton(Translate(L"settingsNavigateConfigFolder"), Tools::Paths::GetConfigPath());

        links.AddLinkButton(Translate(L"settingsSourceCodeCodeberg"), L"https://codeberg.org/ashpynov/AnyFSE/");
        links.AddLinkButton(Translate(L"settingsNavigateSplashFolder"), Tools::Paths::GetSplashDefaultPath());

        links.AddLinkButton(Translate(L"settingsReportIssueGithub"), L"https://github.com/ashpynov/AnyFSE/issues");
        links.AddLinkButton(L"", L"");

        links.AddLinkButton(Translate(L"settingsReportIssueCodeberg"), L"https://codeberg.org/ashpynov/AnyFSE/issues");
        links.AddLinkButton(L"", L"");

        support.SetState(FluentDesign::SettingsLine::Opened);

    }
};
