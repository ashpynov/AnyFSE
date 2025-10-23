#include <filesystem>
#include <fstream>
#include <sstream>
#include "Config.hpp"
#include "Tools/Unicode.hpp"
#include "nlohmann/json.hpp"

namespace nlohmann
{
    template<>
    struct adl_serializer<std::wstring> {
        static void to_json(json& j, const std::wstring& value) {
            j = Unicode::to_string(value);
        }

        static void from_json(const json& j, std::wstring& value) {
            std::string narrow_str = j.get<std::string>();
            value = Unicode::to_wstring(narrow_str);
        }
    };
}

namespace AnyFSE::Configuration
{
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
        //wstring launcher = Registry::ReadString(Config::Root, L"LauncherPath");
        //return !launcher.empty();
        return false;
    }

    std::string Config::GetConfigFileA()
    {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        return fs::path(path).parent_path().string() + "\\AnyFSE.json";
    }

    void Config::Load()
    {
        // LogLevel =
        // LogPath =

        json config = json::parse("{}");
        if (fs::exists(GetConfigFileA()))
        {
            try
            {
                std::ifstream j(GetConfigFileA());
                config = json::parse(j);
            }
            catch(...) {}
        }

        SilentMode = false;
        FseOnStartup = true;

        AggressiveMode          = config.value("AggressiveMode",  false);
        XBoxProcessName         = config.value("XBoxProcessName", std::wstring(L"XboxPcApp.exe"));
        Launcher.StartCommand   = config.value("LauncherPath",    std::wstring());

        // GetStartupConfigured();  // TODO: Move to settings!!

        LoadLauncherSettings(config, Launcher.StartCommand, Launcher);
        CustomSettings = config.value("CustomSettings", Launcher.IsCustom);

    }

    bool Config::LoadLauncherSettings(const json& config, const wstring& path, LauncherConfig& out)
    {
        GetLauncherDefaults(path, out);
        if (out.Type == Custom || config.value("CustomSettings", out.IsCustom))
        {
            out.StartCommand    = config.value("StartCommand",   out.StartCommand);
            out.StartArg        = config.value("StartArg",       out.StartArg);
            out.ProcessName     = config.value("ProcessName",    out.ProcessName);
            out.WindowTitle     = config.value("WindowTitle",    out.WindowTitle);
            out.ProcessNameAlt  = config.value("ProcessNameAlt", out.ProcessNameAlt);
            out.WindowTitleAlt  = config.value("WindowTitleAlt", out.WindowTitleAlt);
            out.IconFile        = config.value("IconFile",       out.IconFile);
        }
        return true;
    }

    bool Config::LoadLauncherSettings(const wstring& path, LauncherConfig& out)
    {

        // TODO  - is it realy need?
        json config = json::parse("{}");
        if (fs::exists(GetConfigFileA()))
        {
            try
            {
                std::ifstream j(GetConfigFileA());
                config = json::parse(j);
            }
            catch(...) {}
        }

        GetLauncherDefaults(path, out);
        if (out.Type == Custom || config.value("CustomSettings", out.IsCustom))
        {
            out.StartCommand    = config.value("StartCommand",   out.StartCommand);
            out.StartArg        = config.value("StartArg",       out.StartArg);
            out.ProcessName     = config.value("ProcessName",    out.ProcessName);
            out.WindowTitle     = config.value("WindowTitle",    out.WindowTitle);
            out.ProcessNameAlt  = config.value("ProcessNameAlt", out.ProcessNameAlt);
            out.WindowTitleAlt  = config.value("WindowTitleAlt", out.WindowTitleAlt);
            out.IconFile        = config.value("IconFile",       out.IconFile);
        }
        return true;
    }

    void Config::Save()
    {
        json config;

        config["LauncherPath"]      = Config::Launcher.StartCommand;
        config["CustomSettings"]    = Config::CustomSettings;
        config["StartCommand"]      = Config::Launcher.StartCommand;
        config["StartArg"]          = Config::Launcher.StartArg;
        config["ProcessName"]       = Config::Launcher.ProcessName;
        config["WindowTitle"]       = Config::Launcher.WindowTitle;
        config["ProcessNameAlt"]    = Config::Launcher.ProcessNameAlt;
        config["WindowTitleAlt"]    = Config::Launcher.WindowTitleAlt;
        config["IconFile"]          = Config::Launcher.IconFile;

        std::ofstream file(GetConfigFileA());
        file << config.dump(4);
        file.close();
    }
}
