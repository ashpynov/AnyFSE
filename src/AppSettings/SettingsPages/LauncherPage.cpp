#include <filesystem>
#include "Tools/Registry.hpp"
#include "Tools/Event.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/List.hpp"
#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "LauncherPage.hpp"


namespace AnyFSE::App::AppSettings::Settings::Page
{
    void LauncherPage::AddPage(std::list<SettingsLine>& settingPageList, ULONG &top)
    {
        FluentDesign::SettingsLine &launcher = m_dialog.AddSettingsLine(settingPageList, top,
            L"Choose home app",
            L"Choose home application for full screen experience",
            m_launcherCombo,
            Layout::LineHeight, Layout::LauncherBrowsePadding, 0,
            Layout::LauncherComboWidth );

        launcher.SetFrame(Gdiplus::FrameFlags::SIDE_NO_BOTTOM | Gdiplus::FrameFlags::CORNER_TOP);

        FluentDesign::SettingsLine &browse = m_dialog.AddSettingsLine(settingPageList, top,
            L"",
            L"",
            m_browseButton,
            Layout::LauncherBrowseLineHeight, Layout::LinePadding, 0,
            Layout::BrowseWidth, Layout::BrowseHeight);

        browse.SetFrame(Gdiplus::FrameFlags::SIDE_NO_TOP | Gdiplus::FrameFlags::CORNER_BOTTOM);

        m_pFseOnStartupLine = &m_dialog.AddSettingsLine(settingPageList, top,
            L"Enter full screen experience on startup",
            L"",
            m_fseOnStartupToggle,
            Layout::LineHeight, Layout::LinePadding, 0);

        m_pCustomSettingsLine = &m_dialog.AddSettingsLine(settingPageList, top,
            L"Use custom settings",
            L"Change monitoring and startups settings for selected home application",
            m_customSettingsToggle,
            Layout::LineHeight, Layout::LinePadding, 0);

        m_pCustomSettingsLine->SetState(FluentDesign::SettingsLine::Next);
        m_pCustomSettingsLine->SetIcon(L'\xE115');
        m_pCustomSettingsLine->OnChanged += delegate(OpenCustomSettingsPage);

        AddCustomPage();

        m_launcherCombo.OnChanged += delegate(OnLauncherChanged);

        m_customSettingsToggle.OnChanged += delegate(OnCustomChanged);

        m_browseButton.SetText(L"Browse");
        m_browseButton.OnChanged += delegate(OnBrowseLauncher);
    }

    void LauncherPage::AddCustomPage()
    {

        /// Custom Settings Page part
        ULONG pageTop = 0;

        SettingsLine & parametersSettingsLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Additional arguments",
            L"Command line arguments passed to application",
            m_additionalArgumentsEdit,
            Layout::LineHeight, Layout::LinePadding, 0);

        parametersSettingsLine.SetIcon(L'\xE62F');

        SettingsLine & primarySettingsLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Home application detection",
            L"Parameters to detect home application is started",
            Layout::LineHeightSmall, 0, 0);


        primarySettingsLine.SetState(SettingsLine::State::Closed);
        primarySettingsLine.OnChanged += delegate(m_dialog.UpdateLayout);
        primarySettingsLine.SetIcon(L'\xF8A5');

        primarySettingsLine.AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Process name",
            L"Name of home application process",
            m_processNameEdit,
            Layout::LineHeightSmall, 0, Layout::LineSmallMargin));

        primarySettingsLine.AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Window class name",
            L"Class of application main window",
            m_classEdit,
            Layout::LineHeightSmall, 0, Layout::LineSmallMargin));

        primarySettingsLine.AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Window title",
            L"Title of app window when it have been activated",
            m_titleEdit,
            Layout::LineHeightSmall, Layout::LinePadding, Layout::LineSmallMargin));

        SettingsLine & secondarySettingsLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Alternative mode detection",
            L"Parameters to detect home application is started in alternative mode",
            Layout::LineHeightSmall, 0, 0);

        secondarySettingsLine.SetState(SettingsLine::State::Closed);
        secondarySettingsLine.OnChanged += delegate(m_dialog.UpdateLayout);
        secondarySettingsLine.SetIcon(L'\xE737');

        secondarySettingsLine.AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Secondary process name",
            L"Name of app process for alternative mode",
            m_processNameAltEdit,
            Layout::LineHeightSmall, 0, Layout::LineSmallMargin));

        secondarySettingsLine.AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Secondary window class name",
            L"Alternative Class name of app window",
            m_classAltEdit,
            Layout::LineHeightSmall, 0, Layout::LineSmallMargin));

        secondarySettingsLine.AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Secondary window title",
            L"Alternative Title of app window",
            m_titleAltEdit,
            Layout::LineHeightSmall, 0, Layout::LineSmallMargin));

        m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"",
            L"",
            m_customResetButton,
            Layout::LineHeightSmall, Layout::LinePadding, Layout::LineSmallMargin,
            Layout::StartupAddWidth, Layout::StartupAddHeight
        ).SetState(FluentDesign::SettingsLine::Caption);

        m_customResetButton.SetText(L"Reset");
        m_customResetButton.OnChanged = delegate(OnCustomReset);

        m_additionalArgumentsEdit.OnChanged += delegate(UpdateCustomResetEnabled);
        m_processNameEdit.OnChanged += delegate(UpdateCustomResetEnabled);
        m_processNameAltEdit.OnChanged += delegate(UpdateCustomResetEnabled);
        m_titleEdit.OnChanged += delegate(UpdateCustomResetEnabled);
        m_titleAltEdit.OnChanged += delegate(UpdateCustomResetEnabled);
        m_classEdit.OnChanged += delegate(UpdateCustomResetEnabled);
        m_classAltEdit.OnChanged += delegate(UpdateCustomResetEnabled);
    }

    void LauncherPage::LoadControls()
    {
        m_currentLauncherPath = Config::Launcher.StartCommand;
        Config::FindLaunchers(m_launchersList);
        UpdateCombo();
        Config::LoadLauncherSettings(m_currentLauncherPath, m_config);

        bool customSettings = Config::CustomSettings;
        m_customSettingsState = customSettings ? FluentDesign::SettingsLine::Next: FluentDesign::SettingsLine::Normal;

        m_isCustom = (customSettings || m_config.IsCustom) && (
            m_config.Type != LauncherType::Xbox
            && m_config.Type != LauncherType::ArmouryCrate
            && m_config.Type != LauncherType::OneGameLauncher
        ) || m_config.Type == LauncherType::Custom;

        m_isAggressive = Config::AggressiveMode && m_config.Type != LauncherType::Xbox;

        m_dialog.UpdateLayout();
        UpdateControls();
        UpdateCustomSettings();
    }

    void LauncherPage::SaveControls()
    {
        const std::wstring gamingConfiguration = L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration";
        const std::wstring startupToGamingHome = L"StartupToGamingHome";
        const std::wstring gamingHomeApp = L"GamingHomeApp";
        const std::wstring xboxApp = L"Microsoft.GamingApp_8wekyb3d8bbwe!Microsoft.Xbox.App";

        if (m_config.Type == LauncherType::None)
        {
            Registry::DeleteValue(gamingConfiguration, gamingHomeApp);
            Registry::WriteBool(gamingConfiguration, startupToGamingHome, false);
        }
        else
        {
            Registry::WriteBool(gamingConfiguration, startupToGamingHome, m_fseOnStartupToggle.GetCheck());
            Registry::WriteString(gamingConfiguration, gamingHomeApp, xboxApp);
        }

        Config::Launcher.StartCommand = m_config.StartCommand;
        Config::CustomSettings = m_customSettingsToggle.GetCheck();
        Config::CustomSettings = m_customSettingsToggle.GetCheck();
        Config::Launcher.StartArg = m_additionalArgumentsEdit.GetText();
        Config::Launcher.ProcessName = m_processNameEdit.GetText();
        Config::Launcher.WindowTitle = m_titleEdit.GetText();
        Config::Launcher.ProcessNameAlt = m_processNameAltEdit.GetText();
        Config::Launcher.WindowTitleAlt = m_titleAltEdit.GetText();
        Config::Launcher.ClassName = m_classEdit.GetText();
        Config::Launcher.ClassNameAlt = m_classAltEdit.GetText();
        Config::Launcher.IconFile = m_config.IconFile;
    }


    void LauncherPage::OnBrowseLauncher()
    {
        OPENFILENAME ofn = {};
        WCHAR szFile[MAX_PATH] = {};

        if (m_currentLauncherPath.find(L"://") == std::wstring::npos
            && std::filesystem::exists(m_currentLauncherPath))
        {
            wcsncpy_s(szFile, m_currentLauncherPath.c_str(), MAX_PATH);
        }

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_dialog.GetHwnd();
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = L"Launchers (*.exe)\0*.exe\0\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn))
        {
            if (m_currentLauncherPath != szFile)
            {
                m_currentLauncherPath = szFile;
                Config::LoadLauncherSettings(m_currentLauncherPath, m_config);
                UpdateCombo();
                UpdateControls();
                UpdateCustomSettings();
            }
        }
    }

    void LauncherPage::OnCustomChanged()
    {
        m_isCustom = m_customSettingsToggle.GetCheck();
        m_pCustomSettingsLine->SetState(m_customSettingsToggle.GetCheck()
            ? FluentDesign::SettingsLine::Next
            : FluentDesign::SettingsLine::Normal);
        m_dialog.UpdateLayout();
    }

    void LauncherPage::OnCustomReset()
    {
        m_config = m_defaultConfig;
        UpdateCustomSettings();
    }

    void LauncherPage::UpdateCustomResetEnabled()
    {
        bool bEnable =
               Unicode::to_lower(m_additionalArgumentsEdit.GetText()) != Unicode::to_lower(m_defaultConfig.StartArg)
            || Unicode::to_lower(m_processNameEdit.GetText()) != Unicode::to_lower(m_defaultConfig.ProcessName)
            || Unicode::to_lower(m_processNameAltEdit.GetText()) != Unicode::to_lower(m_defaultConfig.ProcessNameAlt)
            || Unicode::to_lower(m_titleEdit.GetText()) != Unicode::to_lower(m_defaultConfig.WindowTitle)
            || Unicode::to_lower(m_titleAltEdit.GetText()) != Unicode::to_lower(m_defaultConfig.WindowTitleAlt)
            || Unicode::to_lower(m_classEdit.GetText()) != Unicode::to_lower(m_defaultConfig.ClassName)
            || Unicode::to_lower(m_classAltEdit.GetText()) != Unicode::to_lower(m_defaultConfig.ClassNameAlt)
        ;
        m_customResetButton.Enable(bEnable);
    }

    void LauncherPage::OpenCustomSettingsPage()
    {
        m_dialog.SwitchActivePage(L"Custom settings", &m_pageLinesList);
    }

    void LauncherPage::UpdateControls()
    {
        Config::GetLauncherDefaults(m_currentLauncherPath, m_defaultConfig);

        if (m_defaultConfig.Type == LauncherType::None)
        {
            m_pFseOnStartupLine->Enable(false);
            m_fseOnStartupToggle.SetCheck(false);
        }
        else
        {
            m_pFseOnStartupLine->Enable();
            m_fseOnStartupToggle.SetCheck(Config::FseOnStartup);
        }

        bool alwaysSettings = m_defaultConfig.Type==LauncherType::Custom;
        bool noSettings =
                m_defaultConfig.Type == LauncherType::Xbox
            ||  m_defaultConfig.Type == LauncherType::ArmouryCrate
            || m_defaultConfig.Type == LauncherType::OneGameLauncher
            || m_defaultConfig.Type == LauncherType::None;

        bool haveSettings = m_isCustom && !noSettings || alwaysSettings;
        bool enableCheck = !alwaysSettings && !noSettings;

        m_customSettingsToggle.SetCheck(haveSettings);

        FluentDesign::SettingsLine::State state =
            haveSettings
                ? FluentDesign::SettingsLine::Next
                : FluentDesign::SettingsLine::Normal;

        m_pCustomSettingsLine->SetState(state);

        if (haveSettings && !enableCheck)
        {
            m_pCustomSettingsLine->Enable(haveSettings);
            EnableWindow(m_pCustomSettingsLine->GetChildControl(), enableCheck);
            m_pCustomSettingsLine->Invalidate();
        }
        else
        {
            m_pCustomSettingsLine->Enable(enableCheck);
        }


        if (!haveSettings)
        {
            m_config = m_defaultConfig;
            UpdateCustomSettings();
        }
    }

    void LauncherPage::OnLauncherChanged()
    {
        m_currentLauncherPath = m_launcherCombo.GetCurentValue();;
        Config::LoadLauncherSettings(m_currentLauncherPath, m_config);
        UpdateControls();
        UpdateCustomSettings();
    }

    void LauncherPage::UpdateCombo()
    {
        m_launcherCombo.Reset();

        for ( auto& launcher: m_launchersList)
        {
            LauncherConfig info;
            Config::GetLauncherDefaults(launcher, info);
            Config::UpdatePortableLauncher(info);
            m_launcherCombo.AddItem(info.Name, info.IconFile, info.StartCommand);
        }
        size_t index = List::index_of(m_launchersList, m_currentLauncherPath);

        if (index == List::npos)
        {
            LauncherConfig info;
            Config::GetLauncherDefaults(m_currentLauncherPath, info);
            Config::UpdatePortableLauncher(info);
            m_launcherCombo.AddItem(info.Name, info.IconFile, info.StartCommand, 1);
            index = 1;
        }

        m_launcherCombo.SelectItem((int)index);
    }

    void LauncherPage::UpdateCustomSettings()
    {
        // Set values
        m_additionalArgumentsEdit.SetText(m_config.StartArg);
        m_processNameEdit.SetText(m_config.ProcessName);
        m_processNameAltEdit.SetText(m_config.ProcessNameAlt);
        m_titleEdit.SetText(m_config.WindowTitle);
        m_titleAltEdit.SetText(m_config.WindowTitleAlt);
        m_classEdit.SetText(m_config.ClassName);
        m_classAltEdit.SetText(m_config.ClassNameAlt);

        UpdateCustomResetEnabled();
    }

};