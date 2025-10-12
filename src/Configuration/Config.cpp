#include <filesystem>
#include "Config.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Tools.hpp"

namespace AnyFSE::Configuration
{
    const wstring Config::root = L"Software\\AnyFSE\\Settings";

    namespace fs = std::filesystem;

    LauncherType Config::Type;
    wstring Config::LauncherName;
    wstring Config::LauncherWindowName;
    wstring Config::LauncherIcon;
    wstring Config::LauncherProcessName;
    wstring Config::LauncherStartCommand;
    wstring Config::LauncherStartCommandArgs;
    wstring Config::XBoxProcessName;

    bool Config::AggressiveMode = false;
    bool Config::SilentMode = false;
    bool Config::FseOnStartup = false;

    wstring Config::GetModulePath()
    {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        fs::path binary = path;
        return binary.parent_path().wstring();
    }

    wstring Config::GetFilename()
    {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        fs::path binary = path;
        return binary.replace_extension(".ini").wstring();
    }

    void Config::SetDefault()
    {
        XBoxProcessName = Registry::ReadString(Config::root, L"XBoxProcessName", L"XboxPcApp.exe");
        AggressiveMode = Registry::ReadBool(Config::root, L"AggressiveMode", false);
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
    void Config::Load()
    {
        SetDefault();

        FseOnStartup = IsXboxConfigured() && IsFseOnStartupConfigured();

        wstring launcher = GetCurrentLauncher();

        LauncherConfig config;
        GetLauncherSettings(launcher, config);
        Type = config.Type;
        LauncherName = config.Name;
        LauncherWindowName = config.WindowTitle;
        LauncherProcessName = config.ProcessName;
        LauncherStartCommand = config.StartCommand;
        LauncherStartCommandArgs = config.StartArg;
        LauncherIcon = config.IconFile;
    }

    void Config::Dump()
    {
    }

}
