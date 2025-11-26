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
        BigBox,
        OneGameLauncher,
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

    struct StartupApp
    {
        std::wstring Path;
        std::wstring Args;
        bool Enabled;
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
            static void FindBigBox(std::list<std::wstring>& found);
            static void FindOneGameLauncher(std::list<std::wstring>& found);
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

            static std::wstring GetApplicationName(const std::wstring & filePath);

            static std::wstring GetFileDescription(const std::wstring &filePath);

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

            static std::list<StartupApp> StartupApps;
    };
}

using namespace AnyFSE::Configuration;