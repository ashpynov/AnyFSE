#include <string>
#include <windows.h>

using namespace std;

namespace AnyFSE::Configuration
{
    class Config
    {
            Config() {};

            static wstring GetFilename();
            static void SetDefault();
        
        public:

            static void Load();
            static void Dump();
            
            static wstring LauncherName;
            static wstring LauncherWindowName;
            static wstring LauncherLogoPath;
            static wstring LauncherIcon;
            static wstring LauncherProcessName;
            static wstring LauncherStartCommand;
            static wstring LauncherStartCommandArgs;

            static wstring XBoxProcessName;

            static bool ShowLogo;
            static bool ShowText;
            static bool ShowAnimation;
            static COLORREF BackgroundColor;
            
            static bool SilentMode;
    };
}

using namespace AnyFSE::Configuration;