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
        Native,
        PlayniteFullscreen,
        PlayniteDesktop,
        ArmouryCrate,
        Steam,
        BigBox,
        OneGameLauncher,
        RetroBat
    };

    struct LauncherConfig
    {
        LauncherType Type = LauncherType::None;
        std::wstring Name;
        std::wstring URL;
        std::wstring StartCommand;
        std::wstring StartArg;

        DWORD ExStyle = 0;
        DWORD NoStyle = 0;
        std::wstring ProcessName;
        std::wstring ClassName;
        std::wstring WindowTitle;

        DWORD ExStyleAlt = 0;
        DWORD NoStyleAlt = 0;
        std::wstring ProcessNameAlt;
        std::wstring ClassNameAlt;
        std::wstring WindowTitleAlt;
        std::wstring IconFile;
        bool IsTrayAggressive = false;
        std::wstring ActivationProtocol;
        bool IsCustom = false;
        bool IsPortable = false;
        std::wstring AppUserModelID;
    };

    struct StartupApp
    {
        std::wstring Path;
        std::wstring Args;
        bool Enabled = false;
    };

    class Config
    {
            //static const wstring Root;
            static std::list<LauncherConfig> LauncherConfigs;

            Config() {};

            // Unsafe
        public:
            static void GetStartupConfigured();
            static bool IsFseOnStartupConfigured();
            static bool IsNativeConfigured();
            static bool IsAnyFSEConfigured();
            static bool FindInstalledLaunchers(std::list<std::wstring> &found);
            static bool FindNotInstalledLaunchers(std::list<std::wstring> &found);
            static std::wstring GetNativePath(const std::wstring &launcher);
            static void FindPlaynite(std::list<std::wstring>& found);
            static void FindSteam(std::list<std::wstring>& found);
            static void FindBigBox(std::list<std::wstring>& found);
            static void FindOneGameLauncher(std::list<std::wstring>& found);
            static void FindNativeLaunchers(std::list<std::wstring> &found);
            static void FindArmouryCrate(std::list<std::wstring> &found);
            static void FindRetroBat(std::list<std::wstring>& found);

            static std::wstring GetPathFromCommand(const std::wstring &uninstallCommand);
            static std::wstring SearchAppUserModel(const std::wstring &displayName);
            static std::wstring GetAssociationPath(const std::wstring &extName);
            static std::wstring GetInstallPath(const std::wstring &displayName);

        public:
            // Unsafe
            static void UpdatePortableLauncher(LauncherConfig &out);
            static bool FindLaunchers(std::list<std::wstring> &found);

            static std::string GetConfigFileA(bool readOnly = true);
            static void Load();
            static bool LoadLauncherSettings(const nlohmann::json &config, const std::wstring &path, LauncherConfig &out);
            static bool LoadLauncherSettings(const std::wstring &path, LauncherConfig &out);
            static json GetConfig();
            static void Save();

            static void SaveWindowPlacement(int cmdShow, const RECT & rcNormalPosition);
            static int LoadWindowPlacement(RECT * prcNormalPosition);

            static void SaveUpdateVersion(const std::wstring &lastVersion);

            static std::wstring GetApplicationName(const std::wstring &filePath);

            static std::wstring GetFileDescription(const std::wstring &filePath);

            static bool IsConfigured();


            static bool GetLauncherDefaults(const std::wstring& path, LauncherConfig& out);

            static LauncherConfig Launcher;

            static bool    CustomSettings;

            static LogLevels LogLevel;
            static std::wstring LogPath;

            static bool AggressiveMode;
            static bool FseOnStartup;
            static bool QuickStart;

            static bool SplashShowAnimation;
            static bool SplashShowLogo;
            static bool SplashShowText;
            static std::wstring SplashCustomText;

            static bool SplashShowVideo;
            static bool SplashTillEnd;
            static std::wstring SplashVideoPath;
            static bool SplashVideoMute;
            static bool SplashVideoLoop;
            static bool SplashVideoPause;

            static bool CleanupFailedStart;
            static DWORD RestartDelay;

            static std::list<StartupApp> StartupApps;

            static bool ExitFSEOnHomeExit;

            static bool         UpdatePreRelease;
            static bool         UpdateNotifications;
            static int          UpdateCheckInterval;
            static std::wstring UpdateLastCheck;
            static std::wstring UpdateLastVersion;

            static bool         AllyHidEnable;
            static std::wstring AllyHidACPress;
            static std::wstring AllyHidACHold;
            static std::wstring AllyHidCCPress;
            static std::wstring AllyHidLibraryPress;
            static std::wstring AllyHidModeACPress;
            static std::wstring AllyHidModeACHold;
            static std::wstring AllyHidModeCCPress;
            static std::wstring AllyHidModeLibraryPress;
    };
}

using namespace AnyFSE::Configuration;