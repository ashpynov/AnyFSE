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
    using jp = json::json_pointer;

    LogLevels       Config::LogLevel = LogLevels::Disabled;
    std::wstring    Config::LogPath = L"";
    bool            Config::CustomSettings;
    LauncherConfig  Config::Launcher;
    std::wstring    Config::XBoxProcessName;
    bool            Config::AggressiveMode = false;
    bool            Config::SilentMode = false;
    bool            Config::FseOnStartup = false;
    bool            Config::SplashShowAnimation = true;
    bool            Config::SpalshShowLogo = true;
    bool            Config::SplashShowText = true;
    std::wstring    Config::SplashCustomText = L"";
    bool            Config::SplashShowVideo = true;
    std::wstring    Config::SplashVideoPath = L"";
    bool            Config::SplashVideoMute = false;
    bool            Config::SplashVideoLoop = false;
    bool            Config::QuickStart = false;
    bool            Config::CleanupFailedStart = true;
    DWORD           Config::RestartDelay = 1000;

    std::wstring Config::GetModulePath()
    {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        fs::path binary = path;
        return binary.parent_path().wstring();
    }

    bool Config::IsConfigured()
    {
        return fs::exists(GetConfigFileA());
    }

    std::string Config::GetConfigFileA()
    {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        return fs::path(path).parent_path().string() + "\\AnyFSE.json";
    }

    json Config::GetConfig()
    {
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
        return config;
    }

    void Config::Load()
    {
        LogPath = GetModulePath();

        json config = GetConfig();

        SilentMode = false;
        FseOnStartup = true;

        LogLevel                = (LogLevels)config.value(jp("/Log/Level"),  (int)LogLevels::Disabled);
        LogPath                 = config.value(jp("/Log/Path"),              GetModulePath());
        AggressiveMode          = config.value(jp("/AggressiveMode"),        false);
        QuickStart              = config.value(jp("/QuickStart"),            false);
        CleanupFailedStart      = config.value(jp("/CleanupFailedStart"),    true);
        RestartDelay            = config.value(jp("/RestartDelay"),          (DWORD)1000);
        XBoxProcessName         = config.value(jp("/XBoxProcessName"),       std::wstring(L"XboxPcApp.exe"));

        SplashShowAnimation     = config.value(jp("/Splash/ShowAnimation"),  true);
        SpalshShowLogo          = config.value(jp("/Splash/ShowLogo"),       true);
        SplashShowText          = config.value(jp("/Splash/ShowText"),       true);
        SplashCustomText        = config.value(jp("/Splash/CustomText"),     std::wstring());

        SplashShowVideo         = config.value(jp("/Splash/ShowVideo"),      true);
        SplashVideoPath         = config.value(jp("/Splash/Video/Path"),     std::wstring());
        SplashVideoMute         = config.value(jp("/Splash/Video/Mute"),     false);
        SplashVideoLoop         = config.value(jp("/Splash/Video/Loop"),     false);

        std::wstring launcher   = config.value(jp("/Launcher/Path"),         std::wstring());

        LoadLauncherSettings(config, launcher, Launcher);
        CustomSettings = config.value(jp("/Launcher/CustomSettings"), Launcher.IsCustom);

    }

    bool Config::LoadLauncherSettings(const json& config, const std::wstring &path, LauncherConfig& out)
    {
        GetLauncherDefaults(path, out);
        if (out.Type == Custom || config.value(jp("/Launcher/CustomSettings"), out.IsCustom))
        {
            out.StartCommand    = config.value(jp("/Launcher/StartCommand"),   out.StartCommand);
            out.StartArg        = config.value(jp("/Launcher/StartArg"),       out.StartArg);
            out.ProcessName     = config.value(jp("/Launcher/ProcessName"),    out.ProcessName);
            out.WindowTitle     = config.value(jp("/Launcher/WindowTitle"),    out.WindowTitle);
            out.ProcessNameAlt  = config.value(jp("/Launcher/ProcessNameAlt"), out.ProcessNameAlt);
            out.WindowTitleAlt  = config.value(jp("/Launcher/WindowTitleAlt"), out.WindowTitleAlt);
            out.IconFile        = config.value(jp("/Launcher/IconFile"),       out.IconFile);
        }
        return true;
    }

    bool Config::LoadLauncherSettings(const std::wstring& path, LauncherConfig& out)
    {
        return LoadLauncherSettings(GetConfig(), path, out);
    }

    void Config::Save()
    {
        json config = GetConfig();

        config["Launcher"]["Path"]              = Launcher.StartCommand;
        config["Launcher"]["CustomSettings"]    = CustomSettings;
        config["Launcher"]["StartCommand"]      = Launcher.StartCommand;
        config["Launcher"]["StartArg"]          = Launcher.StartArg;
        config["Launcher"]["ProcessName"]       = Launcher.ProcessName;
        config["Launcher"]["WindowTitle"]       = Launcher.WindowTitle;
        config["Launcher"]["ProcessNameAlt"]    = Launcher.ProcessNameAlt;
        config["Launcher"]["WindowTitleAlt"]    = Launcher.WindowTitleAlt;
        config["Launcher"]["IconFile"]          = Launcher.IconFile;

        config["Splash"]["ShowAnimation"]       = SplashShowAnimation;
        config["Splash"]["ShowLogo"]            = SpalshShowLogo;
        config["Splash"]["ShowText"]            = SplashShowText;
        config["Splash"]["ShowVideo"]           = SplashShowVideo;
        config["Splash"]["CustomText"]          = SplashCustomText;

        config["Splash"]["Video"]["Path"]       = SplashVideoPath;
        config["Splash"]["Video"]["Mute"]       = SplashVideoMute;
        config["Splash"]["Video"]["Loop"]       = SplashVideoLoop;

        config["Log"]["Level"]                  = (int)LogLevel;

        config["QuickStart"]                    = QuickStart;
        config["CleanupFailedStart"]            = CleanupFailedStart;
        config["RestartDelay"]                  = RestartDelay;

    //  config["Log"]["Path"]                   = LogPath;
    //  config["AggressiveMode"]                = AggressiveMode;
    //  config["XBoxProcessName"]               = XBoxProcessName;

        std::ofstream file(GetConfigFileA());
        file << config.dump(4);
        file.close();
    }
}
