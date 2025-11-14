#include "AppInstaller/AppInstaller.hpp"
#include "Tools/Process.hpp"

namespace AnyFSE
{
    std::list<HWND> AppInstaller::CreateCompletePage()
    {
        RECT rc;
        GetClientRect(m_hDialog, &rc);

        rc.left += m_theme.DpiScale(Layout_ImageWidth + Layout_Margins * 2);
        InflateRect(&rc, -m_theme.DpiScale(Layout_Margins), -m_theme.DpiScale(Layout_Margins));
        int width = rc.right - rc.left;

        std::list<HWND> page;

        page.push_back(m_completeImageStatic.Create(m_hDialog,
            rc.left - m_theme.DpiScale(Layout_ImageWidth + Layout_Margins),
            rc.top + m_theme.DpiScale(Layout_ImageTop),
            m_theme.DpiScale(Layout_ImageWidth),
            m_theme.DpiScale(Layout_ImageWidth)
        ));

        page.push_back( m_completeCaptionStatic.Create(m_hDialog,
            rc.left,
            rc.top,
            width,
            m_theme.DpiScale(Layout_CaptionHeight)
        ));

        page.push_back( m_completeTextStatic.Create(m_hDialog,
            rc.left,
            rc.top + m_theme.DpiScale(Layout_CaptionHeight),
            width,
            rc.bottom - rc.top - m_theme.DpiScale(Layout_CaptionHeight + Layout_ButtonHeight + Layout_ButtonPadding)
        ));

        page.push_back( m_completeDoneButton.Create(m_hDialog,
            L"Done", delegate(OnDone),
            rc.right - m_theme.DpiScale(Layout_ButtonWidth * 2 + Layout_ButtonPadding),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ));

        page.push_back( m_completeSettingsButton.Create(m_hDialog,
            L"Setup", delegate(OnSettings),
            rc.right - m_theme.DpiScale(Layout_ButtonWidth),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ));

        m_completeImageStatic.LoadIcon(Icon_Done, 128);

        m_completeCaptionStatic.SetText(L"Done");
        m_completeCaptionStatic.SetLarge(true);
        m_completeCaptionStatic.SetColor(Theme::Colors::TextAccented);

        m_completeTextStatic.SetText(std::wstring(L"AnyFSE installation has been completed"));

        return page;
    }

    void AppInstaller::OnSettings()
    {
        StartAnyFSE(true);
    }

    void AppInstaller::OnDone()
    {
        StartAnyFSE(false);
    }

    void AppInstaller::StartAnyFSE(bool bSettings)
    {
        Process::StartProcess(m_pathEdit.GetText() + L"\\AnyFSE.exe", bSettings ? L"/Settings" : L"");
        EndDialog(m_hDialog, IDOK);
    }
}