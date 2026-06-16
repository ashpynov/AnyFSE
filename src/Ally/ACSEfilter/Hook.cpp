#include <windows.h>

#include "DebugLog.h"
#include "HidReadFilter.h"
#include "IATHook.h"
#include "Native.h"

namespace ACSEFilter::Hook
{
    namespace
    {
        BOOL WINAPI HookReadFile(HANDLE file, LPVOID buffer, DWORD bytesToRead, LPDWORD bytesRead, LPOVERLAPPED overlapped)
        {
            BOOL result = Native::ReadFile(file, buffer, bytesToRead, bytesRead, overlapped);
            const DWORD lastError = GetLastError();

            if (result)
            {
                const DWORD actualBytes = bytesRead ? *bytesRead : bytesToRead;
                PatchCompletedRead(file, buffer, actualBytes, bytesToRead);
            }
            else if (lastError == ERROR_IO_PENDING && overlapped)
            {
                RememberPendingRead(file, buffer, bytesToRead, overlapped);
            }

            SetLastError(lastError);
            return result;
        }

        bool FindPendingWaitHandleIndex(
            DWORD count,
            const HANDLE *handles,
            DWORD& pendingWaitHandleIndex)
        {
            if (!handles)
            {
                return false;
            }

            const HANDLE pendingWaitHandle = PendingReadWaitHandle();
            if (!pendingWaitHandle)
            {
                return false;
            }

            for (DWORD i = 0; i < count; ++i)
            {
                if (handles[i] == pendingWaitHandle)
                {
                    pendingWaitHandleIndex = i;
                    return true;
                }
            }

            return false;
        }

        DWORD WINAPI HookWaitForMultipleObjects(
            DWORD count,
            const HANDLE *handles,
            BOOL waitAll,
            DWORD milliseconds)
        {
            const DWORD result = Native::WaitForMultipleObjects(count, handles, waitAll, milliseconds);
            const DWORD lastError = GetLastError();

            DWORD pendingWaitHandleIndex = 0;
            if (!waitAll
                && FindPendingWaitHandleIndex(count, handles, pendingWaitHandleIndex)
                && result == WAIT_OBJECT_0 + pendingWaitHandleIndex)
            {
                DEBUG(L"Read data via WaitForMultipleObjects");
                CompletePendingReadAfterWait();
            }

            SetLastError(lastError);
            return result;
        }

        const ImportHookSpec *HookSpecs(size_t &count)
        {
            static const ImportHookSpec specs[] =
                {
                    {"ReadFile", reinterpret_cast<void *>(HookReadFile)},
                    {"WaitForMultipleObjects", reinterpret_cast<void *>(HookWaitForMultipleObjects)},
                };

            count = ARRAYSIZE(specs);
            return specs;
        }

    } // namespace

    DWORD WINAPI Install(LPVOID context)
    {
        const HMODULE selfModule = static_cast<HMODULE>(context);
        if (!Native::ResolveKernelFunctions())
        {
            DEBUG(L"Failed to resolve required kernel functions.");
            return 1;
        }

        size_t count = 0;
        const ImportHookSpec *specs = HookSpecs(count);
        PatchAllModuleImports(selfModule, specs, count);

        DEBUG(L"Hooks installed.");
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
