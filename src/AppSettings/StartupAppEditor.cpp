#include <filesystem>
#include "StartupAppEditor.hpp"
#include "Tools/DoubleBufferedPaint.hpp"

namespace AnyFSE::App::AppSettings::Settings
{
    void StartupAppEditor::OnOk()
    {
        m_refPath = m_pathEdit.GetText();
        m_refArgs = m_argsEdit.GetText();
        FluentDesign::Dialog::OnOK();
    }

    void StartupAppEditor::OnCancel()
    {
        FluentDesign::Dialog::OnCancel();
    }

    void StartupAppEditor::OnBrowse()
    {
        OPENFILENAME ofn = {};
        WCHAR szFile[MAX_PATH + 1] = {};

        wcsncpy_s(szFile, m_pathEdit.GetText().c_str(), MAX_PATH);

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_hDialog;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = L"Executable (*.exe)\0*.exe\0\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn))
        {
            m_pathEdit.SetText(szFile);
            ValidatePath();
        }
    }

    void StartupAppEditor::ValidatePath()
    {
        namespace fs = std::filesystem;
        m_okButton.Enable(fs::exists(fs::path(m_pathEdit.GetText())));
    }

    INT_PTR StartupAppEditor::EditApp(HWND hParent, std::wstring &refPath, std::wstring &refArgs)
    {
        StartupAppEditor dialog(refPath, refArgs);
        return dialog.Show(hParent);
    }

    void StartupAppEditor::Create()
    {
        RECT rc;
        GetClientRect(m_hDialog, &rc);
        InflateRect(&rc, -m_theme.DpiScale(Layout_Margins), -m_theme.DpiScale(Layout_Margins));
        int width = rc.right - rc.left;
        int top = rc.top;

        m_captionStatic.Create(m_hDialog, m_refPath.empty() ? L"Add application" : L"Edit application",
            rc.left, top, width, m_theme.DpiScale(Layout_CaptionHeight)
        );
        m_captionStatic.SetLarge(true);
        top += m_theme.DpiScale(Layout_CaptionHeight);

        m_pathStatic.Create(m_hDialog, L"Select app to execute", rc.left, top, width, m_theme.DpiScale(Layout_TextHeight));
        m_pathStatic.Format().SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentFar);

        top += m_theme.DpiScale(Layout_TextHeight);

        m_pathEdit.Create(m_hDialog, rc.left, top,
            width - m_theme.DpiScale(Layout_BrowseButtonWidth + Layout_ButtonPadding),
            m_theme.DpiScale(Layout_EditHeight)
        );

        m_browseButton.Create(m_hDialog, L"Browse", delegate(OnBrowse),
            rc.right - m_theme.DpiScale(Layout_BrowseButtonWidth),
            top + (Layout_EditHeight - Layout_ButtonHeight) / 2,
            m_theme.DpiScale(Layout_BrowseButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        );
        top += m_theme.DpiScale(Layout_EditHeight);

        m_argsStatic.Create(m_hDialog, L"Additional startup arguments",
            rc.left, top, width, m_theme.DpiScale(Layout_TextHeight)
        );
        m_argsStatic.Format().SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentFar);

        top += m_theme.DpiScale(Layout_TextHeight);
        m_argsEdit.Create(m_hDialog, rc.left, top, width, m_theme.DpiScale(Layout_EditHeight));

        m_okButton.Create(m_hDialog, L"OK", delegate(OnOk),
            rc.right - m_theme.DpiScale(Layout_ButtonWidth *2 + Layout_ButtonPadding),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth), m_theme.DpiScale(Layout_ButtonHeight)
        );
        m_cancelButton.Create(m_hDialog, L"Cancel", delegate(OnCancel),
            rc.right - m_theme.DpiScale(Layout_ButtonWidth),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth), m_theme.DpiScale(Layout_ButtonHeight)
        );

        m_pathEdit.SetText(m_refPath);
        m_pathEdit.OnChanged = delegate(ValidatePath);

        m_argsEdit.SetText(m_refArgs);

        ValidatePath();
    }
}