

#include <windows.h>
#include <appmodel.h>
#include <exception>

#include "Packages.hpp"
#include "App/Constants.hpp"
#include "Tools/Unicode.hpp"

#include <winrt/windows.foundation.collections.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.Diagnostics.h>
#include <winrt/Windows.ApplicationModel.AppExtensions.h>

using namespace winrt;
using namespace Windows::Management::Deployment;
using namespace Windows::ApplicationModel;
using namespace Windows::Storage::Streams;
using namespace Windows::System;
using namespace Windows::System::Diagnostics;
using namespace Windows::ApplicationModel::AppExtensions;


namespace AnyFSE::Tools::Packages
{
    std::wstring GetPackageFamilyName(const std::wstring& appUserModelID)
    {
        size_t pos = appUserModelID.find(L'!');
        return pos == std::wstring::npos ? appUserModelID : appUserModelID.substr(0, pos);
    }

    std::wstring GetAppxInstallLocation(const std::wstring &appUserModelID)
    {
        UINT32 count = 0;
        UINT32 bufferLength = 0;

        // Search for packages with the specific name pattern
        std::wstring searchPattern = GetPackageFamilyName(appUserModelID);

        // First call to get the count and buffer size
        LONG result = GetPackagesByPackageFamily(
            searchPattern.c_str(),
            &count,
            nullptr,
            &bufferLength,
            nullptr);

        if (result == ERROR_INSUFFICIENT_BUFFER && count > 0)
        {
            std::vector<WCHAR> buffer(bufferLength);
            std::vector<WCHAR*> packageFullNames(count);

            // Second call to get the actual package names
            result = GetPackagesByPackageFamily(
                searchPattern.c_str(),
                &count,
                packageFullNames.data(),
                &bufferLength,
                buffer.data());

            if (result == ERROR_SUCCESS && count > 0)
            {
                // Get the install path for the first matching package
                WCHAR path[MAX_PATH];
                UINT32 pathLength = MAX_PATH;

                if (GetStagedPackagePathByFullName(buffer.data(), &pathLength, path) == ERROR_SUCCESS)
                {
                    return std::wstring(path);
                }
            }
        }

        return L"";
    }

    std::wstring GetAppDisplayName(const std::wstring &appUserModelID)
    {
        PackageManager pm;
        auto packages = pm.FindPackagesForUser(L"", GetPackageFamilyName(appUserModelID));

        for (auto const &pkg : packages)
        {
            return pkg.DisplayName().c_str();
        }
        return L"";
    }

    std::vector<uint8_t> GetAppDisplayLogoRawBytes(const std::wstring &path, int size)
    {
        PackageManager pm;

        std::wstring familyName;
        std::wstring resourcePath;
        size_t pathPos = path.find(L'/');

        familyName = pathPos ? GetPackageFamilyName(path.substr(0, pathPos)) : App::Constants::PackageFamilyName;
        resourcePath = pathPos != std::wstring::npos ? path.substr(pathPos) : L"";

        auto packages = pm.FindPackagesForUser(L"", familyName);

        for (auto const &pkg : packages)
        {
            try
            {
                IRandomAccessStreamWithContentType stream;
                if ( resourcePath.empty() )
                {
                    auto logoRef = pkg.GetLogoAsRandomAccessStreamReference(Windows::Foundation::Size((float)size, (float)size));
                    stream = logoRef.OpenReadAsync().get();
                }
                else
                {
                    auto installFolder = pkg.InstalledLocation();
                    std::replace(resourcePath.begin(), resourcePath.end(), L'/', L'\\');
                    std::wstring fullPath = installFolder.Path().c_str() + resourcePath;
                    auto file = Windows::Storage::StorageFile::GetFileFromPathAsync(fullPath).get();
                    auto streamRef = RandomAccessStreamReference::CreateFromFile(file);

                    stream = streamRef.OpenReadAsync().get();
                }

                auto buffer = Buffer(static_cast<uint32_t>(stream.Size()));
                auto readBuffer = stream.ReadAsync(buffer, (uint32_t)stream.Size(), InputStreamOptions::None).get();

                std::vector<uint8_t> bytes(readBuffer.Length());
                auto reader = DataReader::FromBuffer(readBuffer);
                reader.ReadBytes(bytes);

                return bytes;
            }
            catch (hresult_error const&)
            {
            }
        }
        return std::vector<uint8_t>();
    }

    std::vector<DWORD> GetAppProcessIds(const std::wstring &appUserModelID)
    {
        std::vector<DWORD> pids;
        auto infos = AppDiagnosticInfo::RequestInfoForAppAsync(appUserModelID).get();

        for (const auto& info : infos)
        {
            auto groups = info.GetResourceGroups();
            for (const auto& group : groups)
            {
                auto processes = group.GetProcessDiagnosticInfos();
                for (const auto& process : processes)
                {
                    const wchar_t * c = process.ExecutableFileName().c_str();
                    pids.push_back(process.ProcessId());
                }
            }
        }
        return pids;
    }

    std::vector<std::wstring> GetNativeLaunchers()
    {
        std::vector<std::wstring> launchers;

        auto catalog = AppExtensionCatalog::Open(L"windows.gamingApp");
        auto extensions = catalog.FindAll();

        for (auto extension : extensions)
        {
            launchers.push_back(extension.AppInfo().AppUserModelId().c_str());
        }

        return launchers;

    }

    bool IsPackageInstalled(const std::wstring &packageFamilyName)
    {
        init_apartment(); // Initialize WinRT apartment for using WinRT APIs
        PackageManager packageManager;

        auto packages = packageManager.FindPackagesForUser(L"", packageFamilyName);
        return packages.begin() != packages.end();
    }

    std::string GetPackageVersion(const std::wstring &packageFamilyName)
    {
        init_apartment();

        PackageManager packageManager;
        const auto packages = packageManager.FindPackagesForUser(L"", packageFamilyName);
        for (const auto &package : packages)
        {
            const auto version = package.Id().Version();
            return std::to_string(version.Major) + "." + std::to_string(version.Minor) + "."
                + std::to_string(version.Build) + "." + std::to_string(version.Revision);
        }
        return "";
    }

    bool InstallPackage(const std::wstring &packageFilePath)
    {
        init_apartment();

        try
        {
            PackageManager packageManager;
            const Windows::Foundation::Uri packageUri(packageFilePath);
            RegisterPackageOptions options;
            options.DeveloperMode(true);
            options.ForceUpdateFromAnyVersion(true);

            const DeploymentResult result = packageManager.RegisterPackageByUriAsync(packageUri, options).get();
            if (result.IsRegistered())
                return true;

            throw std::exception(Unicode::to_string(result.ErrorText().c_str()).c_str());
        }
        catch (const hresult_error &e)
        {
            throw std::exception(Unicode::to_string(e.message().c_str()).c_str());
        }
    }

    bool RemovePackage(const std::wstring &packageFamilyName, bool preserveData)
    {
        init_apartment(); // Initialize WinRT apartment for using WinRT APIs

        try
        {
            PackageManager packageManager;
            for (const Package &package : packageManager.FindPackagesForUser(L"", packageFamilyName))
            {
                RemovalOptions options = !preserveData ? RemovalOptions::None
                    : package.IsDevelopmentMode()
                        ? RemovalOptions::PreserveApplicationData
                        : RemovalOptions::PreserveRoamableApplicationData;

                const DeploymentResult result =
                    packageManager.RemovePackageAsync(package.Id().FullName(), options).get();
                check_hresult(result.ExtendedErrorCode());
            }
            return true;
        }
        catch (const hresult_error &e)
        {
            throw std::exception(Unicode::to_string(e.message().c_str()).c_str());
        }
        return false;
    }
}
