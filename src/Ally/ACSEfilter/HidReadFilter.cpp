#include "HidReadFilter.h"

#include "Config.h"
#include "DebugLog.h"
#include "Native.h"

#include <hidsdi.h>

namespace ACSEFilter
{
    namespace
    {

        struct ThreadReadState
        {
            HANDLE targetFile = nullptr;
            void *pendingBuffer = nullptr;
            DWORD pendingRequestedBytes = 0;
            LPOVERLAPPED pendingOverlapped = nullptr;
            HANDLE pendingEvent = nullptr;
        };

        thread_local ThreadReadState readState;

        bool IsValidHandle(HANDLE file)
        {
            return file && file != INVALID_HANDLE_VALUE;
        }

        bool ProbeBufferContent(BYTE *buffer, DWORD length)
        {
            if (!buffer || length != Config::kExpectedReadLength)
            {
                return false;
            }

            if (buffer[0] != 0x5A)
            {
                return false;
            }

            for (DWORD i = 2; i < length; ++i)
            {
                if (buffer[i] != 0)
                {
                    return false;
                }
            }

            for (const auto& key : Config::kReplacementKeys)
            {
                if (key == buffer[1])
                {
                    return true;
                }
            }

            return false;
        }

        bool ProbeHidHandle(HANDLE file)
        {
            HIDD_ATTRIBUTES attributes = { sizeof(HIDD_ATTRIBUTES) };

            if (!HidD_GetAttributes(file, &attributes))
            {
                return false;
            }

            if (attributes.VendorID != Config::kTargetVendorId)
            {
                return false;
            }

            for (const auto& productId : Config::kTargetProductIds)
            {
                if (productId == attributes.ProductID)
                {
                    DEBUG(L"Correct HID device read: Handle %p: VID_%04X PID_%04X.", file, attributes.VendorID, attributes.ProductID);
                    return true;
                }
            }

            return false;
        }

        bool IsExpectedRead(HANDLE file, void *buffer, DWORD requestedBytes)
        {
            return
                IsValidHandle(file)
                && buffer
                && requestedBytes == Config::kExpectedReadLength;
        }

        bool ValidateTargetRead(HANDLE file, void *buffer, DWORD requestedBytes, DWORD actualBytes)
        {
            if (!IsExpectedRead(file, buffer, requestedBytes))
            {
                return false;
            }

            if (!ProbeBufferContent(static_cast<BYTE *>(buffer), actualBytes))
            {
                return false;
            }

            if (readState.targetFile == file)
            {
                return true;
            }

            if (!ProbeHidHandle(file))
            {
                return false;
            }

            readState.targetFile = file;
            return true;
        }

        bool ShouldRememberPendingRead(HANDLE file, void *buffer, DWORD requestedBytes, LPOVERLAPPED overlapped)
        {
            if (!overlapped || !IsExpectedRead(file, buffer, requestedBytes))
            {
                return false;
            }

            if (readState.targetFile == file)
            {
                return true;
            }

            if (!ProbeHidHandle(file))
            {
                return false;
            }

            readState.targetFile = file;
            return true;
        }

        bool IsPendingRead(LPOVERLAPPED overlapped)
        {
            return overlapped && readState.pendingOverlapped == overlapped;
        }

        void ClearPendingRead()
        {
            readState.pendingBuffer = nullptr;
            readState.pendingRequestedBytes = 0;
            readState.pendingOverlapped = nullptr;
            readState.pendingEvent = nullptr;
        }

    }

    void PatchCompletedRead(HANDLE file, void *buffer, DWORD actualBytes, DWORD requestedBytes)
    {
        if (!IsExpectedRead(file, buffer, requestedBytes))
        {
            return;
        }

        if ( actualBytes == Config::kExpectedReadLength)
        {
            DEBUG(L"Read data: %02X %02X %02X %02X %02X %02X",
                  ((BYTE*)buffer)[0], ((BYTE*)buffer)[1], ((BYTE*)buffer)[2],
                  ((BYTE*)buffer)[3], ((BYTE*)buffer)[4], ((BYTE*)buffer)[5]);
        }

        if (!ValidateTargetRead(file, buffer, requestedBytes, actualBytes))
        {
            DEBUG(L"Pass unchanged");
            return;
        }

        DEBUG(L"Key code to hide 0x%02X detected. Replace by 0x00", ((BYTE*)buffer)[1]);

        CopyMemory(buffer, Config::kReplacementBytes, Config::kExpectedReadLength);
    }

    void RememberPendingRead(HANDLE file, void *buffer, DWORD requestedBytes, LPOVERLAPPED overlapped)
    {
        if (!ShouldRememberPendingRead(file, buffer, requestedBytes, overlapped))
        {
            ClearPendingRead();
            return;
        }

        readState.pendingBuffer = buffer;
        readState.pendingRequestedBytes = requestedBytes;
        readState.pendingOverlapped = overlapped;
        readState.pendingEvent = overlapped->hEvent;
    }

    HANDLE PendingReadWaitHandle()
    {
        return readState.pendingEvent;
    }

    void CompletePendingRead(LPOVERLAPPED overlapped, DWORD actualBytes)
    {
        if (!IsPendingRead(overlapped))
        {
            return;
        }

        HANDLE file = readState.targetFile;
        void *buffer = readState.pendingBuffer;
        const DWORD requestedBytes = readState.pendingRequestedBytes;

        ClearPendingRead();
        PatchCompletedRead(file, buffer, actualBytes, requestedBytes);
    }

    void CompletePendingReadAfterWait()
    {
        LPOVERLAPPED overlapped = readState.pendingOverlapped;
        if (!overlapped)
        {
            return;
        }

        HANDLE file = readState.targetFile;
        DWORD actualBytes = 0;
        if (Native::GetOverlappedResult(file, overlapped, &actualBytes, FALSE))
        {
            CompletePendingRead(overlapped, actualBytes);
            return;
        }

        const DWORD lastError = GetLastError();
        if (lastError != ERROR_IO_INCOMPLETE)
        {
            DropPendingRead(overlapped);
        }
    }

    void DropPendingRead(LPOVERLAPPED overlapped)
    {
        if (IsPendingRead(overlapped))
        {
            ClearPendingRead();
        }
    }

} // namespace ACSEFilter
