#include <filesystem>
#include <windows.h>
#include "Tools/Registry.hpp"
#include "Tools/Elevated.hpp"
#include "App/Constants.hpp"
#include "App/GamingExperience.hpp"
#include "Tools/Event.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/List.hpp"
#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "AppSettings/SettingsPages/ConfirmationsPage.hpp"
#include "LauncherPage.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/Localization.hpp"

namespace c = AnyFSE::App::Constants;

namespace AnyFSE::App::AppSettings::Settings::Page
{
    static Logger log = LogManager::GetLogger("Settings/Launcher");

    void LauncherPage::AddPage(std::list<SettingsLine>& settingPageList, ULONG &top)
    {
        m_pHomeAppSelectionLine = &m_dialog.AddSettingsLine(settingPageList, top,
            Translate(L"settingsHomeAppSelectionUnavailable"),
            Translate(L"settingsHomeAppSelectionUnavailableDescription"),
            m_enableHomeAppSelectionButton,
            Layout::LineHeight, Layout::LinePadding, 0,
            Layout::CustomSettingsWidth, Layout::BrowseHeight);
        m_pHomeAppSelectionLine->SetIcon(L'\xE7BA');
        m_pHomeAppSelectionLine->Show(false);

        m_pLauncherLine = &m_dialog.AddSettingsLine(settingPageList, top,
            Translate(L"settingsChooseHomeApp"),
            Translate(L"settingsChooseHomeAppDescription"),
            m_launcherCombo,
            Layout::LineHeight, Layout::LauncherBrowsePadding, 0,
            Layout::LauncherComboWidth );

        m_pLauncherLine->SetFrame(Gdiplus::FrameFlags::SIDE_NO_BOTTOM | Gdiplus::FrameFlags::CORNER_TOP);

        m_pBrowseLine = &m_dialog.AddSettingsLine(settingPageList, top,
            L"",
            L"",
            m_browseButton,
            Layout::LauncherBrowseLineHeight, Layout::LinePadding, 0,
            Layout::BrowseWidth, Layout::BrowseHeight);

        m_pBrowseLine->SetFrame(Gdiplus::FrameFlags::SIDE_NO_TOP | Gdiplus::FrameFlags::CORNER_BOTTOM);

        m_pFseOnStartupLine = &m_dialog.AddSettingsLine(settingPageList, top,
            Translate(L"settingsEnterFseOnStartup"),
            L"",
            m_fseOnStartupToggle,
            Layout::LineHeight, Layout::LinePadding, 0);
        m_pFseOnStartupLine->SetIcon(L'\xE93A');

        m_pExitOnHomeExitLine = &m_dialog.AddSettingsLine(settingPageList, top,
            Translate(L"settingsLeaveFseOnHomeExit"),
            Translate(L"settingsLeaveFseOnHomeExitDescription"),
            m_fseExitOnHomeExitToggle,
            Layout::LineHeight, Layout::LinePadding, 0);
        m_pExitOnHomeExitLine->SetIcon(L'\xEE47');

        m_dialog.AddPage((new ConfirmationsPage(m_theme, m_dialog))->AddLine(settingPageList, top));

        m_pCustomSettingsLine = &m_dialog.AddSettingsLine(settingPageList, top,
            Translate(L"settingsUseCustomSettings"),
            Translate(L"settingsUseCustomSettingsDescription"),
            m_customSettingsToggle,
            Layout::LineHeight, Layout::LinePadding, 0);

        m_pCustomSettingsLine->SetState(FluentDesign::SettingsLine::Next);
        m_pCustomSettingsLine->SetIcon(L'\xE115');
        m_pCustomSettingsLine->OnChanged += delegate(OpenCustomSettingsPage);

        AddCustomPage();

        m_pSplashSettingsLine = &m_dialog.AddSettingsLine(settingPageList, top,
            Translate(L"settingsSplashScreenSettings"),
            Translate(L"settingsSplashScreenSettingsDescription"),
            Layout::LineHeight, Layout::LinePadding, 0);

        m_pSplashSettingsLine->SetState(SettingsLine::Next);
        m_pSplashSettingsLine->SetIcon(L'\xEB9F');
        m_pSplashSettingsLine->OnChanged += delegate(OpenSplashSettingsPage);

        m_pStartupSettingsLine = &m_dialog.AddSettingsLine(settingPageList, top,
            Translate(L"settingsStartup"),
            Translate(L"settingsStartupDescription"),
            Layout::LineHeight, Layout::LinePadding, 0);

        m_pStartupSettingsLine->SetState(FluentDesign::SettingsLine::Next);
        m_pStartupSettingsLine->SetIcon(L'\xE18C');
        m_pStartupSettingsLine->OnChanged += delegate(OpenStartupSettingsPage);

        m_dialog.AddPage(m_pSplashPage);
        m_dialog.AddPage(m_pStartupPage);

        m_launcherCombo.OnChanged += delegate(OnLauncherChanged);
        m_launcherCombo.OnDropDown += delegate(OnLauncherDropDown);

        m_customSettingsToggle.OnChanged += delegate(OnCustomChanged);

        m_browseButton.SetText(Translate(L"browseBtn"));
        m_browseButton.OnChanged += delegate(OnBrowseLauncher);

        m_enableHomeAppSelectionButton.SetText(
            Translate(L"settingsEnableHomeAppSelection"));
        m_enableHomeAppSelectionButton.OnChanged += delegate(OnEnableHomeAppSelection);
    }

    void LauncherPage::AddCustomPage()
    {

        /// Custom Settings Page part
        ULONG pageTop = 0;

        SettingsLine & parametersSettingsLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            Translate(L"settingsAdditionalArguments"),
            Translate(L"settingsAdditionalArgumentsDescription"),
            m_additionalArgumentsEdit,
            Layout::LineHeight, Layout::LinePadding, 0);

        parametersSettingsLine.SetIcon(L'\xE62F');

        SettingsLine & primarySettingsLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            Translate(L"settingsHomeApplicationDetection"),
            Translate(L"settingsHomeApplicationDetectionDescription"),
            Layout::LineHeightSmall, 0, 0);


        primarySettingsLine.SetState(SettingsLine::State::Closed);
        primarySettingsLine.OnChanged += delegate(m_dialog.UpdateLayout);
        primarySettingsLine.SetIcon(L'\xF8A5');

        primarySettingsLine.AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            Translate(L"settingsProcessName"),
            Translate(L"settingsProcessNameDescription"),
            m_processNameEdit,
            Layout::LineHeightSmall, 0, Layout::LineSmallMargin));

        primarySettingsLine.AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            Translate(L"settingsWindowClassName"),
            Translate(L"settingsWindowClassNameDescription"),
            m_classEdit,
            Layout::LineHeightSmall, 0, Layout::LineSmallMargin));

        primarySettingsLine.AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            Translate(L"settingsFieldWindowTitle"),
            Translate(L"settingsWindowTitleDescription"),
            m_titleEdit,
            Layout::LineHeightSmall, Layout::LinePadding, Layout::LineSmallMargin));

        SettingsLine & secondarySettingsLine = m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            Translate(L"settingsAlternativeModeDetection"),
            Translate(L"settingsAlternativeModeDetectionDescription"),
            Layout::LineHeightSmall, 0, 0);

        secondarySettingsLine.SetState(SettingsLine::State::Closed);
        secondarySettingsLine.OnChanged += delegate(m_dialog.UpdateLayout);
        secondarySettingsLine.SetIcon(L'\xE737');

        secondarySettingsLine.AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            Translate(L"settingsSecondaryProcessName"),
            Translate(L"settingsSecondaryProcessNameDescription"),
            m_processNameAltEdit,
            Layout::LineHeightSmall, 0, Layout::LineSmallMargin));

        secondarySettingsLine.AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            Translate(L"settingsSecondaryWindowClassName"),
            Translate(L"settingsSecondaryWindowClassNameDescription"),
            m_classAltEdit,
            Layout::LineHeightSmall, 0, Layout::LineSmallMargin));

        secondarySettingsLine.AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            Translate(L"settingsSecondaryWindowTitle"),
            Translate(L"settingsSecondaryWindowTitleDescription"),
            m_titleAltEdit,
            Layout::LineHeightSmall, 0, Layout::LineSmallMargin));

        m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"",
            L"",
            m_customResetButton,
            Layout::LineHeightSmall, Layout::LinePadding, Layout::LineSmallMargin,
            Layout::StartupAddWidth, Layout::StartupAddHeight
        ).SetState(FluentDesign::SettingsLine::Caption);

        m_customResetButton.SetText(Translate(L"resetBtn"));
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
        m_currentLauncherPath = Config::GetNativePath(Config::Launcher.StartCommand);
        Config::FindLaunchers(m_launchersList);
        Config::FindNotInstalledLaunchers(m_notInstalledLaunchersList);
        Config::LoadLauncherSettings(m_currentLauncherPath, m_config);
        UpdateCombo();

        bool customSettings = Config::CustomSettings;
        m_customSettingsState = customSettings ? FluentDesign::SettingsLine::Next: FluentDesign::SettingsLine::Normal;

        m_isCustom = (customSettings || m_config.IsCustom) && (
            m_config.AppUserModelID.empty()
        ) || m_config.Type == LauncherType::Custom;

        m_isAggressive = Config::AggressiveMode && m_config.Type != LauncherType::Native;

        m_dialog.UpdateLayout();
        UpdateControls();
        UpdateCustomSettings();
        UpdateHomeAppSelection();
    }

    void LauncherPage::SaveControls()
    {
        const std::wstring gamingConfiguration = L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration";
        const std::wstring gamingHomeApp = c::GamingHomeAppRegValue;
        //const std::wstring xboxApp = L"Microsoft.GamingApp_8wekyb3d8bbwe!Microsoft.Xbox.App";
        const std::wstring anyFSEApp = c::AppUserModelId;
        const bool fseOnStartup = m_fseOnStartupToggle.GetCheck();

        if (m_config.Type == LauncherType::None)
        {
            Registry::DeleteValue(gamingConfiguration, gamingHomeApp);
            Registry::WriteBool(gamingConfiguration, c::StartupToGamingHomeRegValue, false);
        }
        else if (m_config.Type == LauncherType::Native)
        {
            log.Debug("Saving %s as launcher", Unicode::to_string(m_config.Name).c_str());
            Registry::WriteBool(gamingConfiguration, c::StartupToGamingHomeRegValue, fseOnStartup);
            Registry::WriteString(gamingConfiguration, gamingHomeApp, m_config.AppUserModelID);
        }
        else
        {
            log.Debug("Saving AnyFSE as launcher");
            Registry::WriteBool(gamingConfiguration, c::StartupToGamingHomeRegValue, fseOnStartup);
            Registry::WriteString(gamingConfiguration, gamingHomeApp, anyFSEApp);
        }

        std::wstring savedLauncher = Registry::ReadString(gamingConfiguration, gamingHomeApp);
        log.Debug("Saved launcher: %s", Unicode::to_string(savedLauncher).c_str());

        Config::Launcher.Type = m_config.Type;
        Config::Launcher.StartCommand = m_config.StartCommand;
        Config::ExitFSEOnHomeExit = m_fseExitOnHomeExitToggle.GetCheck();
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

    void LauncherPage::OnRestoreGamingPC()
    {
        Elevated::Call(c::ElevatedRestoreGamingPC);
        UpdateHomeAppSelection();
    }

    void LauncherPage::OnEnableHomeAppSelection()
    {
        m_enableHomeAppSelectionButton.Enable(false);
        Elevated::Call(c::ElevatedEnableGamingHandheld);
        m_enableHomeAppSelectionButton.Enable(true);
        UpdateHomeAppSelection();
    }

    void LauncherPage::UpdateHomeAppSelection()
    {
        const bool available = GamingExperience::IsGamingHandheld();
        m_pHomeAppSelectionLine->Show(!available);
        m_pLauncherLine->Show(available);
        m_pBrowseLine->Show(available);
        m_pFseOnStartupLine->Show(available);
        m_pExitOnHomeExitLine->Show(available);
        m_pCustomSettingsLine->Show(available);
        m_pSplashSettingsLine->Show(available);
        m_pStartupSettingsLine->Show(available);

        UpdateRestoreGamingPC();
        m_dialog.UpdateLayout();
    }

    void LauncherPage::UpdateRestoreGamingPC()
    {
        if (Registry::ValueExists(c::DeviceFormRegKey, c::DeviceFormBackupRegValue))
        {
            m_pBrowseLine->OnLink = delegate(OnRestoreGamingPC);
            m_pBrowseLine->SetDescription(Translate(L"settingsRestoreXboxPcMode"));
        }
        else
        {
            m_pBrowseLine->OnLink.Clear();
            m_pBrowseLine->SetDescription(
                m_defaultConfig.Type == LauncherType::Native
                    ? Translate(L"settingsNativeLauncherSelected")
                    : L"");
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
        m_dialog.SwitchActivePage(Translate(L"settingsCustomSettings"), &m_pageLinesList);
    }

    void LauncherPage::OpenSplashSettingsPage()
    {
        m_dialog.SwitchActivePage(Translate(L"settingsSplashSettings"), &m_pSplashPage->GetSettingsLines());
    }

    void LauncherPage::OpenStartupSettingsPage()
    {
        m_dialog.SwitchActivePage(Translate(L"settingsStartup"), &m_pStartupPage->GetSettingsLines());
    }

    void LauncherPage::UpdateControls()
    {
        const auto defaultsPath = m_config.Type == LauncherType::Custom || m_config.Type == LauncherType::Native
            ? m_currentLauncherPath : std::filesystem::path(m_currentLauncherPath).parent_path().wstring();
        Config::GetLauncherDefaults(m_config.Type, defaultsPath, m_defaultConfig);

        if (m_defaultConfig.Type == LauncherType::None)
        {
            m_pFseOnStartupLine->Enable(false);
        }
        else
        {
            m_pFseOnStartupLine->Enable();
            m_pExitOnHomeExitLine->Enable();
        }
        m_fseOnStartupToggle.SetCheck(Config::FseOnStartup);
        m_fseExitOnHomeExitToggle.SetCheck(Config::ExitFSEOnHomeExit);

        bool alwaysSettings = m_defaultConfig.Type==LauncherType::Custom;
        bool noSettings =
               !m_defaultConfig.AppUserModelID.empty()
            || m_defaultConfig.Type == LauncherType::None;

        bool enabledAnyFSE =
               m_defaultConfig.Type != LauncherType::None
            && m_defaultConfig.Type != LauncherType::Native;

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
            m_pCustomSettingsLine->Enable(enabledAnyFSE && haveSettings);
            EnableWindow(m_pCustomSettingsLine->GetChildControl(), enableCheck);
            m_pCustomSettingsLine->Invalidate();
        }
        else
        {
            m_pCustomSettingsLine->Enable(enabledAnyFSE && enableCheck);
        }

        m_pExitOnHomeExitLine->Enable(enabledAnyFSE);
        m_pSplashSettingsLine->Enable(enabledAnyFSE);
        m_pStartupSettingsLine->Enable(enabledAnyFSE);

        if (!haveSettings)
        {
            m_config = m_defaultConfig;
            UpdateCustomSettings();
        }

        UpdateRestoreGamingPC();
    }

    void LauncherPage::OnLauncherDropDown()
    {
        std::list<LauncherConfig> notInstalled;
        Config::FindNotInstalledLaunchers(notInstalled);

        if (notInstalled.size() != m_notInstalledLaunchersList.size())
        {
            m_launchersList.clear();
            m_notInstalledLaunchersList.clear();
            Config::FindLaunchers(m_launchersList);
            Config::FindNotInstalledLaunchers(m_notInstalledLaunchersList);
            UpdateCombo();
        }
    }

    void LauncherPage::OnLauncherChanged()
    {
        const int index = m_launcherCombo.GetSelectedIndex();
        if (index < 0 || static_cast<size_t>(index) >= m_launcherChoices.size())
            return;

        const auto selected = m_launcherChoices[index];
        if (!selected.Installed)
        {
            Process::StartProtocol(selected.Launcher.URL);
            UpdateCombo();
            return;
        }

        m_currentLauncherPath = selected.Launcher.StartCommand;
        Config::LoadLauncherSettings(m_currentLauncherPath, m_config, selected.Launcher.Type);
        UpdateControls();
        UpdateCustomSettings();
    }

    void LauncherPage::UpdateCombo()
    {
        m_launcherCombo.Reset();
        m_launcherChoices.clear();
        size_t selectedIndex = List::npos;
        for (const auto& launcher : m_launchersList)
        {
            if (launcher.Type == m_config.Type
                && Unicode::to_lower(launcher.StartCommand) == Unicode::to_lower(m_currentLauncherPath))
                selectedIndex = m_launcherChoices.size();
            m_launcherChoices.push_back({launcher, true});
        }

        if (selectedIndex == List::npos)
        {
            const auto defaultsPath = m_config.Type == LauncherType::Custom || m_config.Type == LauncherType::Native
                ? m_currentLauncherPath
                : std::filesystem::path(m_currentLauncherPath).parent_path().wstring();

            LauncherConfig launcher = Config::GetLauncherDefaults(m_config.Type, defaultsPath);
            Config::UpdatePortableLauncher(launcher);
            selectedIndex = m_launcherChoices.empty() ? 0 : 1;
            m_launcherChoices.insert(m_launcherChoices.begin() + selectedIndex, {launcher, true});
        }

        for (const auto& launcher : m_notInstalledLaunchersList)
            m_launcherChoices.push_back({launcher, false});

        for (const auto& choice : m_launcherChoices)
        {
            const auto& launcher = choice.Launcher;
            m_launcherCombo.AddItem(launcher.Name, choice.Installed ? launcher.IconFile : L"\xE118", L"");
        }
        m_launcherCombo.SelectItem(static_cast<int>(selectedIndex));
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
