#pragma once
#include <windows.h>

namespace AnyFSE::Tools::Elevated
{
    // Compatible with existing clients: exclusive ownership is the lifetime of
    // a newly created event, never the state of an already existing event.
    inline HANDLE ReserveCall(const wchar_t* name, DWORD timeoutMs)
    {
        const ULONGLONG deadline = GetTickCount64() + timeoutMs;
        for (;;)
        {
            HANDLE event = CreateEventW(nullptr, TRUE, FALSE, name);
            const DWORD error = GetLastError();
            if (!event) return nullptr;
            if (error != ERROR_ALREADY_EXISTS) return event;
            CloseHandle(event);
            if (GetTickCount64() >= deadline)
            {
                SetLastError(ERROR_TIMEOUT);
                return nullptr;
            }
            Sleep(50);
        }
    }
}
