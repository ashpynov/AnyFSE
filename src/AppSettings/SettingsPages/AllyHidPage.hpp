#pragma once

#include <list>
#include "FluentDesign/Theme.hpp"
#include "FluentDesign/Toggle.hpp"
#include "FluentDesign/ComboBox.hpp"
#include "AppSettings/SettingsPages/SettingsPage.hpp"

namespace AnyFSE::App::AppSettings::Settings::Page
{
    using namespace FluentDesign;

    class AllyHidPage : public SettingsPage
    {
        Theme &m_theme;
        SettingsDialog &m_dialog;

    public:
        AllyHidPage(Theme& theme, SettingsDialog &dialog)
            : m_theme(theme)
            , m_dialog(dialog)
            , m_enableAllyHidToggle(theme)
            , m_acPressCombo(theme)
            , m_acHoldCombo(theme)
            , m_ccPressCombo(theme)
            , m_modeACPressCombo(theme)
            , m_modeACHoldCombo(theme)
            , m_modeCCPressCombo(theme)
        {}
        std::list<SettingsLine> &GetSettingsLines() { return m_pageLinesList;  };

        void AddPage(std::list<SettingsLine>& settingPageList, ULONG &top);
        void LoadControls();
        void SaveControls();

    private:
        std::list<SettingsLine> m_pageLinesList;

        Toggle m_enableAllyHidToggle;
        ComboBox m_acPressCombo;
        ComboBox m_acHoldCombo;
        ComboBox m_ccPressCombo;
        ComboBox m_modeACPressCombo;
        ComboBox m_modeACHoldCombo;
        ComboBox m_modeCCPressCombo;

        SettingsLine * m_pAllyHidLine;
        SettingsLine * m_pACPressLine;
        SettingsLine * m_pACHoldLine;
        SettingsLine * m_pCCPressLine;
        SettingsLine * m_pModeACPressLine;
        SettingsLine * m_pModeACHoldLine;
        SettingsLine * m_pModeCCPressLine;

        void OpenAllyHidPage();
        void EnableAllyHidChanged();

        void ReloadIcons();
    };
}
