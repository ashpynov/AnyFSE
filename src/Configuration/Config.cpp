#include <filesystem>
#include "Config.h"

namespace AnyFSE::Configuration
{
    namespace fs = std::filesystem;
    
    wstring Config::LauncherName;
    wstring Config::LauncherWindowName;
    wstring Config::LauncherLogoPath;
    wstring Config::LauncherIcon;
    wstring Config::LauncherProcessName;
    wstring Config::LauncherStartCommand;
    wstring Config::LauncherStartCommandArgs;
    wstring Config::XBoxProcessName;
    bool Config::ShowLogo = true;
    bool Config::ShowText = true;
    bool Config::ShowAnimation = true;
    COLORREF Config::BackgroundColor;
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
        LauncherLogoPath = L"C:\\Users\\Admin\\source\\repos\\ashpynov\\AnyFSE\\media\\playnite-logo-white-150.png";
        LauncherIcon = L"C:\\Tools\\Playnite\\Playnite.FullscreenApp.exe";        
        LauncherProcessName = L"Playnite.FullscreenApp.exe";
        LauncherStartCommand = L"C:\\Tools\\Playnite\\Playnite.FullscreenApp.exe";
        LauncherStartCommandArgs = L"";

        XBoxProcessName = L"XBoxPCApp.exe";

        ShowLogo = true;
        ShowText = true;
        ShowAnimation = true;
        BackgroundColor = RGB(32, 32, 32);
        
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
