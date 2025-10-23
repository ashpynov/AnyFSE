#include <filesystem>
#include "Configuration/Config.hpp"
#include "Registry.hpp"
#include "Tools/Unicode.hpp"

namespace AnyFSE::Configuration
{
    void Config::GetStartupConfigured()
    {
        Launcher.StartCommand = GetXboxPath(Launcher.StartCommand);
        FseOnStartup = IsXboxConfigured() && IsFseOnStartupConfigured();
    }

    void Config::UpdatePortableLauncher(LauncherConfig & out)
    {
        list<wstring> installed;
        FindInstalledLaunchers(installed); // Portable ??

        if (!std::any_of(installed.begin(), installed.end(),
            [&command = Unicode::to_lower(out.StartCommand) ](const wstring& p)
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

    wstring Config::GetXboxPath(const std::wstring& launcher)
    {
        if (!launcher.empty())
        {
            return launcher;
        }

        if (IsXboxConfigured())
        {
            list<wstring> xbox;
            FindXbox(xbox);
            if (xbox.size())
            {
                return xbox.front();
            }
        }
        return L"";
    }

    void Config::FindArmoryCrate(list<wstring>& found)
    {

    }

    bool Config::FindLaunchers(list<wstring>& found)
    {
        size_t existed = found.size();
        found.push_back(L"");
        FindLocal(found);
        FindInstalledLaunchers(found);
        return found.size() > existed;
    }

    bool Config::FindInstalledLaunchers(list<wstring>& found)
    {
        size_t existed = found.size();
        FindPlaynite(found);
        FindSteam(found);
        FindXbox(found);
        FindArmoryCrate(found);
        return found.size() > existed;
    }

    void Config::FindLocal(list<wstring>& found)
    {
        namespace fs = std::filesystem;
        wstring modulePath = Config::GetModulePath();
        if (fs::exists(fs::path(modulePath + L"\\Playnite.FullscreenApp.exe")))
        {
            found.push_back(modulePath + L"\\Playnite.FullscreenApp.exe");
            found.push_back(modulePath + L"\\Playnite.DesktopApp.exe");
        }
    }

    void Config::FindPlaynite(list<wstring>& found)
    {
        wstring installPath = Registry::GetInstallPath(L"Playnite");
        if (!installPath.empty())
        {
            found.push_back(installPath + L"\\Playnite.FullscreenApp.exe");
            found.push_back(installPath + L"\\Playnite.DesktopApp.exe");
        }
    }

    void Config::FindSteam(list<wstring>& found)
    {
        wstring installPath = Registry::GetInstallPath(L"Steam");
        if (!installPath.empty())
        {
            found.push_back(installPath + L"\\Steam.exe");
        }
    }

    void Config::FindXbox(list<wstring>& found)
    {
        namespace fs = std::filesystem;

        std::wstring searchDirectory = L"C:\\Program Files\\WindowsApps\\";
        std::wstring name = L"Microsoft.GamingApp_";
        std::wstring vendor = L"__8wekyb3d8bbwe";

        try
        {
            for (const auto &entry : std::filesystem::directory_iterator(searchDirectory))
            {
                std::wstring filename = entry.path().filename().wstring();
                if (filename.find(name) == 0 && filename.find(vendor) == filename.size() - vendor.size())
                {
                    wstring XboxPath = searchDirectory + filename + L"\\XboxPcApp.exe";
                    if (fs::exists(fs::path(XboxPath)))
                    {
                        found.push_back(XboxPath);
                        return;
                    }
                }
            }
        }
        catch (...)
        {
        }
    }
}
