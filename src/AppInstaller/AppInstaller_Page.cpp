// MIT License
//
// Copyright (c) 2025 Artem Shpynov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <string>
#include <filesystem>

#include <fstream>
#include <filesystem>
#include <ShlObj.h>

#include "Tools/Process.hpp"
#include "Tools/ProcessEx.hpp"
#include "Tools/TaskManager.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/TaskManager.hpp"
#include "AppInstaller.hpp"

namespace AnyFSE
{
    namespace fs = std::filesystem;

    std::list<HWND> AppInstaller::CreatePage()
    {
        RECT rc;
        GetClientRect(m_hDialog, &rc);

        rc.left += m_theme.DpiScale(Layout_ImageWidth + Layout_Margins * 2);
        InflateRect(&rc, -m_theme.DpiScale(Layout_Margins), -m_theme.DpiScale(Layout_Margins));
        int width = rc.right - rc.left;

        std::list<HWND> page;

        page.push_back(m_imageStatic.Create(m_hDialog,
            rc.left - m_theme.DpiScale(Layout_ImageWidth + Layout_Margins),
            rc.top + m_theme.DpiScale(Layout_ImageTop),
            m_theme.DpiScale(Layout_ImageWidth),
            m_theme.DpiScale(Layout_ImageWidth)
        ));

        page.push_back( m_captionStatic.Create(m_hDialog,
            rc.left,
            rc.top,
            width,
            m_theme.DpiScale(Layout_CaptionHeight)
        ));

        page.push_back( m_textStatic.Create(m_hDialog,
            rc.left,
            rc.top + m_theme.DpiScale(Layout_CaptionHeight),
            width,
            rc.bottom - rc.top - m_theme.DpiScale(Layout_CaptionHeight + Layout_ButtonHeight + Layout_ButtonPadding)
        ));

        page.push_back( m_pathEdit.Create(m_hDialog,
            rc.left,
            rc.top + m_theme.DpiScale(Layout_CaptionHeight + Layout_TextHeight),
            width - m_theme.DpiScale(Layout_BrowseButtonWidth + Layout_ButtonPadding),
            m_theme.DpiScale(Layout_EditHeight)
        ));

        page.push_back(m_browseButton.Create(m_hDialog,
            L"Browse", delegate(OnBrowse),
            rc.right - m_theme.DpiScale(Layout_BrowseButtonWidth),
            rc.top + m_theme.DpiScale(
                Layout_CaptionHeight + Layout_TextHeight
                + (Layout_EditHeight - Layout_ButtonHeight) / 2
            ),
            m_theme.DpiScale(Layout_BrowseButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ));

        page.push_back( m_leftButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth * 2 + Layout_ButtonPadding),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ));

        page.push_back( m_rightButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ));

        m_captionStatic.SetLarge(true);
        m_captionStatic.SetColor(Theme::Colors::TextAccented);
        m_pathEdit.OnChanged += delegate(CheckPath);

        return page;
    }

    void AppInstaller::ShowPage(
        const std::wstring &icon,
        const std::wstring &caption,
        const std::wstring &text,
        const std::wstring &buttonRight,
        const std::function<void()> &callbackRight,
        const std::wstring &buttonLeft,
        const std::function<void()> &callbackLeft,
        bool showBrowse)
    {
        if (icon.empty())
        {
            wchar_t modulePath[MAX_PATH];
            GetModuleFileName(NULL, modulePath, MAX_PATH);
            m_imageStatic.LoadIcon(modulePath, 128);
        }
        else
        {
            m_imageStatic.LoadIcon(icon, 128);
        }

        m_captionStatic.SetText(caption);
        m_textStatic.SetText(text);
        m_rightButton.SetText(buttonRight);
        m_rightButton.OnChanged = callbackRight;

        if( !buttonLeft.empty())
        {
            m_leftButton.SetText(buttonLeft);
            m_leftButton.OnChanged = callbackLeft;
            m_leftButton.Show(true);
        }
        else
        {
            m_leftButton.Show(false);
        }

        m_pathEdit.Show(showBrowse);
        m_browseButton.Show(showBrowse);

        RedrawWindow(m_hDialog, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW);
    }

    void AppInstaller::ShowWelcomePage()
    {
        ShowPage(L"",
            L"Welcome to AnyFSE Installer",
            std::wstring(L"This will install AnyFSE version ") +
            Unicode::to_wstring(APP_VERSION) +
            std::wstring(L" to your system.\n\nClick Next to proceed or Cancel to exit."),
            L"Cancel", delegate(OnCancel),
            L"Next", delegate(ShowLicensePage)
        );
    }

    void AppInstaller::ShowLicensePage()
    {
        ShowPage(Icon_EULA,
            L"End User License Agreements",

            L"AnyFSE is free software, distributed under terms of GNU GPLv3 License "
            L"in the hope that it will be useful, "
            L"but WITHOUT ANY WARRANTY; without even the implied warranty of "
            L"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
            L"See https://www.gnu.org/licenses/",

            L"Cancel", delegate(OnCancel),
            L"Accept", delegate(OnSelectPath)
        );
    }

    void AppInstaller::ShowPathPage()
    {
        ShowPage(Icon_Browse,
            L"Select AnyFSE location",
            L"Here you may select folder "
            L"where to install AnyFSE.",
            L"Cancel", delegate(OnCancel),
            L"Install", delegate(OnInstall),
            true
        );
    }

    void AppInstaller::ShowProgressPage()
    {
        ShowPage(Icon_Progress,
            L"Installing AnyFSE",
            L"Preparation",
            L"Cancel", delegate(OnCancel));
    }

    void AppInstaller::ShowCompletePage()
    {
        ShowPage(Icon_Done,
            L"Done",
            L"AnyFSE installation has been completed",
            L"Setup", delegate(OnSettings),
            fs::exists(m_pathEdit.GetText() + L"\\AnyFSE.json") ? L"Done" : L"", delegate(OnDone)
        );
    }

    void AppInstaller::ShowErrorPage(const std::wstring &caption, const std::wstring &text, const std::wstring &icon)
    {
        ShowPage(icon.empty() ? Icon_Error : icon,
                 caption,
                 text,
                 L"Close", delegate(OnCancel));
    }

    void AppInstaller::OnSelectPath()
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

        ShowPathPage();
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
        m_leftButton.Enable(IsValidPath(m_pathEdit.GetText()));
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
    void AppInstaller::SetCurrentProgress(const std::wstring& status)
    {
        m_textStatic.SetText(status);
        RedrawWindow(m_hDialog, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW);
    }

    void AppInstaller::OnInstall()
    {
        try
        {
            fs::path path(m_pathEdit.GetText());
            if (fs::exists(path) && !fs::is_directory(path))
            {
                throw std::exception("File name is used as path");
            }

            ShowProgressPage();
            Sleep(1);

            bool status = true;
            // Kill existing process
            SetCurrentProgress(L"Search and terminate existing application");
            status &= TerminateAnyFSE();

            // Extract resource file
            SetCurrentProgress(L"Unpack files");
            status &= ExtractEmbeddedZip(path);

            // Set task
            SetCurrentProgress(L"Register scheduled task");
            status &= Tools::TaskManager::CreateTask(path.wstring() + L"\\AnyFSE.exe");
        }
        catch(const std::exception& e)
        {
            ShowErrorPage(L"Installation Error", Unicode::to_wstring(e.what()));
            return;
        }

        ShowCompletePage();
    }

    bool AppInstaller::TerminateAnyFSE()
    {
        std::set<DWORD> handles;
        if (Process::FindAllByName(L"AnyFSE.exe", handles))
        {
            for (auto& handle: handles)
            {
                ProcessEx::Kill(handle);
            }
        }

        return true;
    }

    bool AppInstaller::ExtractEmbeddedZip(const std::wstring &path)
    {
        if (FAILED(SHCreateDirectoryExW(NULL, path.c_str(), NULL)))
        {
            throw std::exception("Cannot create folder");
        }

        HINSTANCE hInstance = GetModuleHandle(NULL);

        // Find the ZIP resource
        HRSRC hResource = FindResource(hInstance, MAKEINTRESOURCE(IDR_EMBEDDED_ZIP), L"ZIP");
        if (!hResource)
        {
            throw std::exception("Instalation file is corrupted. No packed files resource.");
        }

        // Load the resource
        HGLOBAL hGlobal = LoadResource(hInstance, hResource);
        if (!hGlobal)
        {
            throw std::exception("Instalation file is corrupted. Cannot load packed files.");
        }

        // Get resource data
        LPVOID zipData = LockResource(hGlobal);
        DWORD zipSize = SizeofResource(hInstance, hResource);

        if (!zipData || zipSize == 0)
        {
            throw std::exception("Instalation file is corrupted. No zipped data.");
        }

        // Write resource to temporary ZIP file
        std::wstring zipArchive = path + L"\\AnyFSE." + Unicode::to_wstring(APP_VERSION) + L".zip";

        std::ofstream tempStream(zipArchive, std::ios::binary);
        tempStream.write(static_cast<const char *>(zipData), zipSize);
        tempStream.close();


        // Use Windows built-in tar to extract
        SHELLEXECUTEINFOW sei = {sizeof(sei)};
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpFile = L"tar";

        std::wstring parameters = L"-xf \"" + zipArchive + L"\" -C \"" + path + L"\"";
        sei.lpParameters = parameters.c_str();
        sei.nShow = SW_HIDE;

        BOOL success = ShellExecuteExW(&sei);
        if (success)
        {
            WaitForSingleObject(sei.hProcess, INFINITE);
            DWORD exitCode;
            GetExitCodeProcess(sei.hProcess, &exitCode);
            success = (exitCode == 0);
            CloseHandle(sei.hProcess);
        }
        else
        {
            throw std::exception("Instalation file is corrupted. Failed unpacking.");
        }

        // Clean up temporary file
        DeleteFile(zipArchive.c_str());

        return success;
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