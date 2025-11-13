#include <fstream>
#include <filesystem>
#include <ShlObj.h>
#include "AppInstaller/AppInstaller.hpp"
#include "Tools/Process.hpp"
#include "Tools/ProcessEx.hpp"
#include "Tools/TaskManager.hpp"
#include "Tools/Unicode.hpp"


namespace AnyFSE
{
    std::list<HWND> AppInstaller::CreateProgressPage()
    {
        RECT rc;
        GetClientRect(m_hDialog, &rc);

        rc.left += m_theme.DpiScale(Layout_ImageWidth + Layout_Margins * 2);
        InflateRect(&rc, -m_theme.DpiScale(Layout_Margins), -m_theme.DpiScale(Layout_Margins));
        int width = rc.right - rc.left;

        std::list<HWND> page;

        page.push_back(m_progressImageStatic.Create(m_hDialog,
            rc.left - m_theme.DpiScale(Layout_ImageWidth + Layout_Margins),
            rc.top + m_theme.DpiScale(Layout_ImageTop),
            m_theme.DpiScale(Layout_ImageWidth),
            m_theme.DpiScale(Layout_ImageWidth)
        ));

        page.push_back( m_progressCaptionStatic.Create(m_hDialog,
            rc.left,
            rc.top,
            width,
            m_theme.DpiScale(Layout_CaptionHeight)
        ));

        page.push_back( m_progressTextStatic.Create(m_hDialog,
            rc.left,
            rc.top + m_theme.DpiScale(Layout_CaptionHeight),
            width,
            rc.bottom - rc.top - m_theme.DpiScale(Layout_CaptionHeight + Layout_ButtonHeight + Layout_ButtonPadding)
        ));

        page.push_back( m_progressCancelButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)

        ));

        m_progressImageStatic.LoadIcon(Icon_Progress, 128);

        m_progressCaptionStatic.SetText(L"Installing AnyFSE");
        m_progressCaptionStatic.SetLarge(true);
        m_progressCaptionStatic.SetColor(Theme::Colors::TextAccented);

        m_progressTextStatic.SetText(std::wstring(L"Preparation"));

        m_progressCancelButton.SetText(L"Cancel");
        m_progressCancelButton.OnChanged += delegate(OnCancel);

        return page;
    }

    void AppInstaller::SetCurrentProgress(const std::wstring& status)
    {
        m_progressTextStatic.SetText(status);
        RedrawWindow(m_hDialog, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW);
    }

    void AppInstaller::Install()
    {
        try
        {
            namespace fs = std::filesystem;
            fs::path path(m_pathEdit.GetText());
            if (fs::exists(path) && !fs::is_directory(path))
            {
                throw std::exception("File name is used as path");
            }

            GoToPage(Pages::Progress);
            Sleep(1);

            bool status = true;
            // Kill existing process
            SetCurrentProgress(L"Search and terminate existing application");
            status &= TerminateActiveApp();

            // Extract resource file
            SetCurrentProgress(L"Unpack files");
            status &= ExtractEmbeddedZip(path);

            // Set task
            SetCurrentProgress(L"Register scheduled task");
            status &= Tools::TaskManager::CreateTask(path.wstring() + L"\\AnyFSE.exe");
        }
        catch(const std::exception& e)
        {
            ShowError(L"Installation Error", Unicode::to_wstring(e.what()));
            return;
        }

        GoToPage(Pages::Complete);
    }

    bool AppInstaller::TerminateActiveApp()
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

}