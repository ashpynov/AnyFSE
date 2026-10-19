#include <windows.h>

#include "../../App/Constants.hpp"
#include "DebugLog.h"
#include "DeviceIoControlFilter.h"
#include "IATHook.h"

namespace ACSEFilter::Hook
{
    DWORD WINAPI Install(LPVOID context)
    {
        const HMODULE selfModule = static_cast<HMODULE>(context);
        const HMODULE targetModule = GetModuleHandle(Constants::AsusOptimizationProcess);
        if (!targetModule || targetModule == selfModule)
        {
            LOG(L"Failed to resolve the target executable module.");
            return 1;
        }

        if (!PatchImport(targetModule, Constants::DeviceIoControlFunction, (void *)HookDeviceIoControl))
        {
            LOG(L"Failed to patch the target DeviceIoControl import.");
            return 1;
        }

        LOG(L"DeviceIoControl filter installed in the target executable module only.");
        return 0;
    }

}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);

        HANDLE thread = CreateThread(nullptr, 0, ACSEFilter::Hook::Install, module, 0, nullptr);
        if (thread)
        {
            CloseHandle(thread);
        }
    }

    return TRUE;
}
