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
            , m_libraryPressCombo(theme)
            , m_modeACPressCombo(theme)
            , m_modeACHoldCombo(theme)
            , m_modeCCPressCombo(theme)
            , m_modeLibraryPressCombo(theme)
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
        ComboBox m_libraryPressCombo;
        ComboBox m_modeACPressCombo;
        ComboBox m_modeACHoldCombo;
        ComboBox m_modeCCPressCombo;
        ComboBox m_modeLibraryPressCombo;

        SettingsLine * m_pAllyHidLine;
        SettingsLine * m_pACPressLine = nullptr;
        SettingsLine * m_pACHoldLine = nullptr;
        SettingsLine * m_pCCPressLine = nullptr;
        SettingsLine * m_pLibraryPressLine = nullptr;
        SettingsLine * m_pModeACPressLine = nullptr;
        SettingsLine * m_pModeACHoldLine = nullptr;
        SettingsLine * m_pModeCCPressLine = nullptr;
        SettingsLine * m_pModeLibraryPressLine = nullptr;

        void OpenAllyHidPage();
        void EnableAllyHidChanged();

        void ReloadIcons();
    };
}
