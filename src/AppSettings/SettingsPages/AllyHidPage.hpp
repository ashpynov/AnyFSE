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

        SettingsLine * m_pAllyHidLine;

        void OpenAllyHidPage();
        void EnableAllyHidChanged();
    };
}
