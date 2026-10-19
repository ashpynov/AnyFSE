#pragma once

#include <windows.h>

namespace ACSEFilter
{
    BOOL WINAPI HookDeviceIoControl(
        HANDLE device,
        DWORD controlCode,
        LPVOID inputBuffer,
        DWORD inputSize,
        LPVOID outputBuffer,
        DWORD outputSize,
        LPDWORD bytesReturned,
        LPOVERLAPPED overlapped);
} // namespace ACSEFilter
