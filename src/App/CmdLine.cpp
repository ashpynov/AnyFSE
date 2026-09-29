#include <filesystem>

#include "Tools/Registry.hpp"
#include "Tools/Elevated.hpp"
#include "Tools/Process.hpp"
#include "Tools/Paths.hpp"
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "App/Launchers.hpp"
#include "App/GamingExperience.hpp"
#include "App/Constants.hpp"
#include "App/App.hpp"
#include "Ally/Handlers.hpp"
#include "Ally/Ally.hpp"

namespace AnyFSE::App::CmdLine
{
    static Logger log = LogManager::GetLogger("CmdLine");

    bool AsHidListener(LPSTR lpCmdLine)
    {
        for (char *a = lpCmdLine; *a; a++)
        {
            if (_strnicmp(a, "/HidListener", 12) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool AsHidListenerJob(LPSTR lpCmdLine)
    {
        for (char *a = lpCmdLine; *a; a++)
        {
            if (_strnicmp(a, "/HidListenerJob", 15) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool AsElevated(LPSTR lpCmdLine)
    {
        return lpCmdLine && _stricmp(lpCmdLine, Constants::AnyFseTaskArgumentA) == 0;
    }

    bool AsFSE(LPSTR lpCmdLine)
    {
        for (char *a = lpCmdLine; *a; a++)
        {
            if (_strnicmp(a, "/FSE", 4) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool AsFSENow(LPSTR lpCmdLine)
    {
        for (char *a = lpCmdLine; *a; a++)
        {
            if (_strnicmp(a, "/FSENow", 7) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool AsFSEReboot(LPSTR lpCmdLine)
    {
        for (char *a = lpCmdLine; *a; a++)
        {
            if (_strnicmp(a, "/FSEReboot", 10) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool AsSettings(LPSTR lpCmdLine)
    {
        // no gamingapp:/// protocol specified
        if (strlen(lpCmdLine) == 0)
        {
            return true;
        }
        // Settings or Config is not exists
        //
        if (!Config::IsConfigured())
        {
            return true;
        }

        for (char *a = lpCmdLine; *a; a++)
        {
            if (_strnicmp(a, "/Settings", 9) == 0)
            {
                return true;
            }
        }

        // or Launcher == None or Launcher == Xbox
        if (Config::Launcher.Type == LauncherType::None
         || Config::Launcher.Type == LauncherType::Native)
        {
            return true;
        }

        // Registry != AnyFSE
        const std::wstring AnyFSEApp = Constants::AppUserModelId;
        const std::wstring selectedApp = Registry::ReadString(
            Constants::GamingHomeAppRegKey,
            Constants::GamingHomeAppRegValue);

        if (_wcsicmp(selectedApp.c_str(), AnyFSEApp.c_str() ) != 0)
        {
            return true;
        }

        if (Config::Launcher.StartCommand.find(L"://") == std::wstring::npos)
        {
            namespace fs = std::filesystem;
            if (!fs::exists(Config::Launcher.StartCommand))
            {
                log.Warn("Launcher file is unavailable; opening settings");
                return true;
            }
        }

        return false;
    }

    bool Elevated(LPSTR lpCmdLine, int &result)
    {
        if (!AsElevated(lpCmdLine))
        {
            return false;
        }

        Elevated::Register(Constants::ElevatedStartLauncher,  []() { Launchers::StartLauncher(false); });
        Elevated::Register(Constants::ElevatedStartupApps, []() { Launchers::LaunchStartupApps(true); });
        Elevated::Register(Constants::ElevatedEnableGamingHandheld, GamingExperience::EnableGamingHandheld);
        Elevated::Register(Constants::ElevatedRestoreGamingPC, GamingExperience::RestoreGamingPC);
        Elevated::Register(Constants::ElevatedHidListener, []() { Process::StartProcess(Paths::GetExeFileName(), L"/HidListenerJob"); });

        result = Elevated::CallHandler() ? 0 : 1;
        return true;
    }

    bool HidListenerJob(LPSTR lpCmdLine, int& result)
    {
        if (!AsHidListenerJob(lpCmdLine))
        {
            return false;
        }

        if (Config::HotkeysEnable || (Config::AllyHidEnable && Ally::IsSupported()))
        {
            log.Debug("Starting background HID/hotkey listener\n");
            AnyFSE::Logging::LogManager::Initialize("AnyFSE/BackgroundListener", Config::LogLevel, Config::LogPath);
            result = Ally::HIDListener(NULL);
        }

        return true;
    }

    bool HidListener(LPSTR lpCmdLine, int& result)
    {
        if (!AsHidListener(lpCmdLine))
        {
            return false;
        }

        if (Config::HotkeysEnable || (Config::AllyHidEnable && Ally::IsSupported()))
        {
            Elevated::Call(Constants::ElevatedHidListener);
        }
        return true;
    }

    bool FSE(LPSTR lpCmdLine, int& result)
    {
        if (!AsFSE(lpCmdLine))
        {
            return false;
        }

        result = 0;

        if (GamingExperience::IsFullscreenMode())
        {
            return true;
        }

        GamingExperience::ConfirmationMode mode = AsFSEReboot(lpCmdLine) ? GamingExperience::Reboot
                                                : AsFSENow(lpCmdLine)    ? GamingExperience::Now
                                                                         : GamingExperience::Ask;

        result = GamingExperience::EnterFSEMode( mode );
        return true;
    }

    bool Settings(LPSTR lpCmdLine, int &result)
    {
        if (!AsSettings(lpCmdLine))
        {
            return false;
        }

        App::ShowSettings();
        result = 0;
        return true;
    }
}