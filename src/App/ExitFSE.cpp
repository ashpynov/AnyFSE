#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Process.hpp"
#include "Tools/Unicode.hpp"

#include "App/Launchers.hpp"
#include "App/GamingExperience.hpp"
#include "App/ExitFSE.hpp"


namespace AnyFSE::App::ExitFSE
{

    bool IsEnabled();
    HANDLE RegisterWaitingMutex();
    bool IsMutexExists();

    static Logger log = LogManager::GetLogger("ExitFSE");

    bool IsEnabled()
    {
        return Config::ExitFSEOnHomeExit && GamingExperience::IsFullscreenMode();
    }

    HANDLE RegisterWaitingMutex()
    {
        HANDLE hMutex = CreateMutex(NULL, TRUE, L"ArtemShpynov.AnyFSE_WaitingExitFSE");

        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            log.Debug("Another instance of AnyFSE is already wait launcher exiting, exiting\n");
            if (hMutex)
            {
                CloseHandle(hMutex);
                return NULL;
            }
        }
        return hMutex;
    }

    bool IsMutexExists()
    {
        HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, L"ArtemShpynov.AnyFSE_WaitingExitFSE");

        if (hMutex)
        {
            CloseHandle(hMutex);
            return true;
        }

        return false;
    }

    bool WaitHomeAppExit()
    {
        if (!IsEnabled() || IsMutexExists() || !Launchers::IsLauncherActive() )
        {
            log.Trace("Skip WaitHomeAppExit");
            return false;
        }

        HANDLE hMutex = RegisterWaitingMutex();
        if (!hMutex)
        {
            log.Trace("Can't register RegisterWaitingMutex");
            return false;
        }

        log.Debug("Option to monitor home app finish");

        Launchers::WaitLauncherExit();
        if (Config::ExitFSEOnHomeExit)
        {
            GamingExperience::ExitFSEMode();
            bool wasGamebar = false;
            bool isGamebar = false;
            do
            {
                Sleep(500);
                std::wstring activeProcess = Unicode::to_lower(Process::GetWindowProcessName(WindowFromPoint(POINT{1, 1})));
                log.Trace("Waiting ExitFSE: Active process: %s", Unicode::to_string(activeProcess).c_str());
                isGamebar = activeProcess == L"gamebar.exe";
                wasGamebar |= isGamebar;
            } while (GamingExperience::IsFullscreenMode() && (isGamebar || !wasGamebar));

            log.Trace("Waiting ExitFSE: Complete, mode is %s", GamingExperience::IsFullscreenMode() ? "FSE" : "Desktop");
        }
        CloseHandle(hMutex);
        return true;
    }

    bool WaitExitFSEMode()
    {
        if (!IsEnabled() || !IsMutexExists() || Launchers::IsLauncherActive())
        {
            return false;
        }

        log.Trace("Check wait mutex");
        HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, L"ArtemShpynov.AnyFSE_WaitingExitFSE");
        if (hMutex)
        {
            log.Trace("Waiting ExitFSE");
            WaitForSingleObject(hMutex, INFINITE);
            CloseHandle(hMutex);
            hMutex = NULL;
            return !GamingExperience::IsFullscreenMode();
        }

        return false;
    }
}