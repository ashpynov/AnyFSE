#pragma once

#include <list>
#include "Configuration/Config.hpp"
#include "FluentDesign/Theme.hpp"
#include "FluentDesign/ComboBox.hpp"
#include "FluentDesign/TextBox.hpp"
#include "FluentDesign/Toggle.hpp"
#include "AppSettings/SettingsPages/SettingsPage.hpp"
#include "AppSettings/SettingsPages/SplashPage.hpp"
#include "AppSettings/SettingsPages/StartupPage.hpp"

namespace AnyFSE::App::AppSettings::Settings::Page
{
    using namespace FluentDesign;

    class LauncherPage : public SettingsPage
    {
        Theme &m_theme;
        SettingsDialog &m_dialog;

    public:
        LauncherPage(Theme& theme, SettingsDialog &dialog)
            : m_theme(theme)
            , m_dialog(dialog)
            , m_launcherCombo(m_theme)
            , m_browseButton(m_theme)
            , m_fseOnStartupToggle(m_theme)
            , m_fseExitOnHomeExitToggle(m_theme)
            , m_customSettingsToggle(m_theme)
            , m_additionalArgumentsEdit(m_theme)
            , m_processNameEdit(m_theme)
            , m_titleEdit(m_theme)
            , m_classEdit(m_theme)
            , m_processNameAltEdit(m_theme)
            , m_titleAltEdit(m_theme)
            , m_classAltEdit(m_theme)
            , m_customResetButton(m_theme)
            , m_customSettingsState(SettingsLine::Closed)
            , m_pSplashPage(new Page::SplashPage(m_theme, m_dialog))
            , m_pStartupPage(new Page::StartupPage(m_theme, m_dialog))

        {}

        std::list<SettingsLine> &GetSettingsLines() { return m_pageLinesList;  };

        void AddPage(std::list<SettingsLine>& settingPageList, ULONG &top);
        void LoadControls();
        void SaveControls();

    private:
        std::list<SettingsLine> m_pageLinesList;

        bool m_isCustom = false;
        bool m_isAggressive = false;
        bool m_enterOnStartup = false;

        LauncherConfig m_config;
        LauncherConfig m_defaultConfig;
        std::wstring m_currentLauncherPath;
        std::list<std::wstring> m_launchersList;
        std::list<std::wstring> m_notInstalledLaunchersList;


        ComboBox m_launcherCombo;
        Button m_browseButton;
        Toggle m_fseOnStartupToggle;
        Toggle m_fseExitOnHomeExitToggle;
        Toggle m_customSettingsToggle;
        Button m_customResetButton;

        TextBox m_additionalArgumentsEdit;
        TextBox m_processNameEdit;
        TextBox m_titleEdit;
        TextBox m_classEdit;
        TextBox m_processNameAltEdit;
        TextBox m_titleAltEdit;
        TextBox m_classAltEdit;

        SettingsLine * m_pBrowseLine = nullptr;
        SettingsLine * m_pFseOnStartupLine = nullptr;
        SettingsLine * m_pExitOnHomeExitLine = nullptr;
        SettingsLine * m_pCustomSettingsLine = nullptr;
        SettingsLine * m_pStartupSettingsLine = nullptr;
        SettingsLine * m_pSplashSettingsLine = nullptr;

        Page::SplashPage *m_pSplashPage = nullptr;
        Page::StartupPage *m_pStartupPage = nullptr;

        SettingsLine::State m_customSettingsState;

        void AddCustomPage();

        void OnBrowseLauncher();
        void OnCustomChanged();
        void OnCustomReset();
        void UpdateCustomResetEnabled();
        void OpenCustomSettingsPage();
        void OpenSplashSettingsPage();
        void OpenStartupSettingsPage();
        void UpdateControls();
        void OnLauncherDropDown();
        void OnLauncherChanged();
        void UpdateCombo();
        void UpdateCustomSettings();
    };
};