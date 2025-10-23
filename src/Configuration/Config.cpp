#include <filesystem>
#include "Config.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"

namespace AnyFSE::Configuration
{
    const wstring Config::Root = L"Software\\AnyFSE\\Settings";

    namespace fs = std::filesystem;

    LogLevels       Config::LogLevel = LogLevels::Disabled;
    wstring         Config::LogPath = L"";
    bool            Config::CustomSettings;
    LauncherConfig  Config::Launcher;
    wstring         Config::XBoxProcessName;
    bool            Config::AggressiveMode = false;
    bool            Config::SilentMode = false;
    bool            Config::FseOnStartup = false;
    bool            Config::ShowAnimation = true;
    bool            Config::ShowLogo = true;
    bool            Config::ShowText = true;

    wstring Config::GetModulePath()
    {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        fs::path binary = path;
        return binary.parent_path().wstring();
    }

    bool Config::IsConfigured()
    {
        wstring launcher = Registry::ReadString(Config::Root, L"LauncherPath");
        return !launcher.empty();
    }

    void Config::Load()
    {
        // LogLevel =
        // LogPath =

        XBoxProcessName = Registry::ReadString(Config::Root, L"XBoxProcessName", L"XboxPcApp.exe");
        AggressiveMode = Registry::ReadBool(Config::Root, L"AggressiveMode", false);
        SilentMode = false;
        FseOnStartup = true;

        Launcher.StartCommand = Registry::ReadString(Root, L"LauncherPath");

        // GetStartupConfigured();  // TODO: Move to settings!!

        LoadLauncherSettings(Launcher.StartCommand, Launcher);
        CustomSettings = Registry::ReadBool(Root, L"CustomSettings", Launcher.IsCustom);

    }

    bool Config::LoadLauncherSettings(const wstring& path, LauncherConfig& out)
    {
        GetLauncherDefaults(path, out);
        if (out.Type == Custom || Registry::ReadBool(Root, L"CustomSettings", out.IsCustom))
        {
            out.StartCommand    = Registry::ReadString(Root, L"StartCommand",   out.StartCommand);
            out.StartArg        = Registry::ReadString(Root, L"StartArg",       out.StartArg);
            out.ProcessName     = Registry::ReadString(Root, L"ProcessName",    out.ProcessName);
            out.WindowTitle     = Registry::ReadString(Root, L"WindowTitle",    out.WindowTitle);
            out.ProcessNameAlt  = Registry::ReadString(Root, L"ProcessNameAlt", out.ProcessNameAlt);
            out.WindowTitleAlt  = Registry::ReadString(Root, L"WindowTitleAlt", out.WindowTitleAlt);
            out.IconFile        = Registry::ReadString(Root, L"IconFile",       out.IconFile);
        }
        return true;
    }

    void Config::Save()
    {
        const std::wstring anyFSERoot = L"HKCU\\Software\\AnyFSE\\Settings";

        Registry::WriteString(anyFSERoot, L"LauncherPath", Config::Launcher.StartCommand);
        Registry::WriteBool(anyFSERoot,   L"CustomSettings", Config::CustomSettings);
        Registry::WriteString(anyFSERoot, L"StartCommand", Config::Launcher.StartCommand);
        Registry::WriteString(anyFSERoot, L"StartArg", Config::Launcher.StartArg);
        Registry::WriteString(anyFSERoot, L"ProcessName", Config::Launcher.ProcessName);
        Registry::WriteString(anyFSERoot, L"WindowTitle", Config::Launcher.WindowTitle);
        Registry::WriteString(anyFSERoot, L"ProcessNameAlt", Config::Launcher.ProcessNameAlt);
        Registry::WriteString(anyFSERoot, L"WindowTitleAlt", Config::Launcher.WindowTitleAlt);
        Registry::WriteString(anyFSERoot, L"IconFile", Config::Launcher.IconFile);
    }
}
