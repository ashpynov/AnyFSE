#include <filesystem>
#include "Config.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Tools.hpp"

namespace AnyFSE::Configuration
{
    const map<LauncherType, LauncherConfig> Config::LauncherConfigs = map<LauncherType, LauncherConfig>{
        {
            LauncherType::PlayniteFullscreen, {
                LauncherType::PlayniteFullscreen,
                L"Playnite Fulscreen",
                L"Playnite.FullscreenApp.exe",
                L"",
                L"Playnite.FullscreenApp.exe",
                L"Playnite",
                L"Playnite.DesktopApp.exe",
                L"Playnite",
                L""
            }
        },
        {
            LauncherType::PlayniteDesktop, {
                LauncherType::PlayniteDesktop,
                L"Playnite Desktop",
                L"Playnite.DesktopApp.exe",
                L"",
                L"Playnite.DesktopApp.exe",
                L"Playnite",
                L"Playnite.FullscreenApp.exe",
                L"Playnite",
                L""
            }
        },
        {
            LauncherType::Steam, {
                LauncherType::Steam,
                L"Steam",
                L"Steam.exe",
                L"",
                L"Steam.exe",
                L"",
                L"",
                L"",
                L""
            }
        },
        {
            LauncherType::Xbox, {
                LauncherType::Xbox,
                L"Xbox",
                L"XBoxPcApp.exe",
                L"",
                L"",
                L"",
                L"",
                L"",
                L""
            }
        }
    };

    namespace fs = std::filesystem;

    wstring Config::GetCurrentLauncher()
    {
        wstring launcher = Registry::ReadString(Config::root, L"LauncherPath", L"C:\\Tools\\Playnite\\Playnite.FullscreenApp.exe");
        if (launcher.empty())
        {
            list<wstring> launchers;
            if (FindLaunchers(launchers))
            {
                return launchers.front();
            }
        }
        return launcher;
    }

    bool Config::FindLaunchers(list<wstring>& found)
    {
        size_t existed = found.size();
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
        wstring XboxPath = L"C:\\Program Files\\WindowsApps\\Microsoft.GamingApp_2510.1001.36.0_x64__8wekyb3d8bbwe\\XboxPcApp.exe";

        if (fs::exists(fs::path(XboxPath)))
        {
            found.push_back(XboxPath);
        }
    }

    void Config::FindArmoryCrate(list<wstring>& found)
    {

    }

    bool Config::GetLauncherDefaults(const wstring& path, LauncherConfig& out)
    {
        out = LauncherConfig();
        const wstring exe = Tools::to_lower(fs::path(path).filename().wstring());
        for (auto it = Config::LauncherConfigs.begin(); it != Config::LauncherConfigs.end(); ++it)
        {
            if (Tools::to_lower(it->second.StartCommand) == exe)
            {
                out = it->second;

                out.StartCommand = path;
                if (out.IconFile.empty())
                {
                    out.IconFile = path;
                }

                list<wstring> installed;
                FindInstalledLaunchers(installed);

                if (!std::any_of(installed.begin(), installed.end(),
                    [&command = Tools::to_lower(out.StartCommand) ](const wstring& p)
                    {
                        return command == Tools::to_lower(p);
                    })
                )
                {
                    out.Name = out.Name + L" (Portable)";
                    out.isPortable = true;
                }
                return true;
            }
        }

        out.Name = fs::path(path).parent_path().filename().wstring();
        if (out.Name.empty() || out.Name == L"bin")
        {
            out.Name = fs::path(path).filename().replace_extension("").wstring();
        }
        out.StartCommand = path;
        out.ProcessName = fs::path(path).filename().wstring();
        out.isCustom = true;
        out.Type = Custom;

        return false;

    }

    bool Config::GetLauncherSettings(const wstring& path, LauncherConfig& out)
    {
        GetLauncherDefaults(path, out);
        if (out.Type == Custom || Registry::ReadBool(root, L"CustomSettings", out.isCustom))
        {
            out.StartCommand    = Registry::ReadString(root, L"StartCommand",   out.StartCommand);
            out.StartArg        = Registry::ReadString(root, L"StartArg",       out.StartArg);
            out.ProcessName     = Registry::ReadString(root, L"ProcessName",    out.ProcessName);
            out.WindowTitle     = Registry::ReadString(root, L"WindowTitle",    out.WindowTitle);
            out.ProcessNameAlt  = Registry::ReadString(root, L"ProcessNameAlt", out.ProcessNameAlt);
            out.WindowTitleAlt  = Registry::ReadString(root, L"WindowTitleAlt", out.WindowTitleAlt);
            out.IconFile        = Registry::ReadString(root, L"IconFile",       out.IconFile);
        }
        return true;
    }
}
