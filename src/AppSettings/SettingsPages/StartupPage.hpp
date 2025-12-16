#pragma once

#include <list>
#include "FluentDesign/Theme.hpp"
#include "FluentDesign/Button.hpp"
#include "FluentDesign/Toggle.hpp"
#include "AppSettings/SettingsPages/SettingsPage.hpp"

namespace AnyFSE::App::AppSettings::Settings::Page
{
    using namespace FluentDesign;

    class StartupPage : public SettingsPage
    {
        Theme &m_theme;
        SettingsDialog &m_dialog;

    public:
        StartupPage(Theme& theme, SettingsDialog &dialog)
            : m_theme(theme)
            , m_dialog(dialog)
            , m_startupAddButton(m_theme)
        {}

        std::list<SettingsLine> &GetSettingsLines() { return m_pageLinesList;  };

        void AddPage(std::list<SettingsLine>& settingPageList, ULONG &top);
        void LoadControls();
        void SaveControls();

    private:
        std::list<SettingsLine> m_pageLinesList;

        SettingsLine * m_pStartupPageLine = nullptr;
        SettingsLine * m_pStartupPageAppsHeader = nullptr;

        Button                      m_startupAddButton;
        std::list<Toggle>           m_startupToggles;
        std::list<SettingsLine *>   m_pStartupAppLines;

        void OpenStartupSettingsPage();
        void OpenMSSettingsStartupApps();

        void AddStartupAppLine(const std::wstring &path, const std::wstring &args, bool enabled);
        Toggle *GetStartupLineToggle(SettingsLine *pLine);
        void SetStartupAppLine(SettingsLine *pLine, const std::wstring &path, const std::wstring &args);
        void OnStartupAdd();
        void OnStartupModify(SettingsLine *pLine);
        void OnStartupDelete(SettingsLine *pLine);

    };
};