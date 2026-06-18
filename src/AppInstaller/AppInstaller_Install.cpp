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
#include <filesystem>
#include <string>

#include "App/AppConstants.hpp"
#include "AppInstaller.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/Process.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/Paths.hpp"
#include "Tools/Packages.hpp"
#include "AppInstaller/Certificate.hpp"

namespace AnyFSE
{
    namespace fs = std::filesystem;
    static Logger log = LogManager::GetLogger("Installer");

    void AppInstaller::OnInstall()
    {
        fs::path path(fs::temp_directory_path().append(AppConstants::TempInstallDirName));
        if (fs::exists(path))
        {
            fs::remove_all(path);
        }

        LogManager::Initialize("AnyFSE.Installer", LogLevels::Trace);
        log.Info("Starting Installation AnyFSE v%s to %s", APP_VERSION, path.string().c_str());

        bool acseServiceWasRunning = IsInjectorServiceRun();
        bool certificateWasInstalled = ToolsEx::Certificate::IsRootCertificateInstalled(Unicode::to_wstring(VER_PUBLISHER_CN));

        try
        {
            ShowProgressPage();
            Sleep(1);

#ifdef OFFLINE_INSTALLER
            // Extract resource file
            SetCurrentProgress(Translate(L"progressUnpackFiles"));
            CheckSuccess(ExtractEmbeddedZip(path));
#else
            SetCurrentProgress(Translate(L"progressDownloadFiles"));
            CheckSuccess(DownloadFiles(path));
#endif

            std::wstring oldPath = Registry::ReadString(registryPath, L"InstallLocation");
            if (!oldPath.empty())
            {
                SetCurrentProgress(Translate(L"progressRemoveOldVersion"));
                CheckSuccess(DeleteOldVersion() && DeleteOldFiles(oldPath));
            }

            m_isDevModeEnabled = IsDeveloperModeEnabled();
            if (!m_isDevModeEnabled)
            {
                SetCurrentProgress(Translate(L"progressEnableDeveloperMode"));
                EnableDeveloperMode(true);
                CheckSuccess(true);
            }

            if (ToolsEx::Certificate::IsRootCertificateInstalled(Unicode::to_wstring(VER_PUBLISHER_CN)))
            {
                SetCurrentProgress(Translate(L"progressInstallPublisherCertificate"));
                m_isRootCertInstalled = ToolsEx::Certificate::InstallRootCertificate(path.wstring() + L"/" + AppConstants::PublisherCertFile);
                CheckSuccess(m_isRootCertInstalled);
            }

            if (IsNeedEnableAsusOptimization())
            {
                SetCurrentProgress(Translate(L"progressRestoreAsusOptimizationService"));
                CheckSuccess(EnableAsusOptimization());
            }

            SetCurrentProgress(Translate(L"progressInstallPackage"));
            CopyFiles(path, Tools::Paths::GetInstallPath());

            CheckSuccess(Tools::Packages::InstallPackage(
                path.wstring() + L"/" + AppConstants::AppxFilePrefix + Unicode::to_wstring(VER_VERSION_STR) + L".identity.appx",
                AppConstants::PackageFamilyName,
                Tools::Paths::GetInstallPath()
            ));

            RegisterUninstall();

            if (acseServiceWasRunning)
            {
                SetCurrentProgress(Translate(L"progressStartAcseInjectorService"));
                CheckSuccess(EnableInjectorService());
            }

            SetCurrentProgress(Translate(L"progressCleanupFiles"));
            CheckSuccess(true);

            if (!m_isDevModeEnabled)
            {
                SetCurrentProgress(Translate(L"progressDisableDeveloperMode"));
                EnableDeveloperMode(false);
                CheckSuccess(true);
            }

            if (m_isRootCertInstalled)
            {
                SetCurrentProgress(Translate(L"progressRemovingCertificate"));
                ToolsEx::Certificate::RemoveRootCertificate(Unicode::to_wstring(VER_PUBLISHER_CN));
                m_isRootCertInstalled = false;
                CheckSuccess(true);
            }

            Process::StartProtocol(AnyFSE::AppConstants::AnyFseProtocolAllyHid);

            ShowCompletePage();

            LogManager::DeleteLog();
            if (fs::exists(path))
            {
                fs::remove_all(path);
            }

        }
        catch (const std::exception& e)
        {
            log.Error(e, "Installation fail:");
            ShowErrorPage(Translate(L"installationErrorCaption"), GetProgressText(4) + Unicode::to_wstring(e.what()));

            // Best-effort service recovery after failed update.
            if (acseServiceWasRunning)
            {
                EnableInjectorService();
            }

            if (!certificateWasInstalled)
            {
                ToolsEx::Certificate::RemoveRootCertificate(Unicode::to_wstring(VER_PUBLISHER_CN));
            }

            if (!m_isDevModeEnabled)
            {
                EnableDeveloperMode(false);
            }
        }
    }
}
