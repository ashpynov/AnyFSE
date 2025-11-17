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
#include <vector>
#include <string>
#include <filesystem>

#include "Registry.hpp"
#include "Tools/Tools.hpp"
#include "Tools/Unicode.hpp"

namespace AnyFSE::Tools
{
    namespace fs = std::filesystem;

    HKEY Registry::GetRootKey(const std::wstring &subKey, std::wstring &actualPath)
    {
        // Extract root key from subKey (e.g., "HKEY_CURRENT_USER\\Software\\MyApp" -> HKEY_CURRENT_USER)
        size_t pos = subKey.find('\\');
        if (pos == std::wstring::npos)
        {
            actualPath = L"";
            return HKEY_CURRENT_USER; // Default root
        }

        std::wstring root = subKey.substr(0, pos);
        actualPath = subKey.substr(pos + 1);

        if (root == L"HKEY_CURRENT_USER" || root == L"HKCU")
            return HKEY_CURRENT_USER;
        if (root == L"HKEY_LOCAL_MACHINE" || root == L"HKLM")
            return HKEY_LOCAL_MACHINE;
        if (root == L"HKEY_CLASSES_ROOT" || root == L"HKCR")
            return HKEY_CLASSES_ROOT;
        if (root == L"HKEY_USERS")
            return HKEY_USERS;
        if (root == L"HKEY_CURRENT_CONFIG")
            return HKEY_CURRENT_CONFIG;

        // Default to HKEY_CURRENT_USER
        actualPath = subKey;
        return HKEY_CURRENT_USER;
    }

    // Read string from registry
    // static
    std::wstring Registry::ReadString(const std::wstring &subKey, const std::wstring &valueName, const std::wstring &defaultValue )
    {
        HKEY hOpenedKey;
        std::wstring result = defaultValue;
        std::wstring actualPath;
        HKEY rootKey = GetRootKey(subKey, actualPath);

        if (RegOpenKeyEx(rootKey, actualPath.c_str(), 0, KEY_READ, &hOpenedKey) == ERROR_SUCCESS)
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

        std::wstring actualPath;
        HKEY rootKey = GetRootKey(subKey, actualPath);

        if (RegOpenKeyEx(rootKey, actualPath.c_str(), 0, KEY_READ, &hOpenedKey) == ERROR_SUCCESS)
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

    bool Registry::WriteString(const std::wstring &subKey, const std::wstring &valueName, const std::wstring &value)
    {
        std::wstring actualPath;
        HKEY rootKey = GetRootKey(subKey, actualPath);

        HKEY hKey;
        LONG result = RegCreateKeyExW(rootKey, actualPath.c_str(), 0, nullptr,
                                      REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
        if (result != ERROR_SUCCESS)
            return false;

        result = RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ,
                                reinterpret_cast<const BYTE *>(value.c_str()),
                                (DWORD)((value.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);

        return (result == ERROR_SUCCESS);
    }

    bool Registry::WriteDWORD(const std::wstring &subKey, const std::wstring &valueName, DWORD value)
    {
        std::wstring actualPath;
        HKEY rootKey = GetRootKey(subKey, actualPath);

        HKEY hKey;
        LONG result = RegCreateKeyExW(rootKey, actualPath.c_str(), 0, nullptr,
                                      REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
        if (result != ERROR_SUCCESS)
            return false;

        result = RegSetValueExW(hKey, valueName.c_str(), 0, REG_DWORD,
                                reinterpret_cast<const BYTE *>(&value), sizeof(DWORD));
        RegCloseKey(hKey);

        return (result == ERROR_SUCCESS);
    }

    bool Registry::WriteBool(const std::wstring &subKey, const std::wstring &valueName, bool value)
    {
        return WriteDWORD(subKey, valueName, value ? 1 : 0);
    }

    bool Registry::WriteBinary(const std::wstring &subKey, const std::wstring &valueName, const BYTE *data, DWORD size)
    {
        std::wstring actualPath;
        HKEY rootKey = GetRootKey(subKey, actualPath);

        HKEY hKey;
        LONG result = RegCreateKeyExW(rootKey, actualPath.c_str(), 0, nullptr,
                                      REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
        if (result != ERROR_SUCCESS)
            return false;

        result = RegSetValueExW(hKey, valueName.c_str(), 0, REG_BINARY, data, size);
        RegCloseKey(hKey);

        return (result == ERROR_SUCCESS);
    }

    bool Registry::DeleteValue(const std::wstring &subKey, const std::wstring &valueName)
    {
        std::wstring actualPath;
        HKEY rootKey = GetRootKey(subKey, actualPath);

        HKEY hKey;
        LONG result = RegOpenKeyExW(rootKey, actualPath.c_str(), 0, KEY_WRITE, &hKey);
        if (result != ERROR_SUCCESS)
            return false;

        result = RegDeleteValueW(hKey, valueName.c_str());
        RegCloseKey(hKey);

        return (result == ERROR_SUCCESS);
    }

    bool Registry::DeleteKey(const std::wstring &subKey)
    {
        std::wstring actualPath;
        HKEY rootKey = GetRootKey(subKey, actualPath);

        // Recursive delete for the key and all subkeys
        return (RegDeleteTreeW(rootKey, actualPath.c_str()) == ERROR_SUCCESS);
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

    std::wstring Registry::SearchAppUserModel(const std::wstring &displayName)
    {
        std::wstring path = L"HKCU\\Software\\Classes\\AppUserModelId";
        std::wstring exactDisplayName = Unicode::to_lower(displayName);

        std::wstring actualPath;
        HKEY root = GetRootKey(path, actualPath);

        HKEY hKey;
        if (RegOpenKeyExW(root, actualPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        {
            return L"";
        }

        DWORD subkeyIndex = 0;
        WCHAR subkeyName[255];
        DWORD subkeyNameSize = 255;

        while (RegEnumKeyExW(hKey, subkeyIndex, subkeyName, &subkeyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            HKEY hSubKey;
            std::wstring subkeyPath = actualPath + L"\\" + subkeyName;

            if (RegOpenKeyExW(root, subkeyPath.c_str(), 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
            {
                WCHAR displayName[1024];
                DWORD dataSize = sizeof(displayName);
                DWORD type;

                if (RegQueryValueExW(hSubKey, L"DisplayName", NULL, &type,
                                    (LPBYTE)displayName, &dataSize) == ERROR_SUCCESS
                    && type == REG_SZ
                    && exactDisplayName == Unicode::to_lower(displayName)
                    && fs::exists(subkeyName)
                )
                {
                    RegCloseKey(hSubKey);
                    RegCloseKey(hKey);
                    return GetPathFromCommand(subkeyName);
                }
            }
            RegCloseKey(hSubKey);
            subkeyIndex++;
            subkeyNameSize = 255;
        }
        RegCloseKey(hKey);
        return L"";
    }

    std::wstring Registry::GetInstallPath(const std::wstring &displayName)
    {
        std::vector<std::wstring> registryPaths = {
            L"HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            L"HKCU\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            L"HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"};

        std::wstring exactDisplayName = Unicode::to_lower(displayName);

        for (const auto &path : registryPaths)
        {
            std::wstring actualPath;
            HKEY root = GetRootKey(path, actualPath);

            HKEY hKey;
            if (RegOpenKeyExW(root, actualPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            {
                continue;
            }

            DWORD subkeyIndex = 0;
            WCHAR subkeyName[255];
            DWORD subkeyNameSize = 255;

            while (RegEnumKeyExW(hKey, subkeyIndex, subkeyName, &subkeyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                HKEY hSubKey;
                std::wstring subkeyPath = actualPath + L"\\" + subkeyName;

                if (RegOpenKeyExW(root, subkeyPath.c_str(), 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
                {
                    WCHAR displayName[1024];
                    WCHAR uninstallString[1024];
                    DWORD dataSize = sizeof(displayName);
                    DWORD type;

                    if (RegQueryValueExW(hSubKey, L"DisplayName", NULL, &type,
                                        (LPBYTE)displayName, &dataSize) == ERROR_SUCCESS
                        && type == REG_SZ
                        && exactDisplayName == Unicode::to_lower(displayName))
                    {

                        dataSize = sizeof(uninstallString);
                        if (RegQueryValueExW(hSubKey, L"UninstallString", NULL, &type,
                                            (LPBYTE)uninstallString, &dataSize) == ERROR_SUCCESS 
                            && type == REG_SZ
                            && fs::exists(uninstallString)
                        )
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

        return L"";
    }
}
