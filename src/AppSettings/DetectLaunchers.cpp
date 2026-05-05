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

#include <filesystem>
#include <set>
#include <windows.h>
#include "Configuration/Config.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/Packages.hpp"
#include "Tools/List.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Paths.hpp"


namespace AnyFSE::Configuration
{
    namespace fs = std::filesystem;

    void Config::GetStartupConfigured()
    {
        FseOnStartup = IsFseOnStartupConfigured();
        Launcher.StartCommand = IsNativeConfigured()
            ? GetNativePath(Launcher.StartCommand)
            : IsAnyFSEConfigured()
            ? Launcher.StartCommand
            : L"";
    }

    void Config::UpdatePortableLauncher(LauncherConfig & out)
    {

        if (out.Type == None || !out.AppUserModelID.empty())
        {
            return;
        }

        std::list<std::wstring> installed;

        FindInstalledLaunchers(installed);

        if (!std::any_of(installed.begin(), installed.end(),
            [&command = Unicode::to_lower(out.StartCommand) ](const std::wstring& p)
            {
                return command == Unicode::to_lower(p);
            })
        )
        {
            out.Name = out.Name + L" (Portable)";
            out.IsPortable = true;
        }
    }

    bool Config::IsFseOnStartupConfigured()
    {
        return 1 == Registry::ReadDWORD(
            L"Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration",
            L"StartupToGamingHome"
        );
    }

    bool Config::IsNativeConfigured()
    {
        std::wstring appPackageId = Registry::ReadString(
            L"Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration",
            L"GamingHomeApp");
        return !IsAnyFSEConfigured() && !Packages::GetAppxInstallLocation(appPackageId).empty();
    }

    bool Config::IsAnyFSEConfigured()
    {
        return Registry::ReadString(
            L"Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration",
            L"GamingHomeApp") == L"ArtemShpynov.AnyFSE_by4wjhxmygwn4!App";
    }

    std::wstring Config::GetNativePath(const std::wstring& launcher)
    {
        if (IsNativeConfigured())
        {
            return Registry::ReadString(
                L"Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration",
                L"GamingHomeApp");
        }
        return launcher;
    }

    void Config::FindArmouryCrate(std::list<std::wstring>& found)
    {
        std::wstring installPath =
            Packages::GetAppxInstallLocation(L"B9ECED6F.ArmouryCrateSE_qmba6cd70vzyy");

        if (!installPath.empty())
        {
            found.push_back(L"asusac://");
        }
    }

    bool Config::FindLaunchers(std::list<std::wstring> &found)
    {
        size_t existed = found.size();
        found.push_back(L"");
        FindInstalledLaunchers(found);
        return found.size() > existed;
    }

    bool Config::FindInstalledLaunchers(std::list<std::wstring>& found)
    {
        size_t existed = found.size();
        FindPlaynite(found);
        FindSteam(found);
        FindBigBox(found);
        FindOneGameLauncher(found);
        FindRetroBat(found);
        FindKodi(found);
        FindArmouryCrate(found);
        FindNativeLaunchers(found);
        return found.size() > existed;
    }

    bool Config::FindNotInstalledLaunchers(std::list<std::wstring>& found)
    {
        std::list<std::wstring> executables;
        FindInstalledLaunchers(executables);

        std::set<std::wstring> installed;

        for (auto it : executables)
        {
            if (fs::exists(it))
            {
                installed.insert(Unicode::to_lower(fs::path(it).filename().wstring()));
            }
            else
            {
                installed.insert(Unicode::to_lower(it));
            }
        }

        for (auto it = Config::LauncherConfigs.begin(); it != Config::LauncherConfigs.end(); ++it)
        {
            if (installed.find(Unicode::to_lower(it->StartCommand)) == installed.end())
            {
                found.push_back(Unicode::to_lower(it->StartCommand));
            }
        }
        return found.size() > 0;
    }

    void Config::FindPlaynite(std::list<std::wstring>& found)
    {
        std::wstring installPath = GetInstallPath(L"Playnite");
        if (installPath.empty())
        {
            installPath = GetAssociationPath(L"Playnite.ext");
        }
        if (!installPath.empty())
        {
            found.push_back(fs::path(installPath).append(L"Playnite.FullscreenApp.exe").wstring());
            found.push_back(fs::path(installPath).append(L"Playnite.DesktopApp.exe").wstring());
        }
    }

    void Config::FindSteam(std::list<std::wstring>& found)
    {
        std::wstring installPath = GetInstallPath(L"Steam");
        if (!installPath.empty())
        {
            found.push_back(fs::path(installPath).append(L"Steam.exe").wstring());
        }
    }

    void Config::FindBigBox(std::list<std::wstring> &found)
    {
        std::wstring installPath = SearchAppUserModel(L"LaunchBox");
        if (!installPath.empty())
        {
            found.push_back(fs::path(installPath).append(L"BigBox.exe").wstring());
        }
    }

    void Config::FindOneGameLauncher(std::list<std::wstring> &found)
    {
        std::wstring installPath = Packages::GetAppxInstallLocation(L"62269AlexShats.OneGameLauncher_gghb1w55myjr2");
        if (!installPath.empty())
        {
            found.push_back(L"ogl://");
        }
    }

    void Config::FindRetroBat(std::list<std::wstring> &found)
    {
        std::wstring installPath = Registry::ReadString(L"HKCU\\Software\\RetroBat", L"LatestKnownInstallPath");
        if (!installPath.empty())
        {
            found.push_back(fs::path(installPath).append(L"RetroBat.exe").wstring());
        }
    }

    void Config::FindKodi(std::list<std::wstring> &found)
    {
        std::wstring installPath = GetInstallPath(L"Kodi");
        if (!installPath.empty())
        {
            found.push_back(fs::path(installPath).append(L"Kodi.exe").wstring());
        }
    }

    void Config::FindNativeLaunchers(std::list<std::wstring> &found)
    {
        auto launchers = Packages::GetNativeLaunchers();
        for (auto appUserModelId : launchers)
        {
            if (appUserModelId == L"ArtemShpynov.AnyFSE_by4wjhxmygwn4!App" )
                continue;

            if (List::npos == List::index_of_if(
                Config::LauncherConfigs,
                [&appUserModelId](const LauncherConfig& l){return l.AppUserModelID == appUserModelId;}
            ))
            {
                LauncherConfig Native;
                Native.Type = LauncherType::Native;
                Native.Name = Packages::GetAppDisplayName(appUserModelId);
                Native.StartCommand = appUserModelId;
                Native.AppUserModelID = appUserModelId;
                Config::LauncherConfigs.push_back(Native);
            }

            if (List::npos == List::index_of(found, appUserModelId))
            {
                found.push_back(appUserModelId);
            }
        }

    }

    std::wstring Config::GetPathFromCommand(const std::wstring &uninstallCommand)
    {
        std::wstring name;
        size_t pos = uninstallCommand.find(L'\"');
        if (pos != std::string::npos)
        {
            size_t last = uninstallCommand.find(L'\"', pos + 1);
            name = last != std::string::npos ? uninstallCommand.substr(pos + 1, last - pos - 1) : uninstallCommand.substr(pos+1);
        }
        else
        {
            name = fs::path(uninstallCommand).wstring();
        }
        std::replace(name.begin(), name.end(), L'/', L'\\');
        return name;
    }

    std::wstring Config::SearchAppUserModel(const std::wstring &displayName)
    {
        std::wstring path = L"HKCU\\Software\\Classes\\AppUserModelId";
        std::wstring exactDisplayName = Unicode::to_lower(displayName);

        std::wstring actualPath;
        HKEY root = Registry::GetRootKey(path, actualPath);

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
                    return fs::path(GetPathFromCommand(subkeyName)).parent_path().wstring();
                }
            }
            RegCloseKey(hSubKey);
            subkeyIndex++;
            subkeyNameSize = 255;
        }
        RegCloseKey(hKey);
        return L"";
    }

    std::wstring Config::GetAssociationPath(const std::wstring &extName)
    {
        std::wstring path = Registry::ReadString(L"HKCR\\" + extName + L"\\shell\\open\\command", L"", L"");
        if (!path.empty())
        {
            std::wstring exe = GetPathFromCommand(path);
            if (fs::exists(exe))
            {
                return fs::path(exe).parent_path().wstring();
            }
        }
        return L"";
    }

    std::wstring Config::GetInstallPath(const std::wstring &displayName)
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
            HKEY root = Registry::GetRootKey(path, actualPath);

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
                        )
                        {
                            std::wstring binary = GetPathFromCommand(uninstallString);
                            if ( !fs::exists(binary))
                            {
                                continue;
                            }
                            RegCloseKey(hSubKey);
                            RegCloseKey(hKey);
                            return fs::path(binary).parent_path().wstring();
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
