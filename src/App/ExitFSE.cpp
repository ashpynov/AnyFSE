#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Process.hpp"
#include "Tools/Unicode.hpp"

#include "App/Launchers.hpp"
#include "App/GamingExperience.hpp"
#include "App/ExitFSE.hpp"


namespace AnyFSE::App::ExitFSE
{

    HANDLE RegisterWaitingMutex();
    bool IsMutexExists();

    static Logger log = LogManager::GetLogger("ExitFSE");


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
        if (!Config::ExitFSEOnHomeExit || !GamingExperience::IsFullscreenMode() || IsMutexExists() || !Launchers::IsLauncherActiveOrMinimized() )
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
            DWORD start = GetTickCount();
            GamingExperience::ExitFSEMode();

            if (GetTickCount() - start < 50)
            {
                log.Trace("GamingExperience::ExitFSEMode() Completed in: %d msec", GetTickCount() - start);

                bool wasGamebar = false;
                bool isGamebar = false;
                log.Debug("Waiting ExitFSE loop");
                while(GamingExperience::IsFullscreenMode() && (isGamebar || !wasGamebar))
                {
                    Sleep(500);
                    std::wstring activeProcess = Unicode::to_lower(Process::GetWindowProcessName(WindowFromPoint(POINT{1, 1})));
                    log.Trace("Waiting ExitFSE: Active process: %s", Unicode::to_string(activeProcess).c_str());
                    isGamebar = activeProcess == L"gamebar.exe";
                    wasGamebar |= isGamebar;
                }
            }
            if (!GamingExperience::IsFullscreenMode())
            {
                Sleep(2000);
            }
            log.Trace("Waiting ExitFSE: Complete, mode is %s", GamingExperience::IsFullscreenMode() ? "FSE" : "Desktop");
        }
        CloseHandle(hMutex);
        return true;
    }

    bool WaitExitFSEMode()
    {
        if (!Config::ExitFSEOnHomeExit || !IsMutexExists() || Launchers::IsLauncherActiveOrMinimized())
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