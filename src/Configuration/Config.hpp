#include <string>
#include <windows.h>

using namespace std;

namespace AnyFSE::Configuration
{
    class Config
    {
            enum LauncherType {
                PlayniteFullscreen = 0,
                PlayniteDesktop,
                ArmoryCrate,
                Steam
            };
            static const wstring root;

            Config() {};

            static wstring GetFilename();
            static void SetDefault();
        
            static wstring GetLauncher();
        public:

            static void Load();
            static void Dump();
            
            static wstring LauncherName;
            static wstring LauncherWindowName;
            static wstring LauncherIcon;
            static wstring LauncherProcessName;
            static wstring LauncherStartCommand;
            static wstring LauncherStartCommandArgs;

            static wstring XBoxProcessName;

            static bool AggressiveMode;
            static bool SilentMode;
    };
}

using namespace AnyFSE::Configuration;