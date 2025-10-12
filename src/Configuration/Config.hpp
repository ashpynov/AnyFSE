#pragma once
#include <string>
#include <windows.h>
#include <list>
#include <map>

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
        bool isCustom;
        bool isPortable;
    };

    class Config
    {
            static const wstring root;
            static const map<LauncherType, LauncherConfig> LauncherConfigs;

            Config() {};

            static wstring GetFilename();
            static wstring GetModulePath();
            static void SetDefault();

            static bool IsFseOnStartupConfigured();
            static bool IsXboxConfigured();

            static void FindLocal(list<wstring>& found);
            static void FindPlaynite(list<wstring>& found);
            static void FindSteam(list<wstring>& found);
            static void FindXbox(list<wstring>& found);
            static void FindArmoryCrate(list<wstring>& found);

        public:

            static void Load();
            static void Dump();

            static wstring GetCurrentLauncher();
            static bool FindLaunchers(std::list<wstring>& found);
            static bool FindInstalledLaunchers(list<wstring>& found);
            static bool GetLauncherDefaults(const wstring& path, LauncherConfig& out);
            static bool GetLauncherSettings(const wstring& name, LauncherConfig& out);

            static LauncherType Type;
            static wstring LauncherName;
            static wstring LauncherWindowName;
            static wstring LauncherIcon;
            static wstring LauncherProcessName;
            static wstring LauncherStartCommand;
            static wstring LauncherStartCommandArgs;

            static wstring XBoxProcessName;

            static bool AggressiveMode;
            static bool FseOnStartup;
            static bool SilentMode;
    };
}

using namespace AnyFSE::Configuration;