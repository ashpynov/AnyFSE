#include "AppInstaller/AppInstaller.hpp"
#include "Tools/Unicode.hpp"

namespace AnyFSE
{
    std::list<HWND> AppInstaller::CreateWelcomePage()
    {
        RECT rc;
        GetClientRect(m_hDialog, &rc);

        rc.left += m_theme.DpiScale( Layout_ImageWidth + Layout_Margins * 2);
        InflateRect(&rc, -m_theme.DpiScale(Layout_Margins), -m_theme.DpiScale(Layout_Margins));
        int width = rc.right - rc.left;

        std::list<HWND> page;

        page.push_back(m_welcomeImageStatic.Create(m_hDialog,
            rc.left - m_theme.DpiScale(Layout_ImageWidth + Layout_Margins),
            rc.top + m_theme.DpiScale(Layout_ImageTop),
            m_theme.DpiScale(Layout_ImageWidth),
            m_theme.DpiScale(Layout_ImageWidth)
        ));

        page.push_back( m_welcomeCaptionStatic.Create(m_hDialog,
            rc.left,
            rc.top,
            width,
            m_theme.DpiScale(Layout_CaptionHeight)
        ));

        page.push_back( m_welcomeTextStatic.Create(m_hDialog,
            rc.left,
            rc.top + m_theme.DpiScale(Layout_CaptionHeight),
            width,
            rc.bottom - rc.top - m_theme.DpiScale(Layout_CaptionHeight + Layout_ButtonHeight + Layout_ButtonPadding)
        ));

        page.push_back( m_welcomeNextButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth * 2 + Layout_ButtonPadding),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ));

        page.push_back( m_welcomeCancelButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)

        ));


        wchar_t modulePath[MAX_PATH];
        GetModuleFileName(NULL, modulePath, MAX_PATH);
        m_welcomeImageStatic.LoadIcon(modulePath, 128);

        m_welcomeCaptionStatic.SetText(L"Welcome to AnyFSE Installer");
        m_welcomeCaptionStatic.SetLarge(true);
        m_welcomeCaptionStatic.SetColor(Theme::Colors::TextAccented);

        m_welcomeTextStatic.SetText(
             std::wstring(L"This will install AnyFSE version ") +
             Unicode::to_wstring(APP_VERSION) +
             std::wstring(L" to your system.\n\nClick Next to proceed or Cancel to exit.")
        );
        m_welcomeNextButton.SetText(L"Next");
        m_welcomeNextButton.OnChanged += delegate(GoToPathPage);
        m_welcomeCancelButton.SetText(L"Cancel");
        m_welcomeCancelButton.OnChanged += delegate(OnCancel);

        return page;
    }

}