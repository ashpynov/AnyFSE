#include <filesystem>
#include "Config.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Tools.hpp"

namespace AnyFSE::Configuration
{
    const wstring Config::root = L"Software\\AnyFSE\\Settings";

    namespace fs = std::filesystem;

    wstring Config::LauncherName;
    wstring Config::LauncherWindowName;
    wstring Config::LauncherIcon;
    wstring Config::LauncherProcessName;
    wstring Config::LauncherStartCommand;
    wstring Config::LauncherStartCommandArgs;
    wstring Config::XBoxProcessName;

    bool Config::AggressiveMode = false;
    bool Config::SilentMode = false;

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

    void Config::Load()
    {
        SetDefault();

        wstring launcher = GetCurrentLauncher();
        if (launcher.empty())
        {
            return;
        }
        LauncherConfig config;
        GetLauncherSettings(launcher, config);
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
