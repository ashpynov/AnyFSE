#pragma once

#include <list>
#include "FluentDesign/Theme.hpp"
#include "FluentDesign/ComboBox.hpp"
#include "FluentDesign/Toggle.hpp"
#include "AppSettings/SettingsPages/SettingsPage.hpp"

namespace AnyFSE::App::AppSettings::Settings::Page
{
    using namespace FluentDesign;

    class TroubleshootPage : public SettingsPage
    {
        Theme &m_theme;
        SettingsDialog &m_dialog;

    public:
        TroubleshootPage(Theme& theme, SettingsDialog &dialog)
            : m_theme(theme)
            , m_dialog(dialog)
            , m_troubleLogLevelCombo(m_theme)
            , m_troubleAggressiveToggle(m_theme)
            , m_troubleExitOnExitToggle(m_theme)
        {}

        std::list<SettingsLine> &GetSettingsLines() { return m_pageLinesList;  };

        void AddPage(std::list<SettingsLine>& settingPageList, ULONG &top);
        void LoadControls();
        void SaveControls();

    private:
        std::list<SettingsLine> m_pageLinesList;

        void OpenTroubleshootSettingsPage();
        void OnGotoLogsFolder();

        ComboBox m_troubleLogLevelCombo;
        Toggle m_troubleAggressiveToggle;
        Toggle m_troubleExitOnExitToggle;
    };
};