// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>

#pragma once

#include <string>
#include <windows.h>
#include <list>
#include <map>
#include "Logging/Logger.hpp"
#include "nlohmann/json_fwd.hpp"

using json = nlohmann::json;

namespace AnyFSE::Configuration
{
    enum LauncherType
    {
        None = 0,
        Custom,
        PlayniteFullscreen,
        PlayniteDesktop,
        ArmouryCrate,
        Steam,
        Xbox
    };

    struct LauncherConfig
    {
        LauncherType Type;
        std::wstring Name;
        std::wstring StartCommand;
        std::wstring StartArg;

        DWORD ExStyle;
        DWORD NoStyle;
        std::wstring ProcessName;
        std::wstring ClassName;
        std::wstring WindowTitle;

        DWORD ExStyleAlt;
        DWORD NoStyleAlt;
        std::wstring ProcessNameAlt;
        std::wstring ClassNameAlt;
        std::wstring WindowTitleAlt;
        std::wstring IconFile;
        bool IsTrayAggressive;
        bool IsCustom;
        bool IsPortable;
    };

    class Config
    {
            //static const wstring Root;
            static const std::map<LauncherType, LauncherConfig> LauncherConfigsMap;

            Config() {};

            // Unsafe
        public:
            static void GetStartupConfigured();
            static bool IsFseOnStartupConfigured();
            static bool IsXboxConfigured();
            static bool FindInstalledLaunchers(std::list<std::wstring>& found);
            static std::wstring GetXboxPath(const std::wstring &launcher);
            static void FindLocal(std::list<std::wstring>& found);
            static void FindPlaynite(std::list<std::wstring>& found);
            static void FindSteam(std::list<std::wstring>& found);
            static void FindXbox(std::list<std::wstring>& found);
            static void FindArmoryCrate(std::list<std::wstring>& found);

        public:
            // Unsafe
            static void UpdatePortableLauncher(LauncherConfig &out);
            static bool FindLaunchers(std::list<std::wstring> &found);

            // safe
            static std::wstring GetModulePath();

            static std::string GetConfigFileA();
            static void Load();
            static bool LoadLauncherSettings(const nlohmann::json &config, const std::wstring &path, LauncherConfig &out);
            static bool LoadLauncherSettings(const std::wstring &path, LauncherConfig &out);
            static json GetConfig();
            static void Save();

            static bool IsConfigured();

            static bool GetLauncherDefaults(const std::wstring& path, LauncherConfig& out);

            static LauncherConfig Launcher;

            static bool    CustomSettings;

            static LogLevels LogLevel;
            static std::wstring LogPath;
            static std::wstring XBoxProcessName;

            static bool AggressiveMode;
            static bool FseOnStartup;
            static bool QuickStart;

            static bool SplashShowAnimation;
            static bool SplashShowLogo;
            static bool SplashShowText;
            static std::wstring SplashCustomText;

            static bool SplashShowVideo;
            static std::wstring SplashVideoPath;
            static bool SplashVideoMute;
            static bool SplashVideoLoop;
            static bool SplashVideoPause;

            static bool CleanupFailedStart;
            static DWORD RestartDelay;
    };
}

using namespace AnyFSE::Configuration;