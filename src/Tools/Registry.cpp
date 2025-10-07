#include <windows.h>
#include <vector>
#include <string>
#include <filesystem>

#include "Registry.hpp"
#include "Tools.hpp"

namespace AnyFSE::Tools
{
    namespace fs = std::filesystem;
    // Read string from registry
    // static
    std::wstring Registry::ReadString(const std::wstring &subKey, const std::wstring &valueName, const std::wstring &defaultValue )
    {
        HKEY hOpenedKey;
        std::wstring result = defaultValue;

        if (RegOpenKeyEx(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_READ, &hOpenedKey) == ERROR_SUCCESS)
        {
            DWORD size = 0;
            // Get required buffer size
            if (RegQueryValueEx(hOpenedKey, valueName.c_str(), NULL, NULL, NULL, &size) == ERROR_SUCCESS)
            {
                wchar_t *buffer = new wchar_t[size / sizeof(wchar_t) + 1];
                if (RegQueryValueEx(hOpenedKey, valueName.c_str(), NULL, NULL, (LPBYTE)buffer, &size) == ERROR_SUCCESS)
                {
                    result = buffer;
                }
                delete[] buffer;
            }
            RegCloseKey(hOpenedKey);
        }
        return result;
    }

    // Read DWORD from registry
    //static
    DWORD Registry::ReadDWORD(const std::wstring &subKey, const std::wstring &valueName, DWORD defaultValue)
    {
        HKEY hOpenedKey;
        DWORD result = defaultValue;
        DWORD size = sizeof(DWORD);

        if (RegOpenKeyEx(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_READ, &hOpenedKey) == ERROR_SUCCESS)
        {
            RegQueryValueEx(hOpenedKey, valueName.c_str(), NULL, NULL, (LPBYTE)&result, &size);
            RegCloseKey(hOpenedKey);
        }
        return result;
    }

    // Read boolean from registry
    //static
    bool Registry::ReadBool(const std::wstring &subKey, const std::wstring &valueName, bool defaultValue)
    {
        return ReadDWORD(subKey, valueName, defaultValue ? 1 : 0) != 0;
    }

    std::wstring GetPathFromCommand(const std::wstring &uninstallCommand)
    {
        size_t pos = uninstallCommand.find(L'\"');
        if (pos == std::string::npos)
        {
            return fs::path(uninstallCommand).parent_path().wstring();
        }

        size_t last = uninstallCommand.find(L'\"', pos + 1);
        last = uninstallCommand.find_last_of(L'\\', last);
        std::wstring name = last != std::string::npos ? uninstallCommand.substr(pos + 1, last - pos - 1) : uninstallCommand.substr(pos+1);
       return name;
    }

    std::wstring Registry::GetInstallPath(const std::wstring &displayName)
    {
        std::vector<HKEY> registryRoots = {
            HKEY_LOCAL_MACHINE,
            HKEY_CURRENT_USER
        };

        std::vector<std::wstring> registryPaths = {
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"};

        std::wstring exactDisplayName = Tools::to_lower(displayName);

        for (const auto &root : registryRoots)
        {
            for (const auto &path : registryPaths)
            {
                HKEY hKey;
                if (RegOpenKeyExW(root, path.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
                {
                    continue;
                }

                DWORD subkeyIndex = 0;
                WCHAR subkeyName[255];
                DWORD subkeyNameSize = 255;

                while (RegEnumKeyExW(hKey, subkeyIndex, subkeyName, &subkeyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                {
                    HKEY hSubKey;
                    std::wstring subkeyPath = path + L"\\" + subkeyName;

                    if (RegOpenKeyExW(root, subkeyPath.c_str(), 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
                    {
                        WCHAR displayName[1024];
                        WCHAR uninstallString[1024];
                        DWORD dataSize = sizeof(displayName);
                        DWORD type;

                        if (RegQueryValueExW(hSubKey, L"DisplayName", NULL, &type,
                                            (LPBYTE)displayName, &dataSize) == ERROR_SUCCESS &&
                            type == REG_SZ &&
                            exactDisplayName == Tools::to_lower(displayName))
                        {

                            dataSize = sizeof(uninstallString);
                            if (RegQueryValueExW(hSubKey, L"UninstallString", NULL, &type,
                                                (LPBYTE)uninstallString, &dataSize) == ERROR_SUCCESS &&
                                type == REG_SZ)
                            {
                                RegCloseKey(hSubKey);
                                RegCloseKey(hKey);
                                return GetPathFromCommand(uninstallString);
                            }
                        }
                        RegCloseKey(hSubKey);
                    }

                    subkeyIndex++;
                    subkeyNameSize = 255;
                }
                RegCloseKey(hKey);
            }
        }

        return L"";
    }
}
