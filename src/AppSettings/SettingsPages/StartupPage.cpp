#include "Tools/Event.hpp"
#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "AppSettings/SettingsPages/StartupEditDlg.hpp"
#include "StartupPage.hpp"


namespace AnyFSE::App::AppSettings::Settings::Page
{
    void StartupPage::AddPage(std::list<SettingsLine>& settingPageList, ULONG &top)
    {
        SettingsLine &startupPageLine = m_dialog.AddSettingsLine(settingPageList, top,
            L"Startup",
            L"Apps that start automatically when you sign in full screen experience",
            Layout::LineHeight, Layout::LinePadding, 0);

        startupPageLine.SetState(FluentDesign::SettingsLine::Next);
        startupPageLine.SetIcon(L'\xE18C');
        startupPageLine.OnChanged += delegate(OpenStartupSettingsPage);

        ULONG pageTop = 0;
        SettingsLine &startupAppLinkLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Native startup settings",
            L"Configure startup apps using native windows settings",
            Layout::LineHeight, Layout::LinePadding, 0);

        startupAppLinkLine.SetState(FluentDesign::SettingsLine::Link);
        startupAppLinkLine.SetIcon(L'\xE18C');
        startupAppLinkLine.OnChanged += delegate(OpenMSSettingsStartupApps);

        m_pStartupPageAppsHeader = &m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Additional startup applications",
            L"Configure specific applications to be executed by AnyFSE on fullscreen experience enter",
            Layout::LineHeight, 0, 0
        );

        m_pStartupPageAppsHeader->SetState(FluentDesign::SettingsLine::Caption);

        SettingsLine & line = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"",
            L"",
            m_startupAddButton,
            Layout::LineHeightSmall, Layout::LinePadding, Layout::LineSmallMargin,
            Layout::StartupAddWidth, Layout::StartupAddHeight
        );

        m_startupAddButton.SetText(L"Add");
        m_startupAddButton.OnChanged = delegate(OnStartupAdd);
        line.SetState(FluentDesign::SettingsLine::Caption);
    }

    void StartupPage::LoadControls()
    {
        for (auto app : Config::StartupApps)
        {
            AddStartupAppLine(app.Path, app.Args, app.Enabled);
        }
    }

    void StartupPage::SaveControls()
    {
        Config::StartupApps.clear();

        for (auto pLine : m_pStartupAppLines)
        {
            Toggle *toggle = GetStartupLineToggle(pLine);

            Config::StartupApps.push_back(
                StartupApp {
                    pLine->GetData(0),
                    pLine->GetData(1),
                    toggle ? toggle->GetCheck() : true
                }
            );
        }
    }


    void StartupPage::OpenStartupSettingsPage()
    {
        m_dialog.SwitchActivePage(L"Startup", &m_pageLinesList);
    }

    void StartupPage::OpenMSSettingsStartupApps()
    {
        Process::StartProtocol(L"ms-settings:startupapps");
    }

    void StartupPage::AddStartupAppLine(const std::wstring& path, const std::wstring& args, bool enabled )
    {
        ULONG top = 0;
        m_startupToggles.emplace_back(m_theme);
        Toggle &startToggle = m_startupToggles.back();

        SettingsLine & line = m_dialog.AddSettingsLine(m_pageLinesList, top,
            L"",
            L"",
            startToggle,
            Layout::LineHeight, Layout::LinePaddingSmall, 0,
            Layout::StartupMenuButtonWidth, Layout::StartupMenuButtonHeight
        );

        SetStartupAppLine(&line, path, args);

        line.SetMenu(
            std::vector<FluentDesign::Popup::PopupItem>
            {
                FluentDesign::Popup::PopupItem(L"\xE13E", L"Modify",[This = this, pLine = &line](){This->OnStartupModify(pLine);}),
                FluentDesign::Popup::PopupItem(L"\xE107", L"Delete",[This = this, pLine = &line](){This->OnStartupDelete(pLine);})
            }
        );
        startToggle.SetCheck(enabled);
        m_pStartupPageAppsHeader->AddGroupItem(&line);

        m_pStartupAppLines.push_back(&line);
    }

    Toggle * StartupPage::GetStartupLineToggle(SettingsLine * pLine)
    {
        auto it = std::find_if(m_startupToggles.begin(), m_startupToggles.end(),
            [pLine](const Toggle& obj)
            {
                return obj.GetHwnd() == pLine->GetChildControl();
            }
        );

        return it != m_startupToggles.end() ? &(*it) : nullptr;
    }

    void StartupPage::SetStartupAppLine(SettingsLine *pLine, const std::wstring& path, const std::wstring& args)
    {
        pLine->SetData(0, path);
        pLine->SetData(1, args);


        pLine->SetName(Config::GetApplicationName(path));
        pLine->SetDescription(path + L" " + args);

        if (!path.empty())
        {
            pLine->SetIcon(path);
        }
    }

    void StartupPage::OnStartupAdd()
    {
        OnStartupModify(NULL);
    }

    void StartupPage::OnStartupModify(SettingsLine * pLine)
    {
        std::wstring path = pLine ? pLine->GetData(0) : L"";
        std::wstring args = pLine ? pLine->GetData(1) : L"";
        if (IDOK == StartupEditDlg::EditApp(m_dialog.GetHwnd(), path, args))
        {
            pLine ? SetStartupAppLine(pLine, path, args) : AddStartupAppLine(path, args, true);
            m_dialog.UpdateLayout();
        }
    }

    void StartupPage::OnStartupDelete(SettingsLine * pLine)
    {
        if (pLine->GetGroupHeader())
        {
            pLine->GetGroupHeader()->DeleteGroupItem(pLine);
        }
        m_startupToggles.remove_if([pLine](const Toggle& obj) {return obj.GetHwnd() == pLine->GetChildControl();});
        m_pStartupAppLines.remove_if([pLine](const SettingsLine* obj) {return obj == pLine;});
        m_pageLinesList.remove_if([pLine](const SettingsLine& obj) {return &obj == pLine;});
        m_dialog.UpdateLayout();
    }

};