// Implementation of power efficiency helpers
#include "PowerEfficiency.hpp"

namespace AnyFSE::Tools
{

    void EnablePowerEfficencyMode(bool bEnable)
    {
        // Change process priority
        SetPriorityClass(GetCurrentProcess(), bEnable ? IDLE_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);

        // Try to (en|dis)able process-level power throttling (execution speed)
        PROCESS_POWER_THROTTLING_STATE pState = {};
        pState.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
        pState.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
        pState.StateMask = bEnable ? PROCESS_POWER_THROTTLING_EXECUTION_SPEED : 0;
        SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &pState, sizeof(pState));

        // Try to (en|dis)able thread-level throttling for the current thread
        THREAD_POWER_THROTTLING_STATE tState = {};
        tState.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
        tState.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        tState.StateMask = bEnable ? THREAD_POWER_THROTTLING_EXECUTION_SPEED : 0;
        SetThreadInformation(GetCurrentThread(), ThreadPowerThrottling, &tState, sizeof(tState));
    }
}
