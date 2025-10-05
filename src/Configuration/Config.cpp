#include <filesystem>
#include "Config.hpp"
#include "Tools/Registry.hpp"

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

        wstring Config::GetFilename()
    {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        fs::path binary = path;
        return binary.replace_extension(".ini").wstring();
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
        LauncherName = L"Playnite";
        LauncherWindowName = L"Playnite";
        //LauncherIcon = L"C:\\Tools\\Playnite\\Playnite.FullscreenApp.exe";
        LauncherIcon = L"C:\\Users\\Admin\\source\\repos\\ashpynov\\AnyFSE\\media\\Tray.ico";
        LauncherProcessName = L"Playnite.FullscreenApp.exe";
        LauncherStartCommand = L"C:\\Tools\\Playnite\\Playnite.FullscreenApp.exe";
        LauncherStartCommandArgs = L"";

        XBoxProcessName = L"XboxPcApp.exe";

        SilentMode = false;
        AggressiveMode = false;
    }

    wstring Config::GetLauncher()
    {
        wstring launcher = Registry::ReadString(Config::root, L"LauncherPath");
        if (launcher.empty())
        {

        }
        return wstring();
    }

    static wstring GetLauncher();

    void Config::Load()
    {
        Config::SetDefault();
    }

    void Config::Dump()
    {
    }
}
