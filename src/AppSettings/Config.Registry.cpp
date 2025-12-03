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
#include "Configuration/Config.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/Packages.hpp"

namespace AnyFSE::Configuration
{
    void Config::GetStartupConfigured()
    {
        FseOnStartup = IsXboxConfigured() && IsFseOnStartupConfigured();
        Launcher.StartCommand = FseOnStartup ? GetXboxPath(Launcher.StartCommand) : L"";
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
        std::wstring modulePath = Config::GetModulePath();
        if (fs::exists(fs::path(modulePath + L"\\Playnite.FullscreenApp.exe")))
        {
            found.push_back(modulePath + L"\\Playnite.FullscreenApp.exe");
            found.push_back(modulePath + L"\\Playnite.DesktopApp.exe");
        }
    }

    void Config::FindPlaynite(std::list<std::wstring>& found)
    {
        std::wstring installPath = Registry::GetInstallPath(L"Playnite");
        if (!installPath.empty())
        {
            found.push_back(installPath + L"\\Playnite.FullscreenApp.exe");
            found.push_back(installPath + L"\\Playnite.DesktopApp.exe");
        }
    }

    void Config::FindSteam(std::list<std::wstring>& found)
    {
        std::wstring installPath = Registry::GetInstallPath(L"Steam");
        if (!installPath.empty())
        {
            found.push_back(installPath + L"\\Steam.exe");
        }
    }

    void Config::FindBigBox(std::list<std::wstring> &found)
    {
        std::wstring installPath = Registry::SearchAppUserModel(L"LaunchBox");
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
}
