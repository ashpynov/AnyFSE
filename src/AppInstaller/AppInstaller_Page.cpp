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
#include "Tools/Unicode.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Paths.hpp"
#include "Tools/Localization.hpp"
#include "App/AppConstants.hpp"
#include "AppInstaller.hpp"
#include "Logging/LogManager.hpp"

#pragma comment(lib, "urlmon.lib")

namespace AnyFSE
{
    namespace fs = std::filesystem;
    static Logger log = LogManager::GetLogger("Installer");

    namespace
    {
        bool ExtractZipArchive(const std::wstring &zipArchive, const std::wstring &path)
        {
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

            return success == TRUE;
        }
    }

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

        page.push_back( m_leftButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth * 2 + Layout_ButtonPadding),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ).GetHwnd());

        page.push_back( m_rightButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ).GetHwnd());

        page.push_back(m_languageButton
            .Create(m_hDialog,
                rc.left - m_theme.DpiScale(Layout_ImageWidth + Layout_Margins * 2),
                rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
                m_theme.DpiScale(Layout_ButtonHeight),
                m_theme.DpiScale(Layout_ButtonHeight))
            //.SetText(Translate(L"language"))
            .SetIcon(L"\xE164")
            .SetFlat(true)
            .GetHwnd()
        );

        PopulateLanguageMenu();
        m_languageButton.Show(false);

        m_captionStatic.SetLarge(true);
        m_captionStatic.SetColor(Theme::Colors::TextAccented);

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

        m_imageStatic.LoadIcon(icon.empty() ? Tools::Paths::GetExeFileName() : icon, 128);

        m_captionStatic.SetText(caption);
        m_textStatic.SetText(text);

        if( !buttonRight.empty())
        {
            m_rightButton.SetText(buttonRight);
            m_rightButton.OnChanged = callbackRight;
            m_rightButton.Show(true);
        }
        else
        {
            m_rightButton.Show(false);
        }


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

        RedrawWindow(m_hDialog, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW);
    }

    void AppInstaller::ShowWelcomePage()
    {
        m_languageButton.Show(true);
        if (m_isUpdate)
        {
            ShowPage(L"",
                Translate(L"updaterWelcomeCaption"),
                TranslateF(L"updaterWelcomeDescription", Unicode::to_wstring(APP_VERSION).c_str()),
                Translate(L"cancelBtn"), delegate(OnCancel),
                Translate(L"updateBtn"), delegate(OnInstall)
            );
        }
        else
        {
            ShowPage(L"",
                Translate(L"installerWelcomeCaption"),
                TranslateF(L"installerWelcomeDescription", Unicode::to_wstring(APP_VERSION).c_str()),
                Translate(L"cancelBtn"), delegate(OnCancel),
                Translate(L"nextBtn"), delegate(ShowLicensePage)
            );
        }
    }

    void AppInstaller::ShowLicensePage()
    {
        m_languageButton.Show(false);
        ShowPage(Icon_EULA,
            Translate(L"licenseCaption"),

            Translate(L"licenseDescription"),

            Translate(L"cancelBtn"), delegate(OnCancel),
            Translate(L"acceptBtn"), delegate(OnInstall)
        );
    }

    void AppInstaller::ShowProgressPage()
    {
        m_languageButton.Show(false);
        ShowPage(Icon_Progress,
            m_isUpdate ? Translate(L"updaterProgressCaption") : Translate(L"installerProgressCaption"),
            Translate(L"progressPreparation"),
            L"", delegate(OnCancel));
    }

    void AppInstaller::ShowCompletePage()
    {
        m_languageButton.Show(false);
        if (m_isUpdate)
        {
            ShowPage(Icon_Done,
                Translate(L"doneBtn"),
                Translate(L"updaterDoneDescription"),
                Translate(L"doneBtn"), delegate(OnDone)
            );
        }
        else
        {
            ShowPage(Icon_Done,
                Translate(L"doneBtn"),
                Translate(L"installerDoneDescription"),
                Translate(L"configureBtn"), delegate(OnSettings),
                IsConfigured() ? Translate(L"doneBtn") : L"", delegate(OnDone)
            );
        }
    }

    bool AppInstaller::IsConfigured()
    {
        return fs::exists(fs::path(Tools::Paths::GetConfigPath() + L"\\AnyFSE.json"));
    }

    void AppInstaller::ShowErrorPage(const std::wstring &caption, const std::wstring &text, const std::wstring &icon)
    {
        m_languageButton.Show(false);
        ShowPage(icon.empty() ? Icon_Error : icon,
                 caption,
                 text,
                 Translate(L"closeBtn"), delegate(OnCancel));
    }

    void AppInstaller::PopulateLanguageMenu()
    {
        std::vector<Popup::PopupItem> items;
        for (const auto &locale : Tools::Localization::EnumerateResourceLocales())
        {
            items.emplace_back(
                Unicode::to_upper(locale.code) == Unicode::to_upper(Tools::Localization::GetCurrentLocale())
                    ? L"\xE1D2"
                    : L"\xEA3F",
                 locale.language,
                 [this, code = locale.code]() { OnSelectLanguage(code); }
            );
        }

        if (!items.empty())
        {
            m_languageButton.SetMenu(items,  m_theme.DpiScale(120), TPM_LEFTALIGN);
            m_languageButton.OnChanged = [this]() { m_languageButton.ShowMenu(); };
        }
    }

    void AppInstaller::OnSelectLanguage(const std::wstring &localeCode)
    {
        Tools::Localization::SetPreferredLocale(localeCode);

        Tools::Localization::InitializeFromLocales();

        PopulateLanguageMenu();
        UpdateDialogTitle();
        ShowWelcomePage();
    }


    #define TRY(hr) if (FAILED((hr))) break;
    #define FREE(f) if (f) f->Release();

    std::wstring AppInstaller::GetProgressText(int lines)
    {
        std::wstring progress;

        auto it = m_progressStatus.begin();
        if (m_progressStatus.size() > lines)
        {
            std::advance(it, m_progressStatus.size() - lines);
        }
        for (; it != m_progressStatus.end(); ++it)
        {
            std::wstring prefix = (*it)[0] < L'\x2000' ? L" \x2012  " : L"";
            progress += prefix + *it + L"\n";
        }
        return progress;
    }
    void AppInstaller::SetCurrentProgress(const std::wstring& status)
    {
        m_progressStatus.push_back(status);
        m_textStatic.SetText(GetProgressText(5));
        RedrawWindow(m_hDialog, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW);
    }

    void AppInstaller::CheckSuccess(bool bSuccess)
    {
        log.Info("%s - %s", Unicode::to_string(m_progressStatus.back()).c_str(), bSuccess ? "OK" : "FAIL");

        std::wstring step = (bSuccess ? L"\x2713 ": L"\x2715 ") + m_progressStatus.back();
        m_progressStatus.pop_back();
        SetCurrentProgress(step);
        if (!bSuccess)
        {
            throw Logging::Logger::APIError();
        }
    }

    bool AppInstaller::DeleteOldVersion()
    {
        std::wstring uninstaller = Registry::ReadString(registryPath, L"UninstallString");
        if (uninstaller.empty() || !fs::exists(uninstaller))
        {
            return true;
        }
        // Execute uninstaller
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpFile = uninstaller.c_str();
        sei.lpParameters = L"/s /u";
        sei.nShow = SW_HIDE;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        if (ShellExecuteExW(&sei))
        {
            WaitForSingleObject(sei.hProcess, 30000);
            CloseHandle(sei.hProcess);
        }
        return true;
    }

    bool AppInstaller::DeleteOldFiles(const std::wstring& dir)
    {
        std::wstring moduleName = Unicode::to_lower(Paths::GetExeFileName());

        fs::path path(dir);
        if (fs::is_directory(path))
        {
            for (const auto & entry : fs::directory_iterator(path))
            {
                if (!entry.is_regular_file() || Unicode::to_lower(entry.path().wstring()) == moduleName)
                {
                    continue;
                }
                std::wstring ext = Unicode::to_lower(entry.path().extension().wstring());

                if (ext == L".log" || ext == L".exe" || ext == L".dll")
                {
                    fs::remove(entry.path());
                }
            }
        }
        return true;
    }

#ifdef OFFLINE_INSTALLER
    bool AppInstaller::ExtractEmbeddedZip(const std::wstring &path)
    {
        try {
            std::filesystem::create_directories(path);
        } catch (const std::filesystem::filesystem_error&) {
            throw std::exception("Cannot create binary folder");
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
        std::wstring zipArchive = path + L"\\" + std::wstring(AppConstants::ReleaseZipPrefix) + Unicode::to_wstring(APP_VERSION) + L".zip";

        std::ofstream tempStream(zipArchive, std::ios::binary);
        tempStream.write(static_cast<const char *>(zipData), zipSize);
        tempStream.close();


        bool success = ExtractZipArchive(zipArchive, path);
        if (!success)
        {
            throw std::exception("Instalation file is corrupted. Failed unpacking.");
        }

        // Clean up temporary file
        DeleteFile(zipArchive.c_str());

        return success;
    }
#else
    bool AppInstaller::DownloadFiles(const std::wstring &path)
    {
        try {
            std::filesystem::create_directories(path);
        } catch (const std::filesystem::filesystem_error&) {
            throw std::exception("Cannot create binary folder");
        }
        const std::wstring rootPath = std::wstring(AppConstants::GitHubReleaseRoot) + Unicode::to_wstring(APP_VERSION);
        const std::wstring rootPathAlt = std::wstring(AppConstants::CodebergReleaseRoot) + Unicode::to_wstring(APP_VERSION);
        const std::wstring zipName = std::wstring(AppConstants::ReleaseZipPrefix) + Unicode::to_wstring(APP_VERSION) + L".zip";
        const std::wstring zipArchive = path + L"\\" + zipName;

        HRESULT hr = URLDownloadToFileW(
            NULL,
            (rootPath + L"/" + zipName).c_str(),
            zipArchive.c_str(),
            0,
            NULL);

        if (FAILED(hr))
        {
            hr = URLDownloadToFileW(
                NULL,
                (rootPathAlt + L"/" + zipName).c_str(),
                zipArchive.c_str(),
                0,
                NULL);
        }

        if (FAILED(hr))
        {
            throw std::runtime_error("Download failed");
        }

        bool success = ExtractZipArchive(zipArchive, path);
        DeleteFile(zipArchive.c_str());
        if (!success)
        {
            throw std::runtime_error("Download failed unpacking");
        }

        return true;
    }
#endif

    void AppInstaller::OnSettings()
    {
        Process::StartProtocol(AppConstants::AnyFseProtocolSettings);
        EndDialog(m_hDialog, IDOK);
    }

    void AppInstaller::OnDone()
    {
        EndDialog(m_hDialog, IDOK);
    }

    bool AppInstaller::AutoDeleteSelf()
    {
        std::wstring batchPath = fs::temp_directory_path().wstring() + L"\\ins000_anyfse_cleanup.bat";

        std::wofstream batch(batchPath);
        if (!batch.is_open()) return false;

        batch << L"@echo off\n";
        batch << L"chcp 65001 >nul\n";
        batch << L"echo Cleaning up...\n";

        batch << L":waitloop\n";
        batch << L"tasklist /fi \"PID eq " << GetCurrentProcessId() << L"\" | find \"" << GetCurrentProcessId() << L"\" >nul\n";
        batch << L"if not errorlevel 1 (\n";
        batch << L"  timeout /t 1 /nobreak >nul\n";
        batch << L"  goto waitloop\n";
        batch << L")\n\n";

        // Delete this batch file
        batch << L"del /f /q \"" << Tools::Paths::GetExeFileName() << "\"\n";
        batch << L"del /f /q \"" << batchPath << L"\"\n";

        batch << L"echo Cleaning complete!\n";

        batch.close();

        // Execute batch
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpFile = batchPath.c_str();
        sei.nShow = SW_HIDE;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        if (ShellExecuteExW(&sei))
        {
            CloseHandle(sei.hProcess);
        }

        return true;
    }

}
