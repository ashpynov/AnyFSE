#pragma once

#include <list>
#include "FluentDesign/Theme.hpp"
#include "AppSettings/SettingsPages/SettingsPage.hpp"

namespace AnyFSE::App::AppSettings::Settings::Page
{
    using namespace FluentDesign;

    class UpdatePage : public SettingsPage
    {
        Theme &m_theme;
        SettingsDialog &m_dialog;

    public:
        UpdatePage(Theme& theme, SettingsDialog &dialog)
            : m_theme(theme)
            , m_dialog(dialog)
            , m_checkIntervalCombo(m_theme)
            , m_preReleaseToggle(m_theme)
            , m_notificationsToggle(m_theme)
        {}

        std::list<SettingsLine> &GetSettingsLines() { return m_pageLinesList;  };

        void AddPage(std::list<SettingsLine>& settingPageList, ULONG &top);
        void LoadControls();
        void SaveControls();

    private:
        std::list<SettingsLine> m_pageLinesList;

        ComboBox m_checkIntervalCombo;
        Toggle m_preReleaseToggle;
        Toggle m_notificationsToggle;

        void OpenUpdateSettingsPage();
        void UpdateSettingsChangedCheck();
        void UpdateSettingsChanged();
    };
};