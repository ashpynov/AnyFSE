

#include <windows.h>
#include <appmodel.h>
#include <iostream>
#include <filesystem>

#include "Packages.hpp"
#include "App/AppConstants.hpp"
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

        familyName = pathPos ? GetPackageFamilyName(path.substr(0, pathPos)) : AppConstants::PackageFamilyName;
        resourcePath = pathPos != std::wstring::npos ? path.substr(pathPos) : L"";

        auto packages = pm.FindPackagesForUser(L"", familyName);

        for (auto const &pkg : packages)
        {
            try
            {
                Windows::Storage::Streams::IRandomAccessStreamWithContentType stream;
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
            catch (winrt::hresult_error const&)
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
        winrt::init_apartment(); // Initialize WinRT apartment for using WinRT APIs
        winrt::Windows::Management::Deployment::PackageManager packageManager;

        auto packages = packageManager.FindPackagesForUser(L"", packageFamilyName);
        return (bool)packages;
    }

    bool InstallPackage( const std::wstring & packageFilePath, const std::wstring & packageFamilyName, const std::wstring & externalInstallPath)
    {
        winrt::init_apartment(); // Initialize WinRT apartment for using WinRT APIs

        try
        {
            namespace fs = std::filesystem;
            namespace d = winrt::Windows::Management::Deployment;
            namespace f = winrt::Windows::Foundation;

            d::PackageManager packageManager;


            f::Uri packageUri(fs::absolute(packageFilePath).wstring());
            std::wcerr << packageUri.ToString().c_str() << std::endl;

            d::AddPackageOptions options;
            options.ForceUpdateFromAnyVersion(true);
            options.ForceTargetAppShutdown(true);
            options.ForceAppShutdown(true);
            options.ExternalLocationUri(f::Uri(externalInstallPath));

            // Start the installation
            auto operation = packageManager.AddPackageByUriAsync(packageUri, options);

            // Wait for installation to complete
            auto result = operation.get();

            if (result.IsRegistered())
            {
                return true;
            }
            else
            {
                throw std::exception(Unicode::to_string(result.ErrorText().c_str()).c_str());
            }

            return true;
        }
        catch (const winrt::hresult_error &e)
        {
            throw std::exception(Unicode::to_string(e.message().c_str()).c_str());
        }
        return false;
    }

    bool RemovePackage(const std::wstring & packageFamilyName)
    {
        winrt::init_apartment(); // Initialize WinRT apartment for using WinRT APIs

        try
        {
            namespace fs = std::filesystem;
            namespace d = winrt::Windows::Management::Deployment;
            namespace f = winrt::Windows::Foundation;

            d::PackageManager packageManager;
            auto packages = packageManager.FindPackages(packageFamilyName);
            for (auto& package : packages)
            {
                auto removing = packageManager.RemovePackageAsync(package.Id().FullName());
                auto removed = removing.get();
            }
            return true;
        }
        catch (const winrt::hresult_error &e)
        {
            throw std::exception(Unicode::to_string(e.message().c_str()).c_str());
        }
        return false;
    }
}
