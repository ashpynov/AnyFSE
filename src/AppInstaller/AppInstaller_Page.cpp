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
#include "AppInstaller.hpp"
#include "Logging/LogManager.hpp"

namespace AnyFSE
{
    namespace fs = std::filesystem;
    static Logger log = LogManager::GetLogger("Installer");

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
        if (m_isUpdate)
        {
            ShowPage(L"",
                L"Welcome to AnyFSE Updater",
                std::wstring(L"This will update AnyFSE to version ") +
                Unicode::to_wstring(APP_VERSION) +
                std::wstring(L" on your system.\n\nClick Update to proceed or Cancel to exit."),
                L"Cancel", delegate(OnCancel),
                L"Update", delegate(OnInstall)
            );
        }
        else
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
    }

    void AppInstaller::ShowLicensePage()
    {
        ShowPage(Icon_EULA,
            L"End User License Agreements",

            L"AnyFSE is free software, distributed under terms of MIT License "
            L"in the hope that it will be useful, "
            L"but WITHOUT ANY WARRANTY; without even the implied warranty of "
            L"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.",

            L"Cancel", delegate(OnCancel),
            L"Accept", delegate(OnInstall)
        );
    }

    void AppInstaller::ShowProgressPage()
    {
        ShowPage(Icon_Progress,
            m_isUpdate ? L"Updating AnyFSE" : L"Installing AnyFSE",
            L"Preparation",
            L"", delegate(OnCancel));
    }

    void AppInstaller::ShowCompletePage()
    {
        if (m_isUpdate)
        {
            ShowPage(Icon_Done,
                L"Done",
                L"AnyFSE update has been completed.",
                L"Done", delegate(OnDone)
            );
        }
        else
        {
            ShowPage(Icon_Done,
                L"Done",
                L"AnyFSE installation has been completed.\n\nPress Configure change settings.\n",
                L"Configure", delegate(OnSettings),
                IsConfigured() ? L"Done" : L"", delegate(OnDone)
            );
        }
    }

    bool AppInstaller::IsConfigured()
    {
        wchar_t appData[MAX_PATH]={0};
        ExpandEnvironmentStringsW(L"%PROGRAMDATA%\\AnyFSE", appData, MAX_PATH);
        return fs::exists(fs::path(std::wstring(appData) + L"\\AnyFSE.json"));
    }

    void AppInstaller::ShowErrorPage(const std::wstring &caption, const std::wstring &text, const std::wstring &icon)
    {
        ShowPage(icon.empty() ? Icon_Error : icon,
                 caption,
                 text,
                 L"Close", delegate(OnCancel));
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

    void AppInstaller::OnInstall()
    {
        fs::path path(fs::temp_directory_path().append(L"AnyFSE_install"));
        if (fs::exists(path))
        {
            fs::remove_all(path);
        }

        fs::create_directories(path.wstring() + L"\\logs");

        LogManager::Initialize("AnyFSE.Installer", LogLevels::Debug, path.wstring() + L"\\logs");
        log.Info("Starting Installation AnyFSE v%s to %s", APP_VERSION, path.string().c_str());

        try
        {
            ShowProgressPage();
            Sleep(1);

            std::wstring oldPath = Registry::ReadString(registryPath, L"InstallLocation");
            if (!oldPath.empty())
            {
                SetCurrentProgress(L"Remove old version");
                CheckSuccess(DeleteOldVersion() && DeleteOldFiles(oldPath));
            }

            // Extract resource file
            SetCurrentProgress(L"Unpack files");
            CheckSuccess(ExtractEmbeddedZip(path));


            m_isDevModeEnabled = IsDeveloperModeEnabled();
            if (!m_isDevModeEnabled)
            {
                SetCurrentProgress(L"Enable developer mode");
                EnableDeveloperMode(true);
                CheckSuccess(true);
            }

            bool isRootCertInstalled = IsRootCertificateInstalled(Unicode::to_wstring(VER_PUBLISHER_CN));
            if (!isRootCertInstalled)
            {
                SetCurrentProgress(L"Install publisher certificate");
                CheckSuccess(InstallRootCertificate(path.wstring() + L"/" + Unicode::to_wstring(VER_COMPANY_NAME) + L".cer"));
            }

            SetCurrentProgress(L"Install package");
            CheckSuccess(InstallPackage(
                path.wstring() + L"/AnyFSE-" + Unicode::to_wstring(VER_VERSION_STR) + L".appx",
                L"ArtemShpynov.AnyFSE_by4wjhxmygwn4"
            ));

            SetCurrentProgress(L"Cleanup files");
            CheckSuccess(true);

            if (!m_isDevModeEnabled)
            {
                SetCurrentProgress(L"Disable developer mode");
                EnableDeveloperMode(false);
                CheckSuccess(true);
            }

            ShowCompletePage();

            LogManager::DeleteLog();
            if (fs::exists(path))
            {
                fs::remove_all(path);
            }

        }
        catch(const std::exception& e)
        {


            if (!m_isDevModeEnabled)
            {
                EnableDeveloperMode(false);
            }

            log.Error(e, "Instalation fail:");
            ShowErrorPage(L"Installation Error", GetProgressText(4) + Unicode::to_wstring(e.what()));
        }
    }

    bool AppInstaller::DeleteOldVersion()
    {
        std::wstring uninstaller = Registry::ReadString(registryPath, L"UninstallString");
        if (uninstaller.empty())
        {
            return true;
        }
        // Execute uninstaller
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpFile = uninstaller.c_str();
        sei.lpParameters = L"/s";
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

    bool AppInstaller::ExtractEmbeddedZip(const std::wstring &path)
    {
        if (FAILED(SHCreateDirectoryExW(NULL, path.c_str(), NULL)))
        {
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
        Process::StartProtocol(L"anyfse://settings");
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