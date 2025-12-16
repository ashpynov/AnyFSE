#include "Tools/Event.hpp"
#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "SplashPage.hpp"


namespace AnyFSE::App::AppSettings::Settings::Page
{
    void SplashPage::AddPage(std::list<SettingsLine>& settingPageList, ULONG &top)
    {

        SettingsLine & dialogLine = m_dialog.AddSettingsLine(settingPageList, top,
            L"Splash screen settings",
            L"Configure Look'n'Feel of splash screen during home app loading",
            Layout::LineHeight, Layout::LinePadding, 0);

        dialogLine.SetState(SettingsLine::Next);
        dialogLine.SetIcon(L'\xEB9F');
        dialogLine.OnChanged += delegate(OpenSplashSettingsPage);

        ULONG pageTop = 0;
        m_pSplashTextLine = &m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Show Loading text",
            L"",
            m_showTextToggle,
            Layout::LineHeight, 0, 0);

        m_pSplashTextLine->AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Use Custom Text",
            L"",
            m_customTextEdit,
            Layout::LineHeightSmall, Layout::LinePadding, Layout::LineSmallMargin));

        m_pSplashLogoLine = &m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Show Home application Logo",
            L"",
            m_showLogoToggle,
            Layout::LineHeight, 0, 0);

        m_pSplashLogoLine->AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Animate Logo",
            L"",
            m_showAnimationToggle,
            Layout::LineHeightSmall, Layout::LinePadding, Layout::LineSmallMargin));


        m_pSplashVideoLine = &m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Show Video",
            L"Show random video from \"splash\" folder",
            m_showVideoToggle,
            Layout::LineHeight, 0, 0);

        m_pSplashVideoLine->OnLink = delegate(OnGotoSplashFolder);

        m_pSplashVideoLine->AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Loop video",
            L"",
            m_videoLoopToggle,
            Layout::LineHeightSmall, 0, Layout::LineSmallMargin));

        m_pSplashVideoLine->AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Mute video",
            L"",
            m_videoMuteToggle,
            Layout::LineHeightSmall, 0, Layout::LineSmallMargin));

        m_pSplashVideoLine->AddGroupItem(&m_dialog.AddSettingsLine(m_pageLinesList, pageTop,
            L"Pause completed",
            L"Show last frame when video completed earlier than home app start",
            m_videoPauseToggle,
            Layout::LineHeightSmall, Layout::LinePadding, Layout::LineSmallMargin));

        m_showTextToggle.OnChanged += delegate(OnShowTextChanged);
        m_pSplashTextLine->OnChanged += delegate(m_dialog.UpdateLayout);

        m_showLogoToggle.OnChanged += delegate(OnShowLogoChanged);
        m_pSplashLogoLine->OnChanged += delegate(m_dialog.UpdateLayout);

        m_showVideoToggle.OnChanged += delegate(OnShowVideoChanged);
        m_pSplashVideoLine->OnChanged += delegate(m_dialog.UpdateLayout);
    }

    void SplashPage::LoadControls()
    {
        m_showTextToggle.SetCheck(Config::SplashShowText);
        m_customTextEdit.SetText(Config::SplashCustomText);

        m_showLogoToggle.SetCheck(Config::SplashShowLogo);
        m_showAnimationToggle.SetCheck(Config::SplashShowAnimation);

        m_showVideoToggle.SetCheck(Config::SplashShowVideo);
        m_videoLoopToggle.SetCheck(Config::SplashVideoLoop);
        m_videoMuteToggle.SetCheck(Config::SplashVideoMute);
        m_videoPauseToggle.SetCheck(Config::SplashVideoPause);
    }

    void SplashPage::SaveControls()
    {
        Config::SplashShowText = m_showTextToggle.GetCheck();
        Config::SplashCustomText = m_customTextEdit.GetText();

        Config::SplashShowLogo = m_showLogoToggle.GetCheck();
        Config::SplashShowAnimation = m_showAnimationToggle.GetCheck();

        Config::SplashShowVideo = m_showVideoToggle.GetCheck();
        Config::SplashVideoLoop = m_videoLoopToggle.GetCheck();
        Config::SplashVideoMute = m_videoMuteToggle.GetCheck();
        Config::SplashVideoPause = m_videoPauseToggle.GetCheck();
    }

    void SplashPage::OnGotoSplashFolder()
    {
        std::wstring path = Config::GetModulePath() + L"\\splash";

        CreateDirectoryW(path.c_str(), NULL);
        Process::StartProtocol(L"\"" + path + L"\"");
    }

    void SplashPage::OpenSplashSettingsPage()
    {
        m_dialog.SwitchActivePage(L"Splash settings", &m_pageLinesList);
    }

    void SplashPage::OnShowTextChanged()
    {
        m_pSplashTextLine->SetState(m_showTextToggle.GetCheck()
            ? SettingsLine::Opened
            : SettingsLine::Normal);

        m_dialog.UpdateLayout();
    }

    void SplashPage::OnShowLogoChanged()
    {
        m_pSplashLogoLine->SetState(m_showLogoToggle.GetCheck()
            ? SettingsLine::Opened
            : SettingsLine::Normal);

        m_dialog.UpdateLayout();
    }

    void SplashPage::OnShowVideoChanged()
    {
        m_pSplashVideoLine->SetState(m_showVideoToggle.GetCheck()
            ? SettingsLine::Opened
            : SettingsLine::Normal);

        m_dialog.UpdateLayout();
    }

};