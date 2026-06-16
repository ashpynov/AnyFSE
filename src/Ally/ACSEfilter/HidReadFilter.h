#pragma once

#include <windows.h>

namespace ACSEFilter
{

    void PatchCompletedRead(HANDLE file, void *buffer, DWORD actualBytes, DWORD requestedBytes);
    void RememberPendingRead(HANDLE file, void *buffer, DWORD requestedBytes, LPOVERLAPPED overlapped);
    HANDLE PendingReadWaitHandle();
    void CompletePendingRead(LPOVERLAPPED overlapped, DWORD actualBytes);
    void CompletePendingReadAfterWait();
    void DropPendingRead(LPOVERLAPPED overlapped);

} // namespace ACSEFilter
