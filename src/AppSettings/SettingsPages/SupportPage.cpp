#include "Tools/Event.hpp"
#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "SupportPage.hpp"
#include "Tools/Paths.hpp"


namespace AnyFSE::App::AppSettings::Settings::Page
{
    void SupportPage::AddPage(std::list<SettingsLine>& settingPageList, ULONG &top)
    {
        m_dialog.AddSettingsLine(settingPageList, top,
            L"Related support",
            L"",
            Layout::LineHeightSmall, 0, 0
        ).SetState(FluentDesign::SettingsLine::Caption);

        SettingsLine &support = m_dialog.AddSettingsLine(settingPageList, top,
            L"Support and community",
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
        links.AddLinkButton(L"Discord AnyFSE community channel", L"https://discord.gg/AfkERzTEut");
        links.AddLinkButton(L"GitHub repository", L"https://github.com/ashpynov/AnyFSE/");
        links.AddLinkButton(L"Report issue or feature request", L"https://github.com/ashpynov/AnyFSE/issues");
        links.AddLinkButton(L"Navigate to log files folder", Tools::Paths::GetDataPath() + L"\\logs");

        support.SetState(FluentDesign::SettingsLine::Opened);

    }
};