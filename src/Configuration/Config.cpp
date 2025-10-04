#include <filesystem>
#include "Config.hpp"

namespace AnyFSE::Configuration
{
    namespace fs = std::filesystem;
    
    wstring Config::LauncherName;
    wstring Config::LauncherWindowName;
    wstring Config::LauncherIcon;
    wstring Config::LauncherProcessName;
    wstring Config::LauncherStartCommand;
    wstring Config::LauncherStartCommandArgs;
    wstring Config::XBoxProcessName;
    bool Config::SilentMode = false;

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
        LauncherIcon = L"C:\\Tools\\Playnite\\Playnite.FullscreenApp.exe";        
        LauncherProcessName = L"Playnite.FullscreenApp.exe";
        LauncherStartCommand = L"C:\\Tools\\Playnite\\Playnite.FullscreenApp.exe";
        LauncherStartCommandArgs = L"";

        XBoxProcessName = L"XboxPcApp.exe";

        SilentMode = false;
    }

    void Config::Load()
    {
        Config::SetDefault();
    }

    void Config::Dump()
    {
    }
}
