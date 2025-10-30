#include <string>
#include <filesystem>
#include <processenv.h>
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Launchers.hpp"


namespace AnyFSE::App::AppControl::Launchers
{
    static Logger log = LogManager::GetLogger("Launchers");

    void LauncherOnBoot()
    {
        switch (Config::Launcher.Type)
        {
            case LauncherType::PlayniteDesktop:
            case LauncherType::PlayniteFullscreen:
                return PlayniteOnBoot();
        };
    }

    void PlayniteOnBoot()
    {
        if (Config::CleanupFailedStart)
        {
            log.Debug("Cleanup Playnite safe startup flag");

            namespace fs = std::filesystem;
            fs::path path = fs::path(Config::Launcher.StartCommand);

            fs::path configPath = path.parent_path();
            bool isPortable = !fs::exists(configPath.append(L"unins000.exe"));

            if (!isPortable)
            {
                log.Debug("Playnite is not portable, config is in %%APPDATA%%");

                wchar_t buffer[MAX_PATH] = {0};
                if (ExpandEnvironmentStringsW(L"%APPDATA%\\Playnite", buffer, MAX_PATH))
                {
                    configPath = fs::path(buffer);
                }
            }

            fs::path flagFile = configPath.append(L"safestart.flag");

            if(fs::exists(flagFile))
            {
                log.Debug("Safestart flag is exist at %s, deleting", flagFile.string().c_str());
                fs::remove(flagFile);
            }
        }
    }
}