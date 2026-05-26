#pragma once

#include <windows.h>

namespace ACSEFilter
{

    bool ShouldPatchRead(HANDLE file, void *buffer, DWORD requestedBytes, DWORD actualBytes);
    void PatchCompletedRead(HANDLE file, void *buffer, DWORD actualBytes, DWORD requestedBytes);
    void RememberPendingRead(HANDLE file, void *buffer, DWORD requestedBytes, LPOVERLAPPED overlapped);
    HANDLE PendingReadWaitHandle();
    void CompletePendingRead(LPOVERLAPPED overlapped, DWORD actualBytes);
    void CompletePendingReadAfterWait();
    void DropPendingRead(LPOVERLAPPED overlapped);

} // namespace ACSEFilter
