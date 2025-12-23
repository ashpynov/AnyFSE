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
#include <windows.h>
#include "Configuration/Config.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/Packages.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Paths.hpp"

namespace AnyFSE::Configuration
{
    namespace fs = std::filesystem;

    void Config::GetStartupConfigured()
    {
        FseOnStartup = IsXboxConfigured() && IsFseOnStartupConfigured();
        Launcher.StartCommand = FseOnStartup ? GetXboxPath(Launcher.StartCommand) : Launcher.StartCommand;
    }

    void Config::UpdatePortableLauncher(LauncherConfig & out)
    {

        if (out.Type == LauncherType::None
            || out.Type == LauncherType::Xbox
            || out.Type == LauncherType::ArmouryCrate
            || out.Type == LauncherType::OneGameLauncher
            || out.StartCommand.find(L'!') != std::wstring::npos)
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

    bool Config::IsXboxConfigured()
    {
        return Registry::ReadString(
            L"Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration",
            L"GamingHomeApp") == L"Microsoft.GamingApp_8wekyb3d8bbwe!Microsoft.Xbox.App";
    }

    std::wstring Config::GetXboxPath(const std::wstring& launcher)
    {
        if (!launcher.empty())
        {
            return launcher;
        }

        if (IsXboxConfigured())
        {
            std::list<std::wstring> xbox;
            FindXbox(xbox);
            if (xbox.size())
            {
                return xbox.front();
            }
        }
        return L"";
    }

    void Config::FindArmoryCrate(std::list<std::wstring>& found)
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
        FindLocal(found);
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
        FindArmoryCrate(found);
        FindXbox(found);
        return found.size() > existed;
    }

    void Config::FindLocal(std::list<std::wstring>& found)
    {
        namespace fs = std::filesystem;
        std::wstring modulePath = Tools::Paths::GetExePath();
        if (fs::exists(fs::path(modulePath + L"\\Playnite.FullscreenApp.exe")))
        {
            found.push_back(modulePath + L"\\Playnite.FullscreenApp.exe");
            found.push_back(modulePath + L"\\Playnite.DesktopApp.exe");
        }
    }

    void Config::FindPlaynite(std::list<std::wstring>& found)
    {
        std::wstring installPath = GetInstallPath(L"Playnite");
        if (!installPath.empty())
        {
            found.push_back(installPath + L"\\Playnite.FullscreenApp.exe");
            found.push_back(installPath + L"\\Playnite.DesktopApp.exe");
        }
    }

    void Config::FindSteam(std::list<std::wstring>& found)
    {
        std::wstring installPath = GetInstallPath(L"Steam");
        if (!installPath.empty())
        {
            found.push_back(installPath + L"\\Steam.exe");
        }
    }

    void Config::FindBigBox(std::list<std::wstring> &found)
    {
        std::wstring installPath = SearchAppUserModel(L"LaunchBox");
        if (!installPath.empty())
        {
            found.push_back(installPath + L"\\BigBox.exe");
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
            found.push_back(installPath + L"\\RetroBat.exe");
        }
    }

    void Config::FindXbox(std::list<std::wstring> &found)
    {
        namespace fs = std::filesystem;

        std::wstring installPath = Packages::GetAppxInstallLocation(L"Microsoft.GamingApp_8wekyb3d8bbwe");
        if (!installPath.empty())
        {
            found.push_back(installPath + L"\\XboxPcApp.exe");
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
