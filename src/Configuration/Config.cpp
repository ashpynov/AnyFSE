// MIT License
//
// Copyright (c) 2025 Artem Shpynov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <filesystem>
#include <fstream>
#include <sstream>
#include "Config.hpp"
#include "Tools/Unicode.hpp"
#include "nlohmann/json.hpp"

#pragma comment(lib, "version.lib")

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

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(StartupApp, Path, Args, Enabled)

    LogLevels       Config::LogLevel = LogLevels::Disabled;
    std::wstring    Config::LogPath = L"";
    bool            Config::CustomSettings;
    LauncherConfig  Config::Launcher;
    std::wstring    Config::XBoxProcessName;
    bool            Config::AggressiveMode = false;
    bool            Config::FseOnStartup = false;
    bool            Config::SplashShowAnimation = true;
    bool            Config::SplashShowLogo = true;
    bool            Config::SplashShowText = true;
    std::wstring    Config::SplashCustomText = L"";
    bool            Config::SplashShowVideo = true;
    std::wstring    Config::SplashVideoPath = L"";
    bool            Config::SplashVideoMute = false;
    bool            Config::SplashVideoLoop = false;
    bool            Config::SplashVideoPause = true;
    bool            Config::QuickStart = false;
    bool            Config::CleanupFailedStart = true;
    DWORD           Config::RestartDelay = 1000;
    std::list<StartupApp> Config::StartupApps;

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

        FseOnStartup = true;

        LogLevel                = (LogLevels)config.value(jp("/Log/Level"),  (int)LogLevels::Disabled);
        LogPath                 = config.value(jp("/Log/Path"),              GetModulePath());
        AggressiveMode          = config.value(jp("/AggressiveMode"),        false);
        QuickStart              = config.value(jp("/QuickStart"),            false);
        CleanupFailedStart      = config.value(jp("/CleanupFailedStart"),    true);
        RestartDelay            = config.value(jp("/RestartDelay"),          (DWORD)1000);
        XBoxProcessName         = config.value(jp("/XBoxProcessName"),       std::wstring(L"XboxPcApp.exe"));

        SplashShowAnimation     = config.value(jp("/Splash/ShowAnimation"),  true);
        SplashShowLogo          = config.value(jp("/Splash/ShowLogo"),       true);
        SplashShowText          = config.value(jp("/Splash/ShowText"),       true);
        SplashCustomText        = config.value(jp("/Splash/CustomText"),     std::wstring());

        SplashShowVideo         = config.value(jp("/Splash/ShowVideo"),      true);
        SplashVideoPath         = config.value(jp("/Splash/Video/Path"),     std::wstring());
        SplashVideoMute         = config.value(jp("/Splash/Video/Mute"),     false);
        SplashVideoLoop         = config.value(jp("/Splash/Video/Loop"),     false);
        SplashVideoPause        = config.value(jp("/Splash/Video/Pause"),    true);
        StartupApps             = config.value(jp("/StartupApps"),           std::list<StartupApp>());

        std::wstring launcher   = config.value(jp("/Launcher/Path"),         std::wstring());



        LoadLauncherSettings(config, launcher, Launcher);
        CustomSettings = config.value(jp("/Launcher/CustomSettings"), Launcher.IsCustom);

    }

    bool Config::LoadLauncherSettings(const json& config, const std::wstring &path, LauncherConfig& out)
    {
        GetLauncherDefaults(path, out);

        if ((out.Type == Custom || config.value(jp("/Launcher/CustomSettings"), out.IsCustom))
            && path == config.value(jp("/Launcher/StartCommand"), out.StartCommand))
        {
            out.StartCommand    = config.value(jp("/Launcher/StartCommand"),   out.StartCommand);
            out.StartArg        = config.value(jp("/Launcher/StartArg"),       out.StartArg);
            out.ExStyle         = config.value(jp("/Launcher/ExStyle"),        out.ExStyle);
            out.NoStyle         = config.value(jp("/Launcher/NoStyle"),        out.NoStyle);
            out.ProcessName     = config.value(jp("/Launcher/ProcessName"),    out.ProcessName);
            out.ClassName       = config.value(jp("/Launcher/ClassName"),      out.ClassName);
            out.WindowTitle     = config.value(jp("/Launcher/WindowTitle"),    out.WindowTitle);
            out.ExStyleAlt      = config.value(jp("/Launcher/ExStyleAlt"),     out.ExStyleAlt);
            out.NoStyleAlt      = config.value(jp("/Launcher/NoStyleAlt"),     out.NoStyleAlt);
            out.ProcessNameAlt  = config.value(jp("/Launcher/ProcessNameAlt"), out.ProcessNameAlt);
            out.ClassNameAlt    = config.value(jp("/Launcher/ClassNameAlt"),   out.ClassNameAlt);
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
        config["Launcher"]["ExStyle"]           = Launcher.ExStyle;
        config["Launcher"]["NoStyle"]           = Launcher.NoStyle;
        config["Launcher"]["ProcessName"]       = Launcher.ProcessName;
        config["Launcher"]["ClassName"]         = Launcher.ClassName;
        config["Launcher"]["WindowTitle"]       = Launcher.WindowTitle;
        config["Launcher"]["ExStyleAlt"]        = Launcher.ExStyleAlt;
        config["Launcher"]["NoStyleAlt"]        = Launcher.NoStyleAlt;
        config["Launcher"]["ProcessNameAlt"]    = Launcher.ProcessNameAlt;
        config["Launcher"]["ClassNameAlt"]      = Launcher.ClassNameAlt;
        config["Launcher"]["WindowTitleAlt"]    = Launcher.WindowTitleAlt;
        config["Launcher"]["IconFile"]          = Launcher.IconFile;

        config["Splash"]["ShowAnimation"]       = SplashShowAnimation;
        config["Splash"]["ShowLogo"]            = SplashShowLogo;
        config["Splash"]["ShowText"]            = SplashShowText;
        config["Splash"]["ShowVideo"]           = SplashShowVideo;
        config["Splash"]["CustomText"]          = SplashCustomText;

        config["Splash"]["Video"]["Path"]       = SplashVideoPath;
        config["Splash"]["Video"]["Mute"]       = SplashVideoMute;
        config["Splash"]["Video"]["Loop"]       = SplashVideoLoop;
        config["Splash"]["Video"]["Pause"]      = SplashVideoPause;

        config["Log"]["Level"]                  = (int)LogLevel;

        config["QuickStart"]                    = QuickStart;
        config["CleanupFailedStart"]            = CleanupFailedStart;
        config["RestartDelay"]                  = RestartDelay;

        config["AggressiveMode"]                = AggressiveMode;
        config["StartupApps"]                   = StartupApps;

    //  config["Log"]["Path"]                   = LogPath;
    //  config["XBoxProcessName"]               = XBoxProcessName;

        std::ofstream file(GetConfigFileA());
        file << config.dump(4);
        file.close();
    }

    void Config::SaveWindowPlacement(int cmdShow, const RECT &rcNormalPosition)
    {
        json config = GetConfig();
        config["WindowPos"]["Left"] = rcNormalPosition.left;
        config["WindowPos"]["Top"] = rcNormalPosition.top;
        config["WindowPos"]["Right"] = rcNormalPosition.right;
        config["WindowPos"]["Bottom"] = rcNormalPosition.bottom;
        config["WindowPos"]["State"] = cmdShow;

        std::ofstream file(GetConfigFileA());
        file << config.dump(4);
        file.close();
    }

    int Config::LoadWindowPlacement(RECT *prcNormalPosition)
    {
        json config = GetConfig();

        prcNormalPosition->left = config.value(jp("/WindowPos/Left"), 0);
        prcNormalPosition->top = config.value(jp("/WindowPos/Top"), 0);
        prcNormalPosition->right = config.value(jp("/WindowPos/Right"), 0);
        prcNormalPosition->bottom = config.value(jp("/WindowPos/Bottom"), 0);

        return config.value(jp("/WindowPos/State"), 0);
    }

    std::wstring Config::GetApplicationName(const std::wstring &filePath)
    {
        std::wstring name = GetFileDescription(filePath);

        if (name.empty())
        {
            namespace fs = std::filesystem;
            fs::path p(filePath);
            name = p.filename().replace_extension(L"").wstring();
        }
        return name;
    }

    std::wstring Config::GetFileDescription(const std::wstring &filePath)
    {
        DWORD dummy;
        DWORD size = GetFileVersionInfoSize(filePath.c_str(), &dummy);
        if (size == 0)
        {
            return L"";
        }

        std::vector<BYTE> data(size);
        if (!GetFileVersionInfo(filePath.c_str(), 0, size, data.data()))
        {
            return L"";
        }

        // Get the file description
        struct LANGANDCODEPAGE
        {
            WORD language;
            WORD codePage;
        } *lpTranslate;

        UINT cbTranslate;
        if (!VerQueryValue(data.data(), TEXT("\\VarFileInfo\\Translation"),
                           (LPVOID *)&lpTranslate, &cbTranslate))
        {
            return L"";
        }

        // Try each language and code page
        for (UINT i = 0; i < (cbTranslate / sizeof(LANGANDCODEPAGE)); i++)
        {
            wchar_t subBlock[256];
            swprintf(subBlock, 256,
                     L"\\StringFileInfo\\%04x%04x\\FileDescription",
                     lpTranslate[i].language, lpTranslate[i].codePage);

            wchar_t *buffer = nullptr;
            UINT dwBytes;
            if (VerQueryValue(data.data(), subBlock, (LPVOID *)&buffer, &dwBytes))
            {
                return std::wstring(buffer);
            }
        }

        return L"";
    }
}
