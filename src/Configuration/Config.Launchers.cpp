#include <filesystem>
#include "Config.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"

namespace AnyFSE::Configuration
{
    const map<LauncherType, LauncherConfig> Config::LauncherConfigsMap = map<LauncherType, LauncherConfig>{
        {
            LauncherType::PlayniteFullscreen, {
                LauncherType::PlayniteFullscreen,
                L"Playnite Fullscreen",
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
                L"Steam Big Picture Mode",
                L"Steam.exe",
                L"steam://open/bigpicture",
                L"steamwebhelper.exe",
                L"Steam Big Picture Mode",
                L"",
                L"",
                L"",
                true // trayable
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

    bool Config::GetLauncherDefaults(const wstring& path, LauncherConfig& out)
    {
        namespace fs = std::filesystem;
        out = LauncherConfig();

        if (path.empty())
        {
            out.Type = None;
            out.Name = L"None";
            return true;
        }

        const wstring exe = Unicode::to_lower(fs::path(path).filename().wstring());
        for (auto it = Config::LauncherConfigsMap.begin(); it != Config::LauncherConfigsMap.end(); ++it)
        {
            if (Unicode::to_lower(it->second.StartCommand) == exe)
            {
                out = it->second;

                out.StartCommand = path;
                if (out.IconFile.empty())
                {
                    out.IconFile = path;
                }
                return true;
            }
        }

        out.Name = fs::path(path).filename().replace_extension("").wstring();
        out.IconFile = path;
        out.StartCommand = path;
        out.ProcessName = fs::path(path).filename().wstring();
        out.IsCustom = true;
        out.Type = Custom;

        return false;

    }
}
