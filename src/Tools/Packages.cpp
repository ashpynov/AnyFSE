

#include <windows.h>
#include <appmodel.h>
#include <iostream>
#include <vector>
#include "Packages.hpp"

namespace AnyFSE::Tools
{
    std::wstring Packages::GetAppxInstallLocation(const std::wstring &packageName)
    {
        UINT32 count = 0;
        UINT32 bufferLength = 0;

        // Search for packages with the specific name pattern
        std::wstring searchPattern = packageName;

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
}