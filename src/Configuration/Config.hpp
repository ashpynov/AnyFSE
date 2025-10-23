#pragma once
#include <string>
#include <windows.h>
#include <list>
#include <map>
#include "Logging/Logger.hpp"

using namespace std;

namespace AnyFSE::Configuration
{
    enum LauncherType
    {
        None = 0,
        Custom,
        PlayniteFullscreen,
        PlayniteDesktop,
        ArmoryCrate,
        Steam,
        Xbox
    };

    struct LauncherConfig
    {
        LauncherType Type;
        wstring Name;
        wstring StartCommand;
        wstring StartArg;
        wstring ProcessName;
        wstring WindowTitle;
        wstring ProcessNameAlt;
        wstring WindowTitleAlt;
        wstring IconFile;
        bool IsTrayAggressive;
        bool IsCustom;
        bool IsPortable;
    };

    class Config
    {
            static const wstring Root;
            static const map<LauncherType, LauncherConfig> LauncherConfigsMap;

            Config() {};

            // Unsafe
            static void GetStartupConfigured();
            static bool IsFseOnStartupConfigured();
            static bool IsXboxConfigured();
            static bool FindInstalledLaunchers(list<wstring>& found);
            static wstring GetXboxPath(const std::wstring &launcher);
            static void FindLocal(list<wstring>& found);
            static void FindPlaynite(list<wstring>& found);
            static void FindSteam(list<wstring>& found);
            static void FindXbox(list<wstring>& found);
            static void FindArmoryCrate(list<wstring>& found);

        public:
            // Unsafe
            static void UpdatePortableLauncher(LauncherConfig &out);
            static bool FindLaunchers(std::list<wstring> &found);

            // safe
            static wstring GetModulePath();
            static void Load();
            static void Save();

            static bool IsConfigured();

            static bool GetLauncherDefaults(const wstring& path, LauncherConfig& out);
            static bool LoadLauncherSettings(const wstring& name, LauncherConfig& out);

            static LauncherConfig Launcher;

            static bool    CustomSettings;

            static LogLevels LogLevel;
            static wstring LogPath;
            static wstring XBoxProcessName;

            static bool AggressiveMode;
            static bool FseOnStartup;
            static bool SilentMode;

            static bool ShowAnimation;
            static bool ShowLogo;
            static bool ShowText;

    };
}

using namespace AnyFSE::Configuration;