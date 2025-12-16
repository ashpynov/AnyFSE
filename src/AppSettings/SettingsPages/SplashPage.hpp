#pragma once

#include <list>
#include "FluentDesign/Theme.hpp"
#include "AppSettings/SettingsPages/SettingsPage.hpp"

namespace AnyFSE::App::AppSettings::Settings::Page
{
    using namespace FluentDesign;

    class SplashPage : public SettingsPage
    {
        Theme &m_theme;
        SettingsDialog &m_dialog;

    public:
        SplashPage(Theme& theme, SettingsDialog &dialog)
            : m_theme(theme)
            , m_dialog(dialog)
            , m_showAnimationToggle(m_theme)
            , m_showLogoToggle(m_theme)
            , m_showTextToggle(m_theme)
            , m_showVideoToggle(m_theme)
            , m_videoLoopToggle(m_theme)
            , m_videoMuteToggle(m_theme)
            , m_videoPauseToggle(m_theme)
            , m_customTextEdit(m_theme)
            , m_splashCustomVideoEdit(m_theme)
        {}

        std::list<SettingsLine> &GetSettingsLines() { return m_pageLinesList;  };

        void AddPage(std::list<SettingsLine>& settingPageList, ULONG &top);
        void LoadControls();
        void SaveControls();

    private:
        std::list<SettingsLine> m_pageLinesList;

        Toggle m_showAnimationToggle;
        Toggle m_showLogoToggle;
        Toggle m_showTextToggle;
        Toggle m_showVideoToggle;
        Toggle m_videoLoopToggle;
        Toggle m_videoMuteToggle;
        Toggle m_videoPauseToggle;

        TextBox m_customTextEdit;
        TextBox m_splashCustomVideoEdit;

        SettingsLine * m_pSplashTextLine = nullptr;
        SettingsLine * m_pSplashLogoLine = nullptr;
        SettingsLine * m_pSplashVideoLine = nullptr;

        void OnGotoSplashFolder();
        void OpenSplashSettingsPage();
        void OnShowTextChanged();
        void OnShowLogoChanged();
        void OnShowVideoChanged();
    };
};