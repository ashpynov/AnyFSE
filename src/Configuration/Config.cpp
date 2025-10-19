#include <filesystem>
#include "Config.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"

namespace AnyFSE::Configuration
{
    const wstring Config::Root = L"Software\\AnyFSE\\Settings";

    namespace fs = std::filesystem;

    LauncherType Config::Type;
    wstring Config::LauncherName;
    wstring Config::LauncherWindowName;
    wstring Config::LauncherIcon;
    bool    Config::LauncherCustomSettings;
    wstring Config::LauncherProcessName;
    wstring Config::LauncherStartCommand;
    wstring Config::LauncherStartCommandArgs;
    wstring Config::XBoxProcessName;
    wstring Config::LauncherProcessNameAlt;
    wstring Config::LauncherWindowNameAlt;
    

    bool Config::LauncherIsTrayAggressive = false;

    bool Config::AggressiveMode = false;
    bool Config::SilentMode = false;
    bool Config::FseOnStartup = false;

    bool Config::ShowAnimation = true;
    bool Config::ShowLogo = true;
    bool Config::ShowText = true;

    wstring Config::GetModulePath()
    {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        fs::path binary = path;
        return binary.parent_path().wstring();
    }

     void Config::SetDefault()
    {
        XBoxProcessName = Registry::ReadString(Config::Root, L"XBoxProcessName", L"XboxPcApp.exe");
        AggressiveMode = Registry::ReadBool(Config::Root, L"AggressiveMode", false);
        SilentMode = false;
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
    
    bool Config::IsConfigured()
    {
        wstring launcher = Registry::ReadString(Config::Root, L"LauncherPath");
        return !launcher.empty();
    }
    
    void Config::Load()
    {
        SetDefault();

        FseOnStartup = IsXboxConfigured() && IsFseOnStartupConfigured();

        wstring launcher = GetCurrentLauncher();

        LauncherConfig config;
        GetLauncherSettings(launcher, config);
        Type = config.Type;
        LauncherName = config.Name;
        LauncherStartCommand = config.StartCommand;
        LauncherCustomSettings = Registry::ReadBool(Root, L"CustomSettings", config.isCustom);
        LauncherWindowName = config.WindowTitle;
        LauncherWindowNameAlt = config.WindowTitleAlt;
        LauncherProcessName = config.ProcessName;
        LauncherProcessNameAlt = config.ProcessNameAlt;
        LauncherStartCommandArgs = config.StartArg;
        LauncherIcon = config.IconFile;
        LauncherIsTrayAggressive = config.isTrayAggressive;
    }
}
