#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <string>
#include <filesystem>

#include "AppInstaller/AppInstaller.hpp"
#include "Tools/TaskManager.hpp"

namespace AnyFSE
{
    std::list<HWND> AppInstaller::CreatePathPage()
    {
        RECT rc;
        GetClientRect(m_hDialog, &rc);

        rc.left += m_theme.DpiScale(Layout_ImageWidth + Layout_Margins * 2);
        InflateRect(&rc, -m_theme.DpiScale(Layout_Margins), -m_theme.DpiScale(Layout_Margins));
        int width = rc.right - rc.left;

        std::list<HWND> page;

        page.push_back(m_pathImageStatic.Create(m_hDialog,
            rc.left - m_theme.DpiScale(Layout_ImageWidth + Layout_Margins),
            rc.top + m_theme.DpiScale(Layout_ImageTop),
            m_theme.DpiScale(Layout_ImageWidth),
            m_theme.DpiScale(Layout_ImageWidth)
        ));

        page.push_back( m_pathCaptionStatic.Create(m_hDialog,
            rc.left,
            rc.top,
            width,
            m_theme.DpiScale(Layout_CaptionHeight)
        ));

        page.push_back( m_pathTextStatic.Create(m_hDialog,
            rc.left,
            rc.top + m_theme.DpiScale(Layout_CaptionHeight),
            width,
            m_theme.DpiScale(Layout_TextHeight)
        ));

        page.push_back( m_pathEdit.Create(m_hDialog,
            rc.left,
            rc.top + m_theme.DpiScale(Layout_CaptionHeight + Layout_TextHeight),
            width - m_theme.DpiScale(Layout_BrowseButtonWidth + Layout_ButtonPadding),
            m_theme.DpiScale(Layout_EditHeight)
        ));

        page.push_back(m_pathBrowseButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_BrowseButtonWidth),
            rc.top + m_theme.DpiScale(
                Layout_CaptionHeight + Layout_TextHeight
                + (Layout_EditHeight - Layout_ButtonHeight) / 2
            ),
            m_theme.DpiScale(Layout_BrowseButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ));

        page.push_back( m_pathNextButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth * 2 + Layout_ButtonPadding),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ));

        page.push_back( m_pathCancelButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)

        ));

        m_pathImageStatic.LoadIcon(Icon_Browse, 128);

        m_pathCaptionStatic.SetText(L"Select AnyFSE location");
        m_pathCaptionStatic.SetLarge(true);
        m_pathCaptionStatic.SetColor(Theme::Colors::TextAccented);

        m_pathTextStatic.SetText(
            L"Here you may select folder "
            L"where to install AnyFSE."
        );

        m_pathEdit.OnChanged += delegate(CheckPath);

        m_pathBrowseButton.SetText(L"Browse");
        m_pathBrowseButton.OnChanged += delegate(OnBrowse);

        m_pathNextButton.SetText(L"Install");
        m_pathNextButton.OnChanged += delegate(Install);

        m_pathCancelButton.SetText(L"Cancel");
        m_pathCancelButton.OnChanged += delegate(OnCancel);

        return page;
    }

    void AppInstaller::GoToPathPage()
    {
        std::wstring path = Tools::TaskManager::GetInstallPath();
        if (path.empty())
        {
            wchar_t programFiles[MAX_PATH] = {0};
            ExpandEnvironmentStringsW(L"%PROGRAMFILES%\\AnyFSE", programFiles, MAX_PATH);
            m_pathEdit.SetText(programFiles);
        }
        else
        {
            m_pathEdit.SetText(path);
        }

        GoToPage(Pages::Path);
    }

    #define TRY(hr) if (FAILED((hr))) break;
    #define FREE(f) if (f) f->Release();

    void AppInstaller::OnBrowse()
    {
        IFileDialog *pFileDialog = nullptr;
        IShellItem *pItem = nullptr;
        IShellItem* pDefaultFolder = nullptr;

        std::wstring installationPath = m_pathEdit.GetText();

        do
        {
            TRY(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));

            TRY(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                 IID_IFileDialog, reinterpret_cast<void **>(&pFileDialog)));

            // Set options for folder picking
            DWORD dwOptions;
            pFileDialog->GetOptions(&dwOptions);
            pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);
            pFileDialog->SetTitle(L"Select AnyFSE Installation Directory");

            namespace fs = std::filesystem;
            fs::path path = fs::path(installationPath);
            if (fs::exists(path) && fs::is_directory(path))
            {
                SHCreateItemFromParsingName(path.wstring().c_str(), NULL, IID_IShellItem,
                                            reinterpret_cast<void **>(&pDefaultFolder));
                if (pDefaultFolder) pFileDialog->SetFolder(pDefaultFolder);
            }

            TRY(pFileDialog->Show(m_hDialog));
            TRY(pFileDialog->GetResult(&pItem));

            PWSTR pszPath;
            TRY(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath));

            installationPath = pszPath;
            path = fs::path(installationPath);


            if (!fs::exists(path.wstring() + L"\\AnyFSE.exe") && _wcsicmp(path.filename().wstring().c_str(), L"AnyFSE"))
                installationPath += std::wstring(L"\\AnyFSE");

            m_pathEdit.SetText(installationPath);
            CoTaskMemFree(pszPath);

        } while (false);

        FREE(pItem);
        FREE(pFileDialog);
        FREE(pDefaultFolder);

        CoUninitialize();
    }
    void AppInstaller::CheckPath()
    {
        m_pathNextButton.Enable(IsValidPath(m_pathEdit.GetText()));
    }

    bool AppInstaller::IsValidPath(const std::wstring& pathStr)
    {
        if (pathStr.empty() || pathStr.find(L':') == std::wstring::npos)
        {
            return false;
        }
        namespace fs = std::filesystem;
        fs::path path(pathStr);

        do
        {
            if (fs::exists(path))
            {
                return fs::is_directory(path);
            }
            path = path.parent_path();
        } while (path.has_parent_path() && path != path.parent_path());

        return false;
    }
}