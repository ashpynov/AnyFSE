#include "AppInstaller/AppInstaller.hpp"

namespace AnyFSE
{
    std::list<HWND> AppInstaller::CreateErrorPage()
    {
        RECT rc;
        GetClientRect(m_hDialog, &rc);

        rc.left += m_theme.DpiScale(Layout_ImageWidth + Layout_Margins * 2);
        InflateRect(&rc, -m_theme.DpiScale(Layout_Margins), -m_theme.DpiScale(Layout_Margins));
        int width = rc.right - rc.left;

        std::list<HWND> page;

        page.push_back(m_errorImageStatic.Create(m_hDialog,
            rc.left - m_theme.DpiScale(Layout_ImageWidth + Layout_Margins),
            rc.top + m_theme.DpiScale(Layout_ImageTop),
            m_theme.DpiScale(Layout_ImageWidth),
            m_theme.DpiScale(Layout_ImageWidth)
        ));

        page.push_back( m_errorCaptionStatic.Create(m_hDialog,
            rc.left,
            rc.top,
            width,
            m_theme.DpiScale(Layout_CaptionHeight)
        ));

        page.push_back( m_errorTextStatic.Create(m_hDialog,
            rc.left,
            rc.top + m_theme.DpiScale(Layout_CaptionHeight),
            width,
            rc.bottom - rc.top - m_theme.DpiScale(Layout_CaptionHeight + Layout_ButtonHeight + Layout_ButtonPadding)
        ));

        page.push_back( m_errorCloseButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)

        ));

        m_errorImageStatic.LoadIcon(Icon_Error, 128);

        m_errorCaptionStatic.SetText(L"Error");
        m_errorCaptionStatic.SetLarge(true);
        m_errorCaptionStatic.SetColor(Theme::Colors::TextAccented);

        m_errorCaptionStatic.SetText(std::wstring(L"Some error description Here"));

        m_errorCloseButton.SetText(L"Close");
        m_errorCloseButton.OnChanged += delegate(OnCancel);

        return page;
    }
}