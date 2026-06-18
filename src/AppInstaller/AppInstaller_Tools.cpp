#include <string>
#include <windows.h>
#include <wincrypt.h>
#include <filesystem>

#include <winrt/Windows.Foundation.h>
#include <winrt/windows.applicationmodel.h>
#include <winrt/windows.foundation.collections.h>
#include <winrt/Windows.Management.Deployment.h>

#include "AppInstaller.hpp"
#include "App/AppConstants.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/Paths.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/Process.hpp"
#include "Ally/Services.hpp"


namespace AnyFSE
{
    static Logger log = LogManager::GetLogger("Installer");
    namespace
    {
        bool WaitForServiceState(SC_HANDLE service, DWORD desiredState, DWORD timeoutMs)
        {
            const ULONGLONG deadline = GetTickCount64() + timeoutMs;

            for (;;)
            {
                SERVICE_STATUS_PROCESS status = {};
                DWORD bytesNeeded = 0;
                if (!QueryServiceStatusEx(
                        service,
                        SC_STATUS_PROCESS_INFO,
                        reinterpret_cast<LPBYTE>(&status),
                        sizeof(status),
                        &bytesNeeded))
                {
                    return false;
                }

                if (status.dwCurrentState == desiredState)
                {
                    return true;
                }

                if (GetTickCount64() >= deadline)
                {
                    return false;
                }

                Sleep(250);
            }
        }

        uint64_t GetDirectorySize(const std::filesystem::path& dir)
        {
            uint64_t total = 0;
            std::error_code ec;

            for (std::filesystem::recursive_directory_iterator it(dir, ec), end; it != end; it.increment(ec))
            {
                if (ec)
                {
                    continue;
                }

                if (it->is_regular_file(ec))
                {
                    total += it->file_size(ec);
                }
            }

            return total;
        }
    }

    bool AppInstaller::IsDeveloperModeEnabled()
    {
        return Registry::ReadDWORD(
            L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock",
            L"AllowDevelopmentWithoutDevLicense",
            0) == 1;
    }

    void AppInstaller::EnableDeveloperMode(bool bEnable)
    {
        Registry::WriteDWORD(
            L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock",
            L"AllowDevelopmentWithoutDevLicense",
            bEnable ? 1 : 0);
    }

    bool AppInstaller::CopyFiles( const std::wstring & sourcePath, const std::wstring & destPath)
    {
        namespace fs = std::filesystem;

        if (fs::exists(destPath))
        {
            fs::remove_all(destPath);
        }

        fs::create_directories(destPath);
        fs::copy(sourcePath, destPath, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        return true;
    }

    bool AppInstaller::RegisterUninstall()
    {
        const std::wstring installPath = Tools::Paths::GetInstallPath();
        const std::wstring uninstallPath = Tools::Paths::GetInstallPath() + L"\\unins000.exe";
        const DWORD estimatedSizeKb = static_cast<DWORD>((GetDirectorySize(installPath) + 1023) / 1024);

        return
            Registry::WriteString(registryPath, L"DisplayName", Unicode::to_wstring(VER_PRODUCT_NAME)) &&
            Registry::WriteString(registryPath, L"DisplayVersion", Unicode::to_wstring(APP_VERSION)) &&
            Registry::WriteString(registryPath, L"Publisher", Unicode::to_wstring(VER_COMPANY_NAME)) &&
            Registry::WriteString(registryPath, L"InstallLocation", installPath) &&
            Registry::WriteString(registryPath, L"DisplayIcon", installPath + L"\\" + Unicode::to_wstring(VER_PRODUCT_NAME) + L".exe") &&
            Registry::WriteString(registryPath, L"UninstallString", uninstallPath) &&
            Registry::WriteString(registryPath, L"QuietUninstallString", uninstallPath + L" /s") &&
            Registry::WriteDWORD(registryPath, L"EstimatedSize", estimatedSizeKb) &&
            Registry::WriteDWORD(registryPath, L"NoModify", 1) &&
            Registry::WriteDWORD(registryPath, L"NoRepair", 1);
    }

    bool AppInstaller::IsInjectorServiceRun()
    {
        return Process::FindFirstByExe(AppConstants::InjectorExe) != 0;
    }

    bool AppInstaller::DisableInjectorService()
    {
        return Ally::Services::DisableInjectorService();
    }

    bool AppInstaller::EnableInjectorService()
    {
        return Ally::Services::EnableInjectorService();
    }

    bool AppInstaller::IsNeedEnableAsusOptimization()
    {
        return Process::FindFirstByExe(AppConstants::ArmouryCrateServiceProcess) && !Process::FindFirstByExe(AppConstants::AsusOptimizationProcess);
    }
    bool AppInstaller::EnableAsusOptimization()
    {
        return Ally::Services::EnableAsusOptimizationService();
    }
};
