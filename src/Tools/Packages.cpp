

#include <windows.h>
#include <appmodel.h>
#include <iostream>
#include <vector>
#include "Packages.hpp"

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


namespace AnyFSE::Tools
{
    std::wstring GetPackageFamilyName(const std::wstring& appUserModelID)
    {
        size_t pos = appUserModelID.find(L'!');
        return pos == std::wstring::npos ? appUserModelID : appUserModelID.substr(0, pos);
    }

    std::wstring Packages::GetAppxInstallLocation(const std::wstring &appUserModelID)
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

    std::wstring Packages::GetAppDisplayName(const std::wstring &appUserModelID)
    {
        PackageManager pm;
        auto packages = pm.FindPackagesForUser(L"", GetPackageFamilyName(appUserModelID));

        for (auto const &pkg : packages)
        {
            return pkg.DisplayName().c_str();
        }
        return L"";
    }

    std::vector<uint8_t> Packages::GetAppDisplayLogoRawBytes(const std::wstring &appUserModelID, int size)
    {
        PackageManager pm;
        auto packages = pm.FindPackagesForUser(L"", GetPackageFamilyName(appUserModelID));

        for (auto const &pkg : packages)
        {
            auto logoRef = pkg.GetLogoAsRandomAccessStreamReference(Windows::Foundation::Size((float)size, (float)size));
            auto stream = logoRef.OpenReadAsync().get();
            auto buffer = Buffer(static_cast<uint32_t>(stream.Size()));
            auto readBuffer = stream.ReadAsync(buffer, (uint32_t)stream.Size(), InputStreamOptions::None).get();

            std::vector<uint8_t> bytes(readBuffer.Length());
            auto reader = DataReader::FromBuffer(readBuffer);
            reader.ReadBytes(bytes);

            return bytes;
        }
        return std::vector<uint8_t>();
    }

    std::vector<DWORD> Packages::GetAppProcessIds(const std::wstring &appUserModelID)
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

    std::vector<std::wstring> Packages::GetNativeLaunchers()
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
}