// Implementation of power efficiency helpers
#include "Tools/PowerEfficiency.hpp"
#include "Logging/LogManager.hpp"

namespace AnyFSE::Tools
{

    static Logger log = LogManager::GetLogger("PowerEfficiency");

    void EnablePowerEfficencyMode(bool bEnable)
    {
        log.Info("Power Efficiency mode -> %s", bEnable ? "enable" : "disable");

        // Change process priority
        if (bEnable)
        {
            if (!SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS))
            {
                log.Warn("SetPriorityClass(IDLE) failed: %d", GetLastError());
            }
        }
        else
        {
            if (!SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS))
            {
                log.Warn("SetPriorityClass(NORMAL) failed: %d", GetLastError());
            }
        }

        // Try to (en|dis)able process-level power throttling (execution speed)
        PROCESS_POWER_THROTTLING_STATE pState = {};
        pState.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
        pState.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
        pState.StateMask = bEnable ? PROCESS_POWER_THROTTLING_EXECUTION_SPEED : 0;
        if (!SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &pState, sizeof(pState)))
        {
            log.Warn("SetProcessInformation(ProcessPowerThrottling) %s failed: %d", bEnable ? "enable" : "disable", GetLastError());
        }

        // Try to (en|dis)able thread-level throttling for the current thread
        THREAD_POWER_THROTTLING_STATE tState = {};
        tState.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
        tState.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        tState.StateMask = bEnable ? THREAD_POWER_THROTTLING_EXECUTION_SPEED : 0;
        if (!SetThreadInformation(GetCurrentThread(), ThreadPowerThrottling, &tState, sizeof(tState)))
        {
            log.Warn("SetThreadInformation(ThreadPowerThrottling) %s failed: %d", bEnable ? "enable" : "disable", GetLastError());
        }
    }
}
