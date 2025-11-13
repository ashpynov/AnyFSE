#include <filesystem>
#include "Configuration/Config.hpp"
#include "Tools/Registry.hpp"
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

        if (out.Type == LauncherType::None
            || out.Type == LauncherType::Xbox
            || out.Type == LauncherType::ArmouryCrate
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
        namespace fs = std::filesystem;

        std::wstring searchDirectory = L"C:\\Program Files\\WindowsApps\\";
        std::wstring name = L"B9ECED6F.ArmouryCrateSE_";
        std::wstring vendor = L"__qmba6cd70vzyy";

        try
        {
            for (const auto &entry : std::filesystem::directory_iterator(searchDirectory))
            {
                std::wstring filename = entry.path().filename().wstring();
                if (filename.find(name) == 0 && filename.find(vendor) == filename.size() - vendor.size())
                {
                    std::wstring ACPath = searchDirectory + filename + L"\\ArmouryCrateSE.exe";
                    if (fs::exists(fs::path(ACPath)))
                    {
                        found.push_back(L"asusac://");
                        return;
                    }
                }
            }
        }
        catch (...)
        {
        }
    }

    bool Config::FindLaunchers(std::list<std::wstring>& found)
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

    void Config::FindXbox(std::list<std::wstring>& found)
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
                    std::wstring XboxPath = searchDirectory + filename + L"\\XboxPcApp.exe";
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
